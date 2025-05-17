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
 */
#include "handle.h"

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "internal.h"
#include "luv.h"

// provides us an address which we guarantee is unique to us to mark handle metatables
static char stream_key;

// libuv only started providing this in 1.19.0
static const char* luv_handle_type_name(uv_handle_type type) {
#define XX(upper, lower)  \
  if (type == UV_##upper) \
    return "uv_" #lower;
  UV_HANDLE_TYPE_MAP(XX)
#undef XX
  return "uv_handle";
}

static int luv_handle_metatable(lua_State* L, const char* name, const luaL_Reg* methods) {
  luaL_newmetatable(L, name);

  lua_pushboolean(L, 1);
  lua_rawsetp(L, -2, &stream_key);

  lua_pushstring(L, name);
  lua_setfield(L, -2, "__name");

  lua_pushcfunction(L, luv_handle_tostring);
  lua_setfield(L, -2, "__tostring");

  lua_pushcfunction(L, luv_handle_gc);
  lua_setfield(L, -2, "__gc");

  luaL_newlib(L, luv_handle_methods);
  luaL_setfuncs(L, methods, 0);
  lua_setfield(L, -2, "__index");

  return 1;
}

static luv_handle_t* luv_handle_create(lua_State* L, uv_handle_type type, const luv_ctx_t* ctx, size_t extra_sz) {
  const size_t attached_size = sizeof(luv_handle_t) + uv_handle_size(type) + extra_sz;
  luv_handle_t* const lhandle = (luv_handle_t*)luv_newuserdata(L, attached_size);

  const char* const tname = luv_handle_type_name(type);
  luv_assert(L, luaL_getmetatable(L, tname) == LUA_TTABLE);
  lua_setmetatable(L, -2);

  lua_pushvalue(L, -1);
  lhandle->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  lhandle->ctx = ctx;

  lhandle->callback[LUV_CB_CLOSE] = LUA_NOREF;
  lhandle->callback[LUV_CB_EVENT] = LUA_NOREF;

  return lhandle;
}

static void luv_handle_unref(lua_State* L, luv_handle_t* lhandle) {
  luaL_unref(L, LUA_REGISTRYINDEX, lhandle->ref);
  luaL_unref(L, LUA_REGISTRYINDEX, lhandle->callback[LUV_CB_CLOSE]);
  luaL_unref(L, LUA_REGISTRYINDEX, lhandle->callback[LUV_CB_EVENT]);

  lhandle->ref = LUA_NOREF;
  lhandle->callback[LUV_CB_CLOSE] = LUA_NOREF;
  lhandle->callback[LUV_CB_EVENT] = LUA_NOREF;
}

static void luv_handle_push(lua_State* L, luv_handle_t* lhandle) {
  lua_rawgeti(L, LUA_REGISTRYINDEX, lhandle->ref);
}

static void luv_handle_check_valid(lua_State* L, luv_handle_t* lhandle) {
  if (lhandle->ref == LUA_NOREF)
    luv_error(L, "handle %p is closed", lhandle);
}

static void luv_handle_set_callback(lua_State* L, enum luv_callback_id what, luv_handle_t* lhandle, int index) {
  luv_checkcallable(L, index);

  // unregister any previous callback
  luaL_unref(L, LUA_REGISTRYINDEX, lhandle->callback[what]);

  // reference and store the new callback
  lua_pushvalue(L, index);
  lhandle->callback[what] = luaL_ref(L, LUA_REGISTRYINDEX);
}

static void luv_handle_unset_callback(lua_State* L, enum luv_callback_id what, luv_handle_t* lhandle) {
  luaL_unref(L, LUA_REGISTRYINDEX, lhandle->callback[what]);
  lhandle->callback[what] = LUA_NOREF;
}

static void luv_handle_send_callback(lua_State* L, enum luv_callback_id what, luv_handle_t* lhandle, int nargs) {
  if (lhandle->callback[what] == LUA_NOREF) {
    lua_pop(L, nargs);
    return;
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, lhandle->callback[what]);
  if (nargs > 0)  // place the callback before the arguments
    lua_insert(L, -(nargs + 1));

  const luv_ctx_t* ctx = lhandle->ctx;
  ctx->cb_pcall(L, nargs, 0, 0);
}

static luv_handle_t* luv_check_handle(lua_State* L, int index) {
  void* const userdata = lua_touserdata(L, index);
  if (userdata == NULL)
    luv_typeerror(L, index, "uv_handle");

  if (lua_getmetatable(L, index) == 0)
    luv_typeerror(L, index, "uv_handle");

  lua_rawgetp(L, -1, &stream_key);
  if (lua_toboolean(L, -1) == 0)
    luv_typeerror(L, index, "uv_handle");

  lua_pop(L, 2);
  return (luv_handle_t*)userdata;
}

// Show the libuv type instead of generic "userdata"
static int luv_handle_tostring(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  // luv_handle_check_valid(L, lhandle); not necessary here because we never actually use the handle here

  lua_pushstring(L, luv_handle_type_name(handle->type));
  lua_pushfstring(L, ": %p", lhandle);

  // mark the handle as closed if necessary
  if (lhandle->ref == LUA_NOREF) {
    lua_pushfstring(L, " (closed)");
    lua_concat(L, 3);
  } else {
    lua_concat(L, 2);
  }

  return 1;
}

// only called when the handle is not yet closed during garbage collection
static void luv_handle_gc_cb(uv_handle_t* handle) {
  luv_handle_t* lhandle = luv_handle_from(handle);
  lua_State* L = lhandle->ctx->L;

  // libuv is now done with the handle, so we can allow lua to collect it.
  luv_handle_unref(L, lhandle);
}

static int luv_handle_gc(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);

  // already closed, nothing to do
  if (lhandle->ref == LUA_NOREF)
    return 0;

  // if we get here, we're being garbage collected while holding a reference to the userdata which means that the lua
  // state is closing and we have no control over keeping the handle alive, we depend on the main luv loop finalizer to
  // prevent this userdata from being collected while the handle is still closing

  // we may need to release the callback references
  luv_handle_unref(L, lhandle);

  // if the handle is not yet closed, we must close it before allowing lua to collect the userdata we do this regardless
  // of whether the handle is already closing or not to ensure we override any previous close callbacks so we can
  // immediately release the userdata
  uv_close(handle, luv_handle_gc_cb);

  return 0;
}

static void luv_close_cb(uv_handle_t* handle) {
  luv_handle_t* lhandle = luv_handle_from(handle);
  lua_State* L = lhandle->ctx->L;

  luv_handle_send_callback(L, LUV_CB_CLOSE, lhandle, 0);

  // unreference the handle because we cannot use it anymore
  luv_handle_unref(L, lhandle);
}

static int luv_close(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  luv_handle_check_valid(L, lhandle);

  if (uv_is_closing(handle) != 0)
    luv_error(L, "handle %p is already closing", lhandle);

  if (!lua_isnoneornil(L, 2))
    luv_handle_set_callback(L, LUV_CB_CLOSE, lhandle, 2);

  uv_close(handle, luv_close_cb);
  return 0;
}

static int luv_is_active(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  luv_handle_check_valid(L, lhandle);

  const int ret = uv_is_active(handle);
  return luv_pushresult(L, ret, LUA_TBOOLEAN);
}

static int luv_is_closing(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  luv_handle_check_valid(L, lhandle);

  const int ret = uv_is_closing(handle);
  return luv_pushresult(L, ret, LUA_TBOOLEAN);
}

static int luv_ref(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  luv_handle_check_valid(L, lhandle);

  uv_ref(handle);
  return 0;
}

static int luv_unref(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  luv_handle_check_valid(L, lhandle);

  uv_unref(handle);
  return 0;
}

static int luv_has_ref(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  luv_handle_check_valid(L, lhandle);

  const int ret = uv_has_ref(handle);
  return luv_pushresult(L, ret, LUA_TBOOLEAN);
}

static int luv_send_buffer_size(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  luv_handle_check_valid(L, lhandle);
  int value = (int)luaL_optinteger(L, 2, 0);

  // set
  if (value != 0) {
    const int ret = uv_send_buffer_size(handle, &value);
    return luv_pushresult(L, ret, LUA_TNUMBER);
  }

  // get
  const int ret = uv_send_buffer_size(handle, &value);
  if (ret < 0)
    return luv_pushfail(L, ret);
  lua_pushinteger(L, value);
  return 1;
}

static int luv_recv_buffer_size(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  luv_handle_check_valid(L, lhandle);
  int value = (int)luaL_optinteger(L, 2, 0);

  // set
  if (value != 0) {
    const int ret = uv_recv_buffer_size(handle, &value);
    return luv_pushresult(L, ret, LUA_TNUMBER);
  }

  // get
  const int ret = uv_recv_buffer_size(handle, &value);
  if (ret < 0)
    return luv_pushfail(L, ret);
  lua_pushinteger(L, value);
  return 1;
}

static int luv_fileno(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);
  luv_handle_check_valid(L, lhandle);

  uv_os_fd_t fd;
  const int ret = uv_fileno(handle, &fd);
  if (ret < 0)
    return luv_pushfail(L, ret);
  lua_pushinteger(L, (lua_Integer)fd);
  return 1;
}

static int luv_handle_get_type(lua_State* L) {
  luv_handle_t* const lhandle = luv_check_handle(L, 1);
  uv_handle_t* const handle = luv_handle_of(uv_handle_t, lhandle);

  const uv_handle_type type = handle->type;

  /* our version of handle_type_name starts with uv_, so remove that by jumping forward 3 characters */
  lua_pushstring(L, luv_handle_type_name(type) + 3);
  lua_pushinteger(L, type);
  return 2;
}
