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
#include "stream.h"

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "handle.h"
#include "internal.h"
#include "luv.h"
#include "request.h"

// provides us an address which we guarantee is unique to us to mark stream metatables
static char stream_key;

static int luv_stream_metatable(lua_State* L, const char* name, const luaL_Reg* methods) {
  luv_handle_metatable(L, name, methods);

  lua_pushboolean(L, 1);
  lua_rawsetp(L, -2, &stream_key);

  lua_getfield(L, -1, "__index");
  luaL_setfuncs(L, luv_stream_methods, 0);
  lua_pop(L, 1);

  return 1;
}

static luv_handle_t* luv_check_stream(lua_State* L, int index) {
  void* const userdata = lua_touserdata(L, index);
  if (userdata == NULL)
    luv_typeerror(L, index, "uv_stream");

  if (lua_getmetatable(L, index) == 0)
    luv_typeerror(L, index, "uv_stream");

  lua_rawgetp(L, -1, &stream_key);
  if (lua_toboolean(L, -1) == 0)
    luv_typeerror(L, index, "uv_stream");

  lua_pop(L, 2);
  return (luv_handle_t*)userdata;
}

static int luv_accept(lua_State* L) {
  luv_handle_t* lhandle_server = luv_check_stream(L, 1);
  luv_handle_t* lhandle_client = luv_check_stream(L, 2);

  uv_stream_t* server = luv_handle_of(uv_stream_t, lhandle_server);
  uv_stream_t* client = luv_handle_of(uv_stream_t, lhandle_client);

  int ret = uv_accept(server, client);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

static void luv_shutdown_cb(uv_shutdown_t* shutdown, int status) {
  luv_request_t* lrequest = luv_request_from(shutdown);
  lua_State* L = lrequest->ctx->L;

  luv_pushstatus(L, status);
  luv_request_finish(L, lrequest, 1);
  luv_request_unref(L, lrequest);
}

static int luv_shutdown(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);

  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);

  luv_request_t* lrequest = luv_request_create(L, UV_SHUTDOWN, ctx, 2);
  uv_shutdown_t* shutdown = luv_request_of(uv_shutdown_t, lrequest);

  int ret = uv_shutdown(shutdown, stream, luv_shutdown_cb);
  if (ret < 0) {
    luv_request_unref(L, lrequest);
    return luv_pushfail(L, ret);
  }
  return 1;
}

static void luv_connection_cb(uv_stream_t* stream, int status) {
  luv_handle_t* lhandle = luv_handle_from(stream);
  lua_State* L = lhandle->ctx->L;

  luv_pushstatus(L, status);
  luv_handle_send_callback(L, LUV_CB_EVENT, lhandle, 1);
}

static int luv_listen(lua_State* L) {
  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);
  int backlog = (int)luaL_checkinteger(L, 2);

  luv_handle_set_callback(L, LUV_CB_EVENT, lhandle, 3);
  int ret = uv_listen(stream, backlog, luv_connection_cb);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

static void luv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  (void)handle;
  buf->base = (char*)malloc(suggested_size);
  buf->len = suggested_size;
}

static void luv_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
  luv_handle_t* lhandle = luv_handle_from(stream);
  lua_State* L = lhandle->ctx->L;

  // EAGAIN or EWOULDBLOCK, no reason to notify the callback
  if (nread == 0) {
    free(buf->base);
    return;
  }

  int nargs = 0;
  if (nread > 0) {
    lua_pushnil(L);
    lua_pushlstring(L, buf->base, nread);
    nargs = 2;
  } else if (nread == UV_EOF) {
    nargs = 0;
  } else if (nread < 0) {
    luv_pushstatus(L, (int)nread);
    lua_pushnil(L);
    nargs = 2;
  }

  free(buf->base);
  luv_handle_send_callback(L, LUV_CB_EVENT, lhandle, nargs);
}

static int luv_read_start(lua_State* L) {
  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);

  luv_handle_set_callback(L, LUV_CB_EVENT, lhandle, 2);
  int ret = uv_read_start(stream, luv_alloc_cb, luv_read_cb);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

static int luv_read_stop(lua_State* L) {
  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);
  int ret = uv_read_stop(stream);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

static void luv_write_cb(uv_write_t* write, int status) {
  luv_request_t* lrequest = luv_request_from(write);
  lua_State* L = lrequest->ctx->L;

  luv_pushstatus(L, status);
  luv_request_finish(L, lrequest, 1);
  luv_request_unref(L, lrequest);
}

static int luv_write(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);

  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);

  luv_bufs_t bufs;
  luv_request_buffers(L, NULL, &bufs, 2);

  luv_request_t* lrequest = luv_request_create(L, UV_WRITE, ctx, 3);
  uv_write_t* write = luv_request_of(uv_write_t, lrequest);

  int ret = uv_write(write, stream, luv_bufs_ptr(bufs), bufs.bufs_count, luv_write_cb);
  luv_bufs_free(bufs);
  if (ret < 0) {
    luv_request_unref(L, lrequest);
    return luv_pushfail(L, ret);
  }
  return 1;
}

static int luv_write2(lua_State* L) {
  luv_ctx_t* ctx = luv_context(L);

  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);

  luv_bufs_t bufs;
  luv_request_buffers(L, NULL, &bufs, 2);

  luv_handle_t* send_lhandle = luv_check_stream(L, 3);
  uv_stream_t* send_stream = luv_handle_of(uv_stream_t, send_lhandle);

  luv_request_t* lrequest = luv_request_create(L, UV_WRITE, ctx, 4);
  uv_write_t* write = luv_request_of(uv_write_t, lrequest);

  int ret = uv_write2(write, stream, luv_bufs_ptr(bufs), bufs.bufs_count, send_stream, luv_write_cb);
  luv_bufs_free(bufs);
  if (ret < 0) {
    luv_request_unref(L, lrequest);
    return luv_pushfail(L, ret);
  }
  return 1;
}

static int luv_try_write(lua_State* L) {
  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);

  luv_bufs_t bufs;
  luv_request_buffers(L, NULL, &bufs, 2);

  int nwritten = uv_try_write(stream, luv_bufs_ptr(bufs), bufs.bufs_count);
  luv_bufs_free(bufs);
  return luv_pushresult(L, nwritten, LUA_TNUMBER);
}

#if LUV_UV_VERSION_GEQ(1, 42, 0)
static int luv_try_write2(lua_State* L) {
  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);

  luv_handle_t* send_lhandle = luv_check_stream(L, 3);
  uv_stream_t* send_stream = luv_handle_of(uv_stream_t, send_lhandle);

  luv_bufs_t bufs;
  luv_request_buffers(L, NULL, &bufs, 2);

  int nwritten = uv_try_write2(stream, luv_bufs_ptr(bufs), bufs.bufs_count, send_stream);
  luv_bufs_free(bufs);
  return luv_pushresult(L, nwritten, LUA_TNUMBER);
}
#endif

static int luv_is_readable(lua_State* L) {
  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);
  lua_pushboolean(L, uv_is_readable(stream));
  return 1;
}

static int luv_is_writable(lua_State* L) {
  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);
  lua_pushboolean(L, uv_is_writable(stream));
  return 1;
}

static int luv_stream_set_blocking(lua_State* L) {
  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);
  luaL_checktype(L, 2, LUA_TBOOLEAN);
  int blocking = lua_toboolean(L, 2);
  int ret = uv_stream_set_blocking(stream, blocking);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

#if LUV_UV_VERSION_GEQ(1, 19, 0)
static int luv_stream_get_write_queue_size(lua_State* L) {
  luv_handle_t* lhandle = luv_check_stream(L, 1);
  uv_stream_t* stream = luv_handle_of(uv_stream_t, lhandle);
  lua_pushinteger(L, (lua_Integer)uv_stream_get_write_queue_size(stream));
  return 1;
}
#endif
