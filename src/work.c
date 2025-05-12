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
  lua_State** vms;
  unsigned int nvms;
  unsigned int idx_vms;
  uv_mutex_t vm_mutex;
} luv_work_vms_t;

typedef struct {
  // bytecode of the worker entrypoint
  const char* code;
  size_t code_len;

  // state that will be running the after work callback
  lua_State* owner;
  // reference to the callback for after work
  int after_work_ref;

  // userdata owned by owner so the parent can clean up old states
  luv_work_vms_t* vms;
} luv_work_ctx_t;

typedef struct {
  uv_work_t work;
  luv_work_ctx_t* ctx;

  // on entry used to pass data from main to worker, on exit used to pass
  // data from worker to main
  luv_thread_args_t args;

  // reference to the luv_work_ctx_t userdata to keep it alive
  int ref;
} luv_work_t;

static uv_once_t once_vmkey = UV_ONCE_INIT;
static uv_key_t tls_vmkey;  /* thread local storage key for Lua state */

#if LUV_UV_VERSION_GEQ(1, 30, 0)
#define MAX_THREADPOOL_SIZE 1024
#else
#define MAX_THREADPOOL_SIZE 128
#endif

static luv_work_ctx_t* luv_check_work_ctx(lua_State* L, int index) {
  luv_work_ctx_t* ctx = (luv_work_ctx_t*)luaL_checkudata(L, index, "luv_work_ctx");
  return ctx;
}

static int luv_work_ctx_gc(lua_State *L) {
  luv_work_ctx_t* ctx = luv_check_work_ctx(L, 1);

  free((void*)ctx->code);
  luaL_unref(L, LUA_REGISTRYINDEX, ctx->after_work_ref);

  return 0;
}

static int luv_work_ctx_tostring(lua_State* L) {
  luv_work_ctx_t* ctx = luv_check_work_ctx(L, 1);
  lua_pushfstring(L, "luv_work_ctx_t: %p", ctx);
  return 1;
}

static int luv_work_cb(lua_State* L) {
  uv_work_t* req = lua_touserdata(L, 1);
  luv_work_t* work = (luv_work_t*)req->data;
  luv_work_ctx_t* ctx = work->ctx;
  luv_ctx_t *lctx = luv_context(L);
  lua_pop(L, 1);

  int top = lua_gettop(L);

  /* push lua function */
  lua_rawgetp(L, LUA_REGISTRYINDEX, ctx->code);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);

    if (luaL_loadbuffer(L, ctx->code, ctx->code_len, "=pool") != 0) {
      fprintf(stderr, "Uncaught Error in work callback: %s\n", lua_tostring(L, -1));
      lua_pop(L, 1);

      lua_pushnil(L);
    } else {
      lua_pushvalue(L, -1);
      lua_rawsetp(L, LUA_REGISTRYINDEX, ctx->code);
    }
  }

  if (lua_isfunction(L, -1)) {
    int nargs = luv_thread_args_push(L, lctx, &work->args);
    luv_thread_args_cleanup(L, &work->args);

    // If exit is called on a thread in the thread pool, abort is called in
    // uv__threadpool_cleanup, so exit is not called in luv_cfpcall.
    int nret = lctx->thrd_pcall(L, nargs, LUA_MULTRET, LUVF_CALLBACK_NOEXIT);
    if (nret >= 0) {
      int new_top = lua_gettop(L);
      luv_thread_args_check(L, top, new_top);
      luv_thread_args_prepare(L, lctx, &work->args, top, new_top, LUVF_THREAD_MODE_ASYNC);
    }
    lua_pop(L, nret);
  } else {
    lua_pop(L, 1);
    return luaL_error(L, "Uncaught Error: %s can't be work entry\n", lua_typename(L, lua_type(L,-1)));
  }

  if (top != lua_gettop(L))
    return luaL_error(L, "stack not balance in luv_work_cb, need %d but %d", top, lua_gettop(L));

  return LUA_OK;
}

static lua_State* luv_work_acquire_vm(luv_work_vms_t* vms) {
  lua_State* L = uv_key_get(&tls_vmkey);
  if (L == NULL) {
    L = acquire_vm_cb();
    uv_key_set(&tls_vmkey, L);
    lua_pushboolean(L, 1);
    lua_setglobal(L, "_THREAD");

    uv_mutex_lock(&vms->vm_mutex);
    vms->vms[vms->idx_vms] = L;
    vms->idx_vms += 1;
    uv_mutex_unlock(&vms->vm_mutex);
  }
  return L;
}

static int luv_work_cleanup(lua_State *L) {
  luv_work_vms_t *vms = (luv_work_vms_t*)lua_touserdata(L, 1);
  
  if (!vms || vms->nvms == 0)
    return 0;

  for (unsigned int i = 0; i < vms->nvms && vms->vms[i]; i++)
    release_vm_cb(vms->vms[i]);

  free(vms->vms);

  uv_mutex_destroy(&vms->vm_mutex);
  vms->nvms = 0;
  return 0;
}

static void luv_work_cb_wrapper(uv_work_t* req) {
  luv_work_t *work = (luv_work_t*)req->data;
  lua_State *L = luv_work_acquire_vm(work->ctx->vms);

  // If exit is called on a thread in the thread pool, abort is called in
  // uv__threadpool_cleanup, so exit is not called in luv_cfpcall.
  luv_context(L)->thrd_cpcall(L, luv_work_cb, (void*)req, LUVF_CALLBACK_NOEXIT);
}

static void luv_after_work_cb(uv_work_t* req, int status) {
  luv_work_t *work = (luv_work_t*)req->data;
  luv_work_ctx_t *work_ctx = work->ctx;
  lua_State *L = work_ctx->owner;
  luv_ctx_t *ctx = luv_context(L);

  (void)status;

  lua_rawgeti(L, LUA_REGISTRYINDEX, work_ctx->after_work_ref);
  int i = luv_thread_args_push(L, ctx, &work->args);
  ctx->cb_pcall(L, i, 0, 0);

  // remove our reference to the work context
  luaL_unref(L, LUA_REGISTRYINDEX, work->ref);
  work->ref = LUA_NOREF;

  luv_thread_args_cleanup(L, &work->args);
  free(work);
}

static int luv_new_work(lua_State* L) {
  luv_work_ctx_t* ctx;
  ctx = (luv_work_ctx_t*)lua_newuserdata(L, sizeof(*ctx));
  memset(ctx, 0, sizeof(*ctx));
  
  luaL_getmetatable(L, "luv_work_ctx");
  lua_setmetatable(L, -2);
  
  ctx->owner = L;

  luv_check_callable(L, 2);
  lua_pushvalue(L, 2);
  ctx->after_work_ref = luaL_ref(L, LUA_REGISTRYINDEX);

  luv_thread_prepare_entrypoint(L, 1);
  const char* code = lua_tolstring(L, -1, &ctx->code_len);
  ctx->code = malloc(ctx->code_len);
  if (!ctx->code) {
    luaL_error(L, "out of memory");
  }

  memcpy((void*)ctx->code, code, ctx->code_len);
  lua_pop(L, 1);

  lua_rawgetp(L, LUA_REGISTRYINDEX, &luv_work_cleanup);
  ctx->vms = (luv_work_vms_t*)lua_touserdata(L, -1);
  lua_pop(L, 1);

  return 1;
}

static int luv_queue_work(lua_State* L) {
  luv_work_ctx_t* ctx = luv_check_work_ctx(L, 1);
  int top = lua_gettop(L);

  luv_thread_args_check(L, 2, top);

  luv_work_t* work = (luv_work_t*)malloc(sizeof(*work));
  memset(work, 0, sizeof(*work));

  work->ctx = ctx;
  work->work.data = work;

  luv_thread_args_prepare(L, luv_context(L), &work->args, 2, top, LUVF_THREAD_MODE_ASYNC);

  int ret = uv_queue_work(luv_loop(L), &work->work, luv_work_cb_wrapper, luv_after_work_cb);
  if (ret < 0) {
    luv_thread_args_cleanup(L, &work->args);
    free(work);
    return luv_error(L, ret);
  }

  // ref up to ctx
  lua_pushvalue(L, 1);
  work->ref = luaL_ref(L, LUA_REGISTRYINDEX);

  lua_pushboolean(L, 1);
  return 1;
}

static const luaL_Reg luv_work_ctx_methods[] = {
  {"queue", luv_queue_work},
  {NULL, NULL}
};

static void luv_key_init_once(void) {
  int status = uv_key_create(&tls_vmkey);
  if (status != 0) {
    fprintf(stderr, "*** threadpool not works\n");
    fprintf(stderr, "Error to uv_key_create with %s: %s\n",
      uv_err_name(status), uv_strerror(status));
    abort();
  }
}

static void luv_work_init(lua_State* L) {
  luaL_newmetatable(L, "luv_work_ctx");
  lua_pushcfunction(L, luv_work_ctx_tostring);
  lua_setfield(L, -2, "__tostring");
  lua_pushcfunction(L, luv_work_ctx_gc);
  lua_setfield(L, -2, "__gc");
  luaL_newlib(L, luv_work_ctx_methods);
  lua_setfield(L, -2, "__index");
  lua_pop(L, 1);

  luaL_newmetatable(L, "luv_work_vms");
  lua_pushcfunction(L, luv_work_cleanup);
  lua_setfield(L, -2, "__gc");
  lua_pop(L, 1);
  
  /* ref to https://github.com/libuv/libuv/blob/v1.x/src/threadpool.c init_threads */
  const char* val;
  unsigned int nvms = 4;
  val = getenv("UV_THREADPOOL_SIZE");
  if (val != NULL)
    nvms = atoi(val);
  if (nvms == 0)
    nvms = 1;
  if (nvms > MAX_THREADPOOL_SIZE)
    nvms = MAX_THREADPOOL_SIZE;

  luv_work_vms_t* vms = (luv_work_vms_t*)lua_newuserdata(L, sizeof(luv_work_vms_t));
  int status = uv_mutex_init(&vms->vm_mutex);
  if (status != 0)
  {
    fprintf(stderr, "*** threadpool not works\n");
    fprintf(stderr, "Error to uv_mutex_init with %s: %s\n",
      uv_err_name(status), uv_strerror(status));
    abort();
  }

  vms->vms = (lua_State**)calloc(nvms, sizeof(lua_State*));
  vms->nvms = nvms;
  vms->idx_vms = 0;

  luaL_getmetatable(L, "luv_work_vms");
  lua_setmetatable(L, -2);

  // store the luv_work_vms_t in registry
  lua_rawsetp(L, LUA_REGISTRYINDEX, &luv_work_cleanup);

  uv_once(&once_vmkey, luv_key_init_once);
}
