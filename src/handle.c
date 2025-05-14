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

#include "handle.h"

#include <lauxlib.h>
#include <lua.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "luv.h"
#include "private.h"

LUV_DEFAPI luaL_Reg luv_handle_methods[] = {
  {"close", luv_close},
  {"is_active", luv_is_active},
  {"is_closing", luv_is_closing},
  {"ref", luv_ref},
  {"unref", luv_unref},
  {"has_ref", luv_has_ref},
  {"send_buffer_size", luv_send_buffer_size},
  {"recv_buffer_size", luv_recv_buffer_size},
  {"fileno", luv_fileno},
  {"get_type", luv_handle_get_type},
  {NULL, NULL},
};

LUV_DEFAPI luaL_Reg luv_handle_functions[] = {
  {"close", luv_close},
  {"is_active", luv_is_active},
  {"is_closing", luv_is_closing},
  {"ref", luv_ref},
  {"unref", luv_unref},
  {"has_ref", luv_has_ref},
  {"send_buffer_size", luv_send_buffer_size},
  {"recv_buffer_size", luv_recv_buffer_size},
  {"fileno", luv_fileno},
  {"handle_get_type", luv_handle_get_type},
  {NULL, NULL},
};

/* provides us an address which we guarantee is unique to us to mark handle metatables */
static char handle_key;

LUV_LIBAPI int luv_handle_metatable(lua_State *L, const char *name, const luaL_Reg *methods) {
  luaL_newmetatable(L, name);

  lua_pushboolean(L, 1);
  lua_rawsetp(L, -2, &handle_key);

  lua_pushstring(L, name);
  lua_setfield(L, -2, "__name");

  lua_pushcfunction(L, luv_handle__tostring);
  lua_setfield(L, -2, "__tostring");

  lua_pushcfunction(L, luv_handle__gc);
  lua_setfield(L, -2, "__gc");

  luaL_newlib(L, luv_handle_methods);
  luaL_setfuncs(L, methods, 0);
  lua_setfield(L, -2, "__index");

  return 1;
}

LUV_LIBAPI luv_handle_t *luv_new_handle(
  lua_State *const L,
  const uv_handle_type type,
  const luv_ctx_t *const ctx,
  const size_t extra_sz
) {
  const size_t attached_size = uv_handle_size(type) + extra_sz;

  luv_handle_t *const lhandle = (luv_handle_t *)luv_newuserdata(L, sizeof(luv_handle_t) + attached_size);
  if (lhandle == NULL) {
    luaL_error(L, "out of memory");
    return NULL;
  }

  switch (type) {
#define XX(upper, lower)                \
  case UV_##upper:                      \
    luaL_getmetatable(L, "uv_" #lower); \
    break;
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default:
      luaL_error(L, "unknown handle type");
  }

  lua_pushvalue(L, -1);
  lhandle->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  lhandle->ctx = ctx;

  lhandle->callbacks[0] = LUA_NOREF;
  lhandle->callbacks[1] = LUA_NOREF;

  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  memset((void *)handle, 0, attached_size);
  handle->type = type;

  return handle;
}

LUV_LIBAPI void luv_handle_unref(lua_State *const L, luv_handle_t *const data) {
  if (L != data->ctx->L) {
    luaL_error(L, "luv_handle_push: invalid lua state");
    return;
  }

  luaL_unref(L, LUA_REGISTRYINDEX, data->ref);
  luaL_unref(L, LUA_REGISTRYINDEX, data->callbacks[0]);
  luaL_unref(L, LUA_REGISTRYINDEX, data->callbacks[1]);

  data->ref = LUA_NOREF;
  data->callbacks[0] = LUA_NOREF;
  data->callbacks[1] = LUA_NOREF;
}

LUV_LIBAPI void luv_handle_push(lua_State *const L, luv_handle_t *const data) {
  if (L != data->ctx->L) {
    luaL_error(L, "invalid lua state");
    return;
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, data->ref);
}

LUV_LIBAPI void luv_callback_prep(
  lua_State *const L,
  const enum luv_callback_id what,
  luv_handle_t *const data,
  int const index
) {
  if (L != data->ctx->L) {
    luaL_error(L, "invalid lua state");
    return;
  }

  luv_check_callable(L, index);
  luaL_unref(L, LUA_REGISTRYINDEX, data->callbacks[what]);
  lua_pushvalue(L, index);
  data->callbacks[what] = luaL_ref(L, LUA_REGISTRYINDEX);
}

LUV_LIBAPI void luv_callback_send(
  lua_State *const L,
  const enum luv_callback_id what,
  const luv_handle_t *const data,
  int const nargs
) {
  if (L != data->ctx->L) {
    luaL_error(L, "invalid lua state");
    return;
  }

  const int ref = data->callbacks[what];
  if (ref == LUA_NOREF) {
    lua_pop(L, nargs);
  } else {
    // fetch the callback and insert it before any arguments
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (nargs != 0) {
      lua_insert(L, -1 - nargs);
    }

    const luv_ctx_t *const ctx = data->ctx;
    ctx->cb_pcall(L, nargs, 0, 0);
  }
}

LUV_LIBAPI luv_handle_t *luv_check_handle(lua_State *const L, const int index) {
  void *userdata = lua_touserdata(L, index);
  if (userdata == NULL) {
    luaL_argerror(L, index, "expected uv_handle userdata");
    return NULL;
  }

  if (lua_getmetatable(L, index) == 0) {
    luaL_argerror(L, index, "expected uv_handle userdata");
    return NULL;
  }

  lua_rawgetp(L, -1, &handle_key);
  if (lua_toboolean(L, -1) == 0) {
    luaL_argerror(L, index, "expected uv_handle userdata");
    return NULL;
  }

  lua_pop(L, 2);
  return *(luv_handle_t **)userdata;
}

// Show the libuv type instead of generic "userdata"
LUV_LUAAPI int luv_handle__tostring(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  switch (handle->type) {
#define XX(upper, lower)                             \
  case UV_##upper:                                   \
    lua_pushfstring(L, "uv_" #lower ": %p", handle); \
    break;
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default:
      lua_pushfstring(L, "uv_handle: %p", handle);
      break;
  }
  return 1;
}

LUV_CBAPI void luv_gc_close_cb(uv_handle_t *const handle) {
  luv_handle_t *const lhandle = luv_handle_from(handle);

  // libuv is done with the handle, we can free it now.
  handle->type = UV_UNKNOWN_HANDLE;
  free(lhandle);
}

LUV_LUAAPI int luv_handle__gc(lua_State *L) {
  luv_handle_t **const udata = (luv_handle_t **)lua_touserdata(L, 1);
  luv_handle_t *const lhandle = *udata;
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  // release any references we may be holding
  luv_handle_unref(L, lhandle);

  if (uv_is_closing(handle) == 0) {
    // if the handle is not closed yet, we must close it first before freeing
    // memory.
    uv_close(handle, luv_gc_close_cb);
  } else {
    // otherwise, free the memory right away.
    handle->type = UV_UNKNOWN_HANDLE;
    free(lhandle);
  }

  *udata = NULL;
  return 0;
}

LUV_CBAPI void luv_close_cb(uv_handle_t *const handle) {
  luv_handle_t *const lhandle = luv_handle_from(handle);
  lua_State *L = lhandle->ctx->L;

  if (lhandle->ref != LUA_NOREF) {
    luv_callback_send(L, LUV_CB_CLOSE, lhandle, 0);
    luv_handle_unref(L, lhandle);
  } else {
    free(handle);
  }
}

LUV_LUAAPI int luv_close(lua_State *L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  if (uv_is_closing(handle) != 0) {
    luaL_error(L, "handle %p is already closing", handle);
  }

  if (!lua_isnoneornil(L, 2)) {
    luv_callback_prep(L, LUV_CB_CLOSE, lhandle, 2);
  }

  uv_close(handle, luv_close_cb);
  return 0;
}

LUV_LUAAPI int luv_is_active(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  const int ret = uv_is_active(handle);
  if (ret < 0) {
    return luv_pushfail(L, ret);
  }
  lua_pushboolean(L, ret);
  return 1;
}

LUV_LUAAPI int luv_is_closing(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  const int ret = uv_is_closing(handle);
  if (ret < 0) {
    return luv_pushfail(L, ret);
  }
  lua_pushboolean(L, ret);
  return 1;
}

LUV_LUAAPI int luv_ref(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  uv_ref(handle);
  return 0;
}

LUV_LUAAPI int luv_unref(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  uv_unref(handle);
  return 0;
}

LUV_LUAAPI int luv_has_ref(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  const int ret = uv_has_ref(handle);
  if (ret < 0) {
    return luv_pushfail(L, ret);
  }
  lua_pushboolean(L, ret);
  return 1;
}

LUV_LUAAPI int luv_send_buffer_size(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);
  int value = (int)luaL_optinteger(L, 2, 0);

  if (value != 0) {  // set buffer size
    const int ret = uv_send_buffer_size(handle, &value);
    return luv_pushresult(L, ret);
  }

  // get buffer size
  const int ret = uv_send_buffer_size(handle, &value);
  if (ret < 0) {
    return luv_pushfail(L, ret);
  }
  lua_pushinteger(L, value);
  return 1;
}

LUV_LUAAPI int luv_recv_buffer_size(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);
  int value = (int)luaL_optinteger(L, 2, 0);

  if (value != 0) {  // set buffer size
    const int ret = uv_recv_buffer_size(handle, &value);
    return luv_pushresult(L, ret);
  }

  // get buffer size
  const int ret = uv_recv_buffer_size(handle, &value);
  if (ret < 0) {
    return luv_pushfail(L, ret);
  }
  lua_pushinteger(L, value);
  return 1;
}

LUV_LUAAPI int luv_fileno(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  uv_os_fd_t fd;
  const int ret = uv_fileno(handle, &fd);
  if (ret < 0) {
    return luv_pushfail(L, ret);
  }
  lua_pushinteger(L, (lua_Integer)fd);
  return 1;
}

LUV_LUAAPI int luv_handle_get_type(lua_State *const L) {
  luv_handle_t *const lhandle = luv_check_handle(L, 1);
  uv_handle_t *const handle = luv_handle_of(uv_handle_t, lhandle);

  const uv_handle_type type = handle->type;
  switch (type) {
#define XX(upper, lower)             \
  case UV_##upper:                   \
    lua_pushstring(L, "uv_" #lower); \
    break;
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default:
      lua_pushstring(L, "uv_handle");
      break;
  }
  lua_pushinteger(L, type);
  return 2;
}
