/*
*  Copyright 2014 The Luvit Authors. All Rights Reserved.
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
*
*/
#include "private.h"

typedef struct {
  uv_thread_t handle;

  // bytecode of the thread entrypoint
  const char* code;
  size_t code_len;

  // the lua state that created this luv_thread_t
  lua_State *owner;

  // used to wait for thread initialization
  uv_sem_t sem;

  // used to wait for thread exit to ensure the thread exits before the main thread
  uv_async_t exit_notify;
  int thread_ref;

  luv_thread_args_t *args;
} luv_thread_t;

static lua_State* luv_thread_acquire_vm(void) {
  lua_State* L = luaL_newstate();

  // Add in the lua standard libraries
  luaL_openlibs(L);

  // Get package.loaded, so we can store uv in it.
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaded");
  lua_remove(L, -2); // Remove package

  // Store uv module definition at loaded.uv
  luaopen_luv(L);
  lua_setfield(L, -2, "luv");
  lua_pop(L, 1);

  return L;
}

static void luv_thread_release_vm(lua_State* L) {
  lua_close(L);
}

// Ensure that the arguments are valid for passing to a thread.
static void luv_thread_args_check(lua_State *L, int idx, int top) {
  if (top - idx >= LUV_THREAD_MAXNUM_ARG) {
    luaL_error(L, "too many thread arguments, max %d", LUV_THREAD_MAXNUM_ARG);
  }

  idx = lua_absindex(L, idx);
  top = lua_absindex(L, top);

  for (int i = idx; i <= top; i++) {
    switch (lua_type(L, i)) {
    case LUA_TNIL:
    case LUA_TBOOLEAN:
    case LUA_TNUMBER:
    case LUA_TSTRING:
    case LUA_TLIGHTUSERDATA:
      break;
    case LUA_TUSERDATA:
      if (lua_getmetatable(L, i)) {
        lua_getfield(L, -1, "__name");
        const char* name = lua_tostring(L, -1);

        if (!name) 
          luaL_argerror(L, i, "cannot pass unnamed full userdata to another thread");

        lua_getfield(L, -2, "__thread");
        if (lua_isboolean(L, -1)) {
          if (!lua_toboolean(L, -1)) {
            luaL_argerror(L, i, lua_pushfstring(L, "'%s' cannot be passed to another thread, explicitly disallowed", name));
          }

          luaL_getmetatable(L, name);
          if (!lua_rawequal(L, -1, -4)) {
            luaL_argerror(L, i, lua_pushfstring(L, "'%s' cannot be passed to another thread, metatable name mismatch", name));
          }

          lua_getfield(L, -1, "__gc");
          if (lua_isfunction(L, -1)) {
            luaL_argerror(L, i, lua_pushfstring(L, "'%s' cannot be passed to another thread, custom garbage collector requires thread constructor", name));
          }

          lua_pop(L, 5);
        } else if (lua_iscfunction(L, -1)) {
          lua_pop(L, 3);
        } else {
          luaL_argerror(L, i, lua_pushfstring(L, "'%s' cannot be passed to another thread", name));
        }
      }
      break;
    default:
      luaL_argerror(L, i, "thread argument not supported");
    }
  }
}

// initialize arguments to pass to a different thread
static void luv_thread_args_prepare(lua_State *L, luv_ctx_t *ctx, luv_thread_args_t* args, int idx, int top, int flags) {
  args->producer = L;
  args->flags = flags;
  args->argc = top - idx + 1;

  // async requires us to copy any persistent data because we don't know when it will be used.
  int is_async = LUVF_THREAD_ASYNC(flags);

  if (idx + LUV_THREAD_MAXNUM_ARG < top) {
    top = idx + LUV_THREAD_MAXNUM_ARG - 1;
  }

  for (unsigned int i = idx; i <= top; i++) {
    luv_thread_arg_t *arg = &args->argv[i - idx];
    arg->type = lua_type(L, i);
    switch (arg->type) {
      case LUA_TNIL:
        break;
      case LUA_TBOOLEAN:
        arg->boolean = lua_toboolean(L, i);
        break;
      case LUA_TNUMBER:
        arg->number.isinteger = lua_isinteger(L, i);
        if (arg->number.isinteger) {
          arg->number.integer = lua_tointeger(L, i);
        } else {
          arg->number.number = lua_tonumber(L, i);
        }
        break;
      case LUA_TSTRING:
        if (is_async) {
          const char* base = lua_tolstring(L, i, &arg->string.len);
          arg->string.base = malloc(arg->string.len);
          if (!arg->string.base) {
            luaL_error(L, "out of memory");
          }

          memcpy((void*)arg->string.base, (const void*)base, arg->string.len);
        } else {
          arg->string.base = lua_tolstring(L, i, &arg->string.len);
        }
        break;
      case LUA_TLIGHTUSERDATA:
        arg->userdata.data = lua_touserdata(L, i);
        arg->userdata.size = 0;
        break;
      case LUA_TUSERDATA:
        if (!lua_getmetatable(L, i)) {
          luaL_argerror(L, i, "thread argument not supported");
        } 

        lua_getfield(L, -1, "__thread");
        if (lua_isboolean(L, -1)) {
          luaL_argcheck(L, lua_toboolean(L, -1), i, "thread argument not supported");

          arg->userdata.data = lua_touserdata(L, i);
          arg->userdata.size = lua_rawlen(L, i);
          arg->userdata.constructor = NULL;
        } else if (lua_iscfunction(L, -1)) {
          lua_CFunction constructor = lua_tocfunction(L, -1);

          arg->userdata.data = lua_touserdata(L, i);
          arg->userdata.size = lua_rawlen(L, i);
          arg->userdata.constructor = constructor;
        } else {
          luaL_argerror(L, i, "thread argument not supported");
        }

        // store the name of the userdata
        lua_getfield(L, -2, "__name");
        if (is_async) {
          size_t len;
          const char* name = lua_tolstring(L, -1, &len);
          
          arg->userdata.name = malloc(len);
          if (!arg->userdata.name) {
            luaL_error(L, "out of memory");
          }

          memcpy((void*)arg->userdata.name, (const void*)name, len);
          lua_pop(L, 1); // pop __name
        } else {
          arg->userdata.name = lua_tostring(L, -1);
          arg->userdata.name_ref = luaL_ref(L, LUA_REGISTRYINDEX);
        }

        lua_pop(L, 2); // pop metatable and __thread

        // store reference to the userdata
        lua_pushvalue(L, i);
        arg->userdata.producer_ref = luaL_ref(L, LUA_REGISTRYINDEX);

        // if we know the userdata already exists on the other side, tell the other side where to find it
        lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->ut_ref);
        arg->userdata.consumer_ref = LUA_NOREF;
        if (lua_rawgetp(L, -1, args->consumer) == LUA_TTABLE) {
          if (lua_rawgetp(L, -1, arg->userdata.data) == LUA_TNUMBER) {
            arg->userdata.consumer_ref = (int)lua_tointeger(L, -1);
          }
        }

        lua_pop(L, 2); // pop ut_ref table and reference
        break;
      default:
        luaL_argerror(L, i, "thread argument not supported");
        break;
    }
  }
}

static int luv_thread_args_push(lua_State* L, luv_ctx_t* ctx, const luv_thread_args_t* args) {
  for (unsigned int i = 0; i < args->argc; i++) {
    const luv_thread_arg_t* arg = &args->argv[i];
    switch (arg->type) {
    case LUA_TNIL:
      lua_pushnil(L);
      break;
    case LUA_TBOOLEAN:
      lua_pushboolean(L, arg->boolean);
      break;
    case LUA_TNUMBER:
      if (arg->number.isinteger) {
        lua_pushinteger(L, arg->number.integer);
      } else {
        lua_pushnumber(L, arg->number.number);
      }
      break;
    case LUA_TSTRING:
      lua_pushlstring(L, arg->string.base, arg->string.len);
      break;
    case LUA_TLIGHTUSERDATA:
      lua_pushlightuserdata(L, (void*)arg->userdata.data);
      break;
    case LUA_TUSERDATA:
      // if we know the userdata already exists here, we can just push it
      if (arg->userdata.consumer_ref != LUA_NOREF) {
        lua_rawgeti(L, LUA_REGISTRYINDEX, arg->userdata.consumer_ref);
        if (!lua_isuserdata(L, -1)) {
          luaL_error(L, "thread argument reference not found");
        }

        continue;
      }

      if (arg->userdata.constructor) {
        lua_pushcfunction(L, arg->userdata.constructor);
        lua_pushlightuserdata(L, (void*)arg->userdata.data);
        lua_pushinteger(L, (lua_Integer)arg->userdata.size);
        lua_pushstring(L, arg->userdata.name);
        lua_call(L, 3, 1);

        if (!lua_isuserdata(L, -1)) {
          luaL_error(L, "thread argument constructor failed");
        }
      } else {
        if (!arg->userdata.name) {
          luaL_error(L, "thread argument name not found");
        }

        char *p = lua_newuserdata(L, arg->userdata.size);
        memcpy(p, arg->userdata.data, arg->userdata.size);
        luaL_getmetatable(L, arg->userdata.name);
        if (lua_isnil(L, -1)) {
          luaL_error(L, "thread argument metatable not found");
        }

        lua_setmetatable(L, -2);
      }

      lua_rawgeti(L, LUA_REGISTRYINDEX, ctx->ut_ref);
      if (lua_rawgetp(L, -1, args->producer) != LUA_TTABLE) {
        lua_pop(L, 1);
        
        lua_newtable(L);
        lua_getfield(L, -2, "mt");
        lua_setmetatable(L, -2);

        lua_pushvalue(L, -1);
        lua_rawsetp(L, -2, args->producer);
      }

      lua_pushinteger(L, arg->userdata.producer_ref);
      lua_rawsetp(L, -2, lua_touserdata(L, -3));
      lua_pop(L, 2); // pop ut_ref table and producer table

      break;
    default:
      luaL_error(L, "thread argument not supported");
    }
  }
  return args->argc;
}

static void luv_thread_args_cleanup(lua_State* L, luv_thread_args_t* args) {
  int is_async = LUVF_THREAD_ASYNC(args->flags);

  for (int i = 0; i < args->argc; i++) {
    luv_thread_arg_t* arg = &args->argv[i];
    switch (arg->type) {
    case LUA_TSTRING:
      if (is_async) {
        free((void*)arg->string.base);
      }
      break;
    case LUA_TUSERDATA:
      if (is_async) {
        free((void*)arg->userdata.name);
      } else {
        luaL_unref(L, LUA_REGISTRYINDEX, arg->userdata.name_ref);
        arg->userdata.name_ref = LUA_NOREF;
        arg->userdata.name = NULL;
      }
      break;
    default:
      break;
    }
  }
}

static int luv_write_buffer(lua_State *L, const void *b, size_t size, void *ud) {
  (void)L;

  luaL_addlstring((luaL_Buffer *)ud, (const char *)b, size);
  return 0;
}

static void luv_thread_prepare_entrypoint(lua_State* L, int idx) {
  // Assume any string is already bytecode.
  if (lua_isstring(L, idx)) {
    lua_pushvalue(L, idx);
    return;
  }

  // Ensure we have an absolute index to the function because luaL_buffer pushes items onto the stack.
  idx = lua_absindex(L, idx);
  luaL_checktype(L, idx, LUA_TFUNCTION);

  luaL_Buffer B;
  luaL_buffinit(L, &B);

  // luv_write_buffer cannot return non-zero, so this must succeed.
  lua_pushvalue(L, idx);
  lua_dump(L, luv_write_buffer, &B, 0);

  lua_pop(L, 1); // pop the function
  luaL_pushresult(&B);
}

static luv_thread_t* luv_check_thread(lua_State* L, int index) {
  luv_thread_t* thread = (luv_thread_t*)luaL_checkudata(L, index, "uv_thread");
  return thread;
}

static int luv_thread_tostring(lua_State* L) {
  luv_thread_t* thd = luv_check_thread(L, 1);
  lua_pushfstring(L, "uv_thread_t: %p", (void*)thd->handle);
  return 1;
}

static void luv_thread_cb(void* varg) {
  luv_thread_t* thrd = (luv_thread_t*)varg;

  // acquire a lua state for the thread
  lua_State* L = acquire_vm_cb();
  luv_ctx_t *ctx = luv_context(L);

  lua_pushboolean(L, 1);
  lua_setglobal(L, "_THREAD");

  // push lua function for thread entry
  if (luaL_loadbuffer(L, thrd->code, thrd->code_len, "=thread") == 0) {
    // push parameters given to thread
    int i = luv_thread_args_push(L, ctx, thrd->args);

    // notify the main thread that we're done initializing and will not access args anymore.
    uv_sem_post(&thrd->sem);

    ctx->thrd_pcall(L, i, 0, LUVF_CALLBACK_NOEXIT);
  } else {
    fprintf(stderr, "Uncaught Error in thread: %s\n", lua_tostring(L, -1));
    lua_pop(L, 1);
  }

  release_vm_cb(L);

  // Notify the thread exit callback on the main thread.
  uv_async_send(&thrd->exit_notify);
}

static void luv_thread_notify_close_cb(uv_handle_t *handle) {
  luv_thread_t *thread = handle->data;
  if (thread->handle != 0)
    uv_thread_join(&thread->handle);

  luaL_unref(thread->owner, LUA_REGISTRYINDEX, thread->thread_ref);
  thread->thread_ref = LUA_NOREF;
  thread->owner = NULL;
}

static void luv_thread_exit_cb(uv_async_t* handle) {
  uv_close((uv_handle_t*)handle, luv_thread_notify_close_cb);
}

static int luv_new_thread(lua_State* L) {
  int ret;
  luv_thread_args_t args;
  luv_thread_t* thread;
  luv_ctx_t* ctx = luv_context(L);
  
  int callback_idx = 1;
#if LUV_UV_VERSION_GEQ(1, 26, 0)
  uv_thread_options_t options;
  options.flags = UV_THREAD_NO_FLAGS;
  if (lua_istable(L, 1)) {
    callback_idx++;

    lua_getfield(L, 1, "stack_size");
    if (lua_isnumber(L, -1)) {
      options.flags |= UV_THREAD_HAS_STACK_SIZE;
      options.stack_size = lua_tointeger(L, -1);
    } else if (!lua_isnil(L, -1)) {
      return luaL_argerror(L, 1, "stack_size option must be a number if set");
    }
    lua_pop(L, 1);
  }
#endif

  int top = lua_gettop(L);

  // ensure we can handle all of the thread arguments before we do anything.
  luv_thread_args_check(L, callback_idx + 1, top);
  luv_thread_prepare_entrypoint(L, callback_idx);

  size_t code_len;
  const char* code = lua_tolstring(L, -1, &code_len);

  thread = (luv_thread_t*)lua_newuserdata(L, sizeof(*thread));
  luaL_getmetatable(L, "uv_thread");
  lua_setmetatable(L, -2);

  thread->code = code;
  thread->code_len = code_len;

  args.consumer = NULL;
  luv_thread_args_prepare(L, ctx, &args, callback_idx + 1, top, 0);
  thread->args = &args;

  thread->exit_notify.data = thread;
  thread->owner = L;

  thread->thread_ref = LUA_NOREF;
  ret = uv_async_init(ctx->loop, &thread->exit_notify, luv_thread_exit_cb);
  if (ret < 0)
    return luv_error(L, ret);

  lua_pushvalue(L, -1);
  thread->thread_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  uv_sem_init(&thread->sem, 0);
  thread->handle = 0;
#if LUV_UV_VERSION_GEQ(1, 26, 0)
  ret = uv_thread_create_ex(&thread->handle, &options, luv_thread_cb, thread);
#else
  ret = uv_thread_create(&thread->handle, luv_thread_cb, thread);
#endif
  if (ret < 0) {
    luv_thread_args_cleanup(L, &args);
    uv_close((uv_handle_t*)&thread->exit_notify, luv_thread_notify_close_cb);
    return luv_error(L, ret);
  }

  uv_sem_wait(&thread->sem);
  luv_thread_args_cleanup(L, &args);

  thread->code = NULL;
  thread->code_len = 0;

  return 1;
}

#if LUV_UV_VERSION_GEQ(1, 45, 0)
static int luv_thread_getaffinity(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  int default_mask_size = uv_cpumask_size();
  if (default_mask_size < 0) {
    return luv_error(L, default_mask_size);
  }
  int mask_size = luaL_optinteger(L, 2, default_mask_size);
  if (mask_size < default_mask_size) {
    return luaL_argerror(L, 2, lua_pushfstring(L, "cpumask size must be >= %d (from cpumask_size()), got %d", default_mask_size, mask_size));
  }
  char* cpumask = malloc(mask_size);
  int ret = uv_thread_getaffinity(&tid->handle, cpumask, mask_size);
  if (ret < 0) {
    free(cpumask);
    return luv_error(L, ret);
  }
  lua_newtable(L);
  for (int i = 0; i < mask_size; i++) {
    lua_pushboolean(L, cpumask[i]);
    lua_rawseti(L, -2, i + 1);
  }
  free(cpumask);
  return 1;
}

static int luv_thread_setaffinity(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  luaL_checktype(L, 2, LUA_TTABLE);
  int get_old_mask = lua_toboolean(L, 3);
  int min_mask_size = uv_cpumask_size();
  if (min_mask_size < 0) {
    return luv_error(L, min_mask_size);
  }
  int mask_size = lua_rawlen(L, 2);
  // If the provided table's length is not at least min_mask_size,
  // we'll use the min_mask_size and fill in any missing values with
  // false.
  if (mask_size < min_mask_size) {
    mask_size = min_mask_size;
  }
  char* cpumask = malloc(mask_size);
  for (int i = 0; i < mask_size; i++) {
    lua_rawgeti(L, 2, i+1);
    int val = lua_toboolean(L, -1);
    cpumask[i] = val;
    lua_pop(L, 1);
  }
  char* oldmask = get_old_mask ? malloc(mask_size) : NULL;
  int ret = uv_thread_setaffinity(&tid->handle, cpumask, oldmask, mask_size);
  // Done with cpumask at this point
  free(cpumask);
  if (ret < 0) {
    if (get_old_mask) free(oldmask);
    return luv_error(L, ret);
  }
  if (get_old_mask) {
    lua_newtable(L);
    for (int i = 0; i < mask_size; i++) {
      lua_pushboolean(L, oldmask[i]);
      lua_rawseti(L, -2, i + 1);
    }
    free(oldmask);
  }
  else {
    lua_pushboolean(L, 1);
  }
  return 1;
}

static int luv_thread_getcpu(lua_State* L) {
  int ret = uv_thread_getcpu();
  if (ret < 0) return luv_error(L, ret);
  // Make the returned value start at 1 to match how getaffinity/setaffinity
  // masks are implemented (they use array-like tables so the first
  // CPU is index 1).
  lua_pushinteger(L, ret + 1);
  return 1;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 48, 0)
static int luv_thread_getpriority(lua_State* L) {
  int priority;
  luv_thread_t* tid = luv_check_thread(L, 1);
  int ret = uv_thread_getpriority(tid->handle, &priority);
  if (ret < 0) return luv_error(L, ret);
  lua_pushinteger(L, priority);
  return 1;
}

static int luv_thread_setpriority(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  int priority = luaL_checkinteger(L, 2);
  int ret = uv_thread_setpriority(tid->handle, priority);
  if (ret < 0) return luv_error(L, ret);
  lua_pushboolean(L, 1);
  return 1;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 50, 0)
static int luv_thread_detach(lua_State *L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  int ret = uv_thread_detach(&tid->handle);
  if (ret < 0) return luv_error(L, ret);
  tid->handle = 0;
  lua_pushboolean(L, 1);
  return 1;
}

static int luv_thread_getname(lua_State *L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  char name[MAX_THREAD_NAME_LENGTH];
  int ret = uv_thread_getname(&tid->handle, name, MAX_THREAD_NAME_LENGTH);
  if (ret < 0) return luv_error(L, ret);
  lua_pushstring(L, name);
  return 1;
}

static int luv_thread_setname(lua_State *L) {
  const char* name = luaL_checkstring(L, 1);
  int ret = uv_thread_setname(name);
  return luv_result(L, ret);
}
#endif

static int luv_thread_join(lua_State* L) {
  luv_thread_t* tid = luv_check_thread(L, 1);
  if (tid->handle == 0)
    luaL_error(L, "thread already joined");
  
  int ret = uv_thread_join(&tid->handle);
  if (ret < 0) return luv_error(L, ret);
  tid->handle = 0;
  lua_pushboolean(L, 1);
  return 1;
}

static int luv_thread_self(lua_State* L) {
  luv_thread_t* thread;
  uv_thread_t t = uv_thread_self();
  thread = (luv_thread_t*)lua_newuserdata(L, sizeof(*thread));
  memset(thread, 0, sizeof(*thread));
  memcpy(&thread->handle, &t, sizeof(t));
  luaL_getmetatable(L, "uv_thread");
  lua_setmetatable(L, -2);
  return 1;
}

static int luv_thread_equal(lua_State* L) {
  luv_thread_t* t1 = luv_check_thread(L, 1);
  luv_thread_t* t2 = luv_check_thread(L, 2);
  int ret = uv_thread_equal(&t1->handle, &t2->handle);
  lua_pushboolean(L, ret);
  return 1;
}

static const luaL_Reg luv_thread_methods[] = {
  {"equal", luv_thread_equal},
  {"join", luv_thread_join},
#if LUV_UV_VERSION_GEQ(1, 45, 0)
  {"getaffinity", luv_thread_getaffinity},
  {"setaffinity", luv_thread_setaffinity},
  {"getcpu", luv_thread_getcpu},
#endif
#if LUV_UV_VERSION_GEQ(1, 48, 0)
  {"getpriority", luv_thread_getpriority},
  {"setpriority", luv_thread_setpriority},
#endif
#if LUV_UV_VERSION_GEQ(1, 50, 0)
  {"setname", luv_thread_setname},
  {"detach", luv_thread_detach},
#endif
  {NULL, NULL}
};

static void luv_thread_init(lua_State* L) {
  luaL_newmetatable(L, "uv_thread");
  lua_pushcfunction(L, luv_thread_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pushcfunction(L, luv_thread_equal);
  lua_setfield(L, -2, "__eq");
  lua_newtable(L);
  luaL_setfuncs(L, luv_thread_methods, 0);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  if (acquire_vm_cb == NULL) acquire_vm_cb = luv_thread_acquire_vm;
  if (release_vm_cb == NULL) release_vm_cb = luv_thread_release_vm;
}

LUALIB_API void luv_set_thread_cb(luv_acquire_vm acquire, luv_release_vm release)
{
  acquire_vm_cb = acquire;
  release_vm_cb = release;
}
