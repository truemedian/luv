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
#include "stream.h"

#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>
#include <uv.h>

#include "handle.h"
#include "luv.h"
#include "private.h"
#include "request.h"
#include "util.h"

LUV_DEFAPI luaL_Reg luv_stream_methods[] = {
  {"shutdown", luv_shutdown},
  {"listen", luv_listen},
  {"accept", luv_accept},
  {"read_start", luv_read_start},
  {"read_stop", luv_read_stop},
  {"write", luv_write},
  {"write2", luv_write2},
  {"try_write", luv_try_write},
  {"is_readable", luv_is_readable},
  {"is_writable", luv_is_writable},
  {"set_blocking", luv_stream_set_blocking},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"get_write_queue_size", luv_stream_get_write_queue_size},
#endif
#if LUV_UV_VERSION_GEQ(1, 42, 0)
  {"try_write2", luv_try_write2},
#endif
  {NULL, NULL},
};

LUV_DEFAPI luaL_Reg luv_stream_functions[] = {
  {"shutdown", luv_shutdown},
  {"listen", luv_listen},
  {"accept", luv_accept},
  {"read_start", luv_read_start},
  {"read_stop", luv_read_stop},
  {"write", luv_write},
  {"write2", luv_write2},
  {"try_write", luv_try_write},
  {"is_readable", luv_is_readable},
  {"is_writable", luv_is_writable},
  {"stream_set_blocking", luv_stream_set_blocking},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"stream_get_write_queue_size", luv_stream_get_write_queue_size},
#endif
#if LUV_UV_VERSION_GEQ(1, 42, 0)
  {"try_write2", luv_try_write2},
#endif
  {NULL, NULL},
};

/* provides us an address which we guarantee is unique to us to mark stream metatables */
static char stream_key;

LUV_LIBAPI int luv_stream_metatable(lua_State *L, const char *name, const luaL_Reg *methods) {
  luv_handle_metatable(L, name, methods);

  lua_pushboolean(L, 1);
  lua_rawsetp(L, -2, &stream_key);

  lua_getfield(L, -1, "__index");
  luaL_setfuncs(L, luv_stream_methods, 0);
  lua_pop(L, 1);

  return 1;
}

LUV_LIBAPI uv_stream_t *luv_check_stream(lua_State *const L, const int index) {
  void *userdata = lua_touserdata(L, index);
  if (userdata == NULL) {
    luaL_argerror(L, index, "expected uv_stream userdata");
    return NULL;
  }

  if (lua_getmetatable(L, index) == 0) {
    luaL_argerror(L, index, "expected uv_stream userdata");
    return NULL;
  }

  lua_rawgetp(L, -1, &stream_key);
  if (lua_toboolean(L, -1) == 0) {
    luaL_argerror(L, index, "expected uv_stream userdata");
    return NULL;
  }

  lua_pop(L, 2);
  luv_handle_t *lhandle = *(luv_handle_t **)userdata;
  return luv_handle_of(uv_stream_t, lhandle);
}

LUV_CBAPI void luv_shutdown_cb(uv_shutdown_t *const req, const int status) {
  luv_req_t *lreq = luv_request_from(req);
  lua_State *L = lreq->ctx->L;

  luv_status(L, status);
  luv_fulfill_req(L, lreq, 1);
  luv_cleanup_req(L, lreq);
}

LUV_LUAAPI int luv_shutdown(lua_State *const L) {
  luv_ctx_t *ctx = luv_context(L);
  uv_stream_t *stream = luv_check_stream(L, 1);

  luv_req_t *lreq = luv_new_request(L, UV_SHUTDOWN, ctx, 2);
  uv_shutdown_t *shutdown = luv_request_of(uv_shutdown_t, lreq);

  const int ret = uv_shutdown(shutdown, stream, luv_shutdown_cb);
  if (ret < 0) {
    luv_cleanup_req(L, lreq);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }

  return 1;
}

LUV_CBAPI void luv_connection_cb(uv_stream_t *const stream, const int status) {
  luv_handle_t *lhandle = luv_handle_from(stream);
  lua_State *L = lhandle->ctx->L;

  luv_status(L, status);
  luv_callback_send(L, LUV_CB_EVENT, lhandle, 1);
}

LUV_LUAAPI int luv_listen(lua_State *const L) {
  uv_stream_t *stream = luv_check_stream(L, 1);
  luv_handle_t *lhandle = luv_handle_from(stream);
  const int backlog = (int)luaL_checkinteger(L, 2);

  luv_callback_prep(L, LUV_CB_EVENT, lhandle, 3);
  const int ret = uv_listen(stream, backlog, luv_connection_cb);
  return luv_result(L, ret);
}

LUV_LUAAPI int luv_accept(lua_State *const L) {
  uv_stream_t *server = luv_check_stream(L, 1);
  uv_stream_t *client = luv_check_stream(L, 2);
  int ret = uv_accept(server, client);
  return luv_result(L, ret);
}

LUV_CBAPI void luv_alloc_cb(uv_handle_t *const handle, const size_t suggested_size, uv_buf_t *const buf) {
  (void)handle;
  buf->base = (char *)malloc(suggested_size);
  buf->len = suggested_size;
}

LUV_CBAPI void luv_read_cb(uv_stream_t *const stream, const ssize_t nread, const uv_buf_t *const buf) {
  luv_handle_t *lhandle = luv_handle_from(stream);
  lua_State *L = lhandle->ctx->L;

  if (nread > 0) {
    lua_pushnil(L);
    lua_pushlstring(L, buf->base, nread);
  }

  free(buf->base);
  if (nread == 0) {
    return;
  }

  if (nread == UV_EOF) {
    lua_pushnil(L);
    lua_pushnil(L);
  } else if (nread < 0) {
    luv_status(L, (int)nread);
    lua_pushnil(L);
  }

  luv_callback_send(L, LUV_CB_EVENT, lhandle, 2);
}

LUV_LUAAPI int luv_read_start(lua_State *const L) {
  uv_stream_t *stream = luv_check_stream(L, 1);
  luv_handle_t *lhandle = luv_handle_from(stream);

  luv_callback_prep(L, LUV_CB_EVENT, lhandle, 2);
  const int ret = uv_read_start(stream, luv_alloc_cb, luv_read_cb);
  return luv_result(L, ret);
}

LUV_LUAAPI int luv_read_stop(lua_State *const L) {
  uv_stream_t *stream = luv_check_stream(L, 1);
  const int ret = uv_read_stop(stream);
  return luv_result(L, ret);
}

LUV_CBAPI void luv_write_cb(uv_write_t *const req, const int status) {
  luv_req_t *lreq = luv_request_from(req);
  lua_State *L = lreq->ctx->L;

  luv_status(L, status);
  luv_fulfill_req(L, lreq, 1);
  luv_cleanup_req(L, lreq);
}

LUV_LUAAPI int luv_write(lua_State *const L) {
  luv_ctx_t *ctx = luv_context(L);
  uv_stream_t *const stream = luv_check_stream(L, 1);

  luv_req_t *lreq = luv_new_request(L, UV_WRITE, ctx, 3);
  uv_write_t *const write = luv_request_of(uv_write_t, lreq);

  luv_bufs_t bufs;
  luv_request_bufs(L, lreq, &bufs, 2);

  const int ret = uv_write(write, stream, bufs.bufs, bufs.bufs_count, luv_write_cb);
  luv_request_bufs_free(&bufs);
  if (ret < 0) {
    luv_cleanup_req(L, lreq);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }

  return 1;
}

LUV_LUAAPI int luv_write2(lua_State *const L) {
  luv_ctx_t *ctx = luv_context(L);
  uv_stream_t *stream = luv_check_stream(L, 1);

  uv_stream_t *const send_handle = luv_check_stream(L, 3);

  luv_req_t *lreq = luv_new_request(L, UV_WRITE, ctx, 4);
  uv_write_t *const write = luv_request_of(uv_write_t, lreq);

  luv_bufs_t bufs;
  luv_request_bufs(L, lreq, &bufs, 2);

  const int ret = uv_write2(write, stream, bufs.bufs, bufs.bufs_count, send_handle, luv_write_cb);
  luv_request_bufs_free(&bufs);
  if (ret < 0) {
    luv_cleanup_req(L, lreq);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }

  return 1;
}

LUV_LUAAPI int luv_try_write(lua_State *const L) {
  uv_stream_t *stream = luv_check_stream(L, 1);

  luv_bufs_t bufs;
  luv_request_bufs(L, NULL, &bufs, 2);

  const int nwrite = uv_try_write(stream, bufs.bufs, bufs.bufs_count);
  luv_request_bufs_free(&bufs);

  if (nwrite < 0) {
    return luv_error(L, nwrite);
  }
  lua_pushinteger(L, nwrite);
  return 1;
}

LUV_LUAAPI int luv_is_readable(lua_State *const L) {
  uv_stream_t *const stream = luv_check_stream(L, 1);
  lua_pushboolean(L, uv_is_readable(stream));
  return 1;
}

LUV_LUAAPI int luv_is_writable(lua_State *const L) {
  uv_stream_t *const stream = luv_check_stream(L, 1);
  lua_pushboolean(L, uv_is_writable(stream));
  return 1;
}

LUV_LUAAPI int luv_stream_set_blocking(lua_State *const L) {
  uv_stream_t *const stream = luv_check_stream(L, 1);

  luaL_checktype(L, 2, LUA_TBOOLEAN);
  const int blocking = lua_toboolean(L, 2);

  const int ret = uv_stream_set_blocking(stream, blocking);
  return luv_result(L, ret);
}

#if LUV_UV_VERSION_GEQ(1, 19, 0)
LUV_LUAAPI int luv_stream_get_write_queue_size(lua_State *const L) {
  uv_stream_t *const stream = luv_check_stream(L, 1);
  lua_pushinteger(L, (lua_Integer)uv_stream_get_write_queue_size(stream));
  return 1;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 42, 0)
LUV_LUAAPI int luv_try_write2(lua_State *const L) {
  uv_stream_t *const stream = luv_check_stream(L, 1);

  luv_bufs_t bufs;
  luv_request_bufs(L, NULL, &bufs, 2);

  uv_stream_t *const send_handle = luv_check_stream(L, 3);

  const int nwrite = uv_try_write2(stream, bufs.bufs, bufs.bufs_count, send_handle);
  luv_request_bufs_free(&bufs);

  if (nwrite < 0) {
    return luv_error(L, nwrite);
  }
  lua_pushinteger(L, nwrite);
  return 1;
}
#endif
