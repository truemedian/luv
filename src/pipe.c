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
#include "pipe.h"

#include <lauxlib.h>
#include <lua.h>
#include <stddef.h>
#include <uv.h>

#include "handle.h"
#include "internal.h"
#include "luv.h"
#include "request.h"

LUV_LIBAPI uv_pipe_t *luv_check_pipe(lua_State *const L, const int index) {
  const luv_handle_t *const lhandle = (const luv_handle_t *)luv_checkuserdata(L, index, "uv_pipe");
  uv_pipe_t *const pipe = luv_handle_of(uv_pipe_t, lhandle);

  luaL_argcheck(L, pipe->type == UV_NAMED_PIPE, index, "Expected uv_pipe");
  return pipe;
}

LUV_LUAAPI int luv_new_pipe(lua_State *const L) {
  const luv_ctx_t *const ctx = luv_context(L);

  luaL_argcheck(L, lua_isnoneornil(L, 1) || lua_isboolean(L, 1), 1, "expected boolean or nil");
  const int ipc = lua_toboolean(L, 1);

  luv_handle_t *const lhandle = luv_new_handle(L, UV_NAMED_PIPE, ctx, 0);
  uv_pipe_t *const pipe = luv_handle_of(uv_pipe_t, lhandle);

  const int ret = uv_pipe_init(ctx->loop, pipe, ipc);
  if (ret < 0) {
    luv_handle_unref(L, lhandle);
    return luv_pushfail(L, ret);
  }

  return 1;
}

LUV_LUAAPI int luv_pipe_open(lua_State *const L) {
  uv_pipe_t *const pipe = luv_check_pipe(L, 1);
  uv_file file = (uv_file)luaL_checkinteger(L, 2);
  int ret = uv_pipe_open(pipe, file);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

LUV_LUAAPI int luv_pipe_bind(lua_State *const L) {
  uv_pipe_t *const pipe = luv_check_pipe(L, 1);
  const char *const name = luaL_checkstring(L, 2);
  const int ret = uv_pipe_bind(pipe, name);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

/* reused with tcp for connect_cb */
LUV_LIBAPI void luv_connect_cb(uv_connect_t *const connect, const int status) {
  luv_req_t *lreq = luv_request_from(connect);
  lua_State *L = lreq->ctx->L;

  luv_pushstatus(L, status);
  luv_fulfill_req(L, lreq, 1);
  luv_cleanup_req(L, lreq);
}

LUV_LUAAPI int luv_pipe_connect(lua_State *const L) {
  luv_ctx_t *ctx = luv_context(L);
  uv_pipe_t *handle = luv_check_pipe(L, 1);
  const char *name = luaL_checkstring(L, 2);
  
  luv_req_t *lreq = luv_new_request(L, UV_CONNECT, ctx, 3);
  uv_connect_t *connect = luv_request_of(uv_connect_t, lreq);

  uv_pipe_connect(connect, handle, name, luv_connect_cb);
  return 1;
}

LUV_LUAAPI int luv_pipe_getsockname(lua_State *const L) {
  size_t len = 2 * (size_t)PATH_MAX;
  char buf[2 * PATH_MAX];

  uv_pipe_t *const pipe = luv_check_pipe(L, 1);
  const int ret = uv_pipe_getsockname(pipe, buf, &len);
  if (ret < 0)
    return luv_pushfail(L, ret);
  lua_pushlstring(L, buf, len);
  return 1;
}

LUV_LUAAPI int luv_pipe_pending_instances(lua_State *const L) {
  uv_pipe_t *const pipe = luv_check_pipe(L, 1);
  int count = (int)luaL_checkinteger(L, 2);
  uv_pipe_pending_instances(pipe, count);
  return 0;
}

LUV_LUAAPI int luv_pipe_pending_count(lua_State *const L) {
  uv_pipe_t *const pipe = luv_check_pipe(L, 1);
  lua_pushinteger(L, uv_pipe_pending_count(pipe));
  return 1;
}

LUV_LUAAPI int luv_pipe_pending_type(lua_State *const L) {
  uv_pipe_t *const pipe = luv_check_pipe(L, 1);
  switch (uv_pipe_pending_type(pipe)) {
#define XX(upper, lower)       \
  case UV_##upper:             \
    lua_pushstring(L, #lower); \
    return 1;

    UV_HANDLE_TYPE_MAP(XX)
#undef XX
    default:
      break;
  }

  return 0;
}

#if LUV_UV_VERSION_GEQ(1, 3, 0)
LUV_LUAAPI int luv_pipe_getpeername(lua_State *const L) {
  size_t len = 2 * (size_t)PATH_MAX;
  char buf[2 * PATH_MAX];

  uv_pipe_t *const pipe = luv_check_pipe(L, 1);
  const int ret = uv_pipe_getpeername(pipe, buf, &len);
  if (ret < 0)
    return luv_pushfail(L, ret);
  lua_pushlstring(L, buf, len);
  return 1;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 16, 0)
static const char *const luv_pipe_chmod_flags[] = {"r", "w", "rw", "wr", NULL};

LUV_LUAAPI int luv_pipe_chmod(lua_State *const L) {
  uv_pipe_t *pipe = luv_check_pipe(L, 1);

  int flags = 0;
  switch (luaL_checkoption(L, 2, NULL, luv_pipe_chmod_flags)) {
    case 0:
      flags = UV_READABLE;
      break;
    case 1:
      flags = UV_WRITABLE;
      break;
    case 2:
    case 3:
      flags = UV_READABLE | UV_WRITABLE;
      break;
    default:
      luv_error(L, "unreachable");
  }
  const int ret = uv_pipe_chmod(pipe, flags);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}
#endif

#if LUV_UV_VERSION_GEQ(1, 41, 0)
LUV_LUAAPI int luv_pipe(lua_State *const L) {
  int read_flags = 0;
  int write_flags = 0;

  if (luv_checkopttype(L, 1, LUA_TTABLE) != 0) {
    lua_getfield(L, 1, "nonblock");
    if (lua_toboolean(L, -1))
      read_flags |= UV_NONBLOCK_PIPE;
    lua_pop(L, 1);
  }
  if (luv_checkopttype(L, 2, LUA_TTABLE) != 0) {
    lua_getfield(L, 2, "nonblock");
    if (lua_toboolean(L, -1))
      write_flags |= UV_NONBLOCK_PIPE;
    lua_pop(L, 1);
  }

  uv_file fds[2];
  const int ret = uv_pipe(fds, read_flags, write_flags);
  if (ret < 0)
    return luv_pushfail(L, ret);

  // return as a table with 'read' and 'write' keys containing the corresponding fds
  lua_createtable(L, 0, 2);
  lua_pushinteger(L, fds[0]);
  lua_setfield(L, -2, "read");
  lua_pushinteger(L, fds[1]);
  lua_setfield(L, -2, "write");
  return 1;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 46, 0)
LUV_LIBAPI unsigned int luv_pipe_optflags(lua_State *const L, const int index, const unsigned int flags) {
  // flags param can be nil, an integer, or a table
  if (lua_type(L, index) == LUA_TNUMBER || lua_isnoneornil(L, index)) {
    return (unsigned int)luaL_optinteger(L, index, flags);
  }

  if (lua_type(L, index) == LUA_TTABLE) {
    unsigned int flags = 0;

    lua_getfield(L, index, "no_truncate");
    if (lua_toboolean(L, -1))
      flags |= UV_PIPE_NO_TRUNCATE;
    lua_pop(L, 1);
    return flags;
  }

  luv_argerror(L, index, "expected nil, integer, or table");
}

LUV_LUAAPI int luv_pipe_bind2(lua_State *const L) {
  size_t namelen;
  uv_pipe_t *pipe = luv_check_pipe(L, 1);
  const char *const name = luaL_checklstring(L, 2, &namelen);
  const unsigned int flags = luv_pipe_optflags(L, 3, 0);
  int ret = uv_pipe_bind2(pipe, name, namelen, flags);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

LUV_LUAAPI int luv_pipe_connect2(lua_State *const L) {
  size_t namelen;
  luv_ctx_t *ctx = luv_context(L);
  uv_pipe_t *pipe = luv_check_pipe(L, 1);
  const char *const name = luaL_checklstring(L, 2, &namelen);
  unsigned int flags = luv_pipe_optflags(L, 3, 0);

  luv_req_t *lreq = luv_new_request(L, UV_CONNECT, ctx, 4);
  uv_connect_t *connect = luv_request_of(uv_connect_t, lreq);
  const int ret = uv_pipe_connect2(connect, pipe, name, namelen, flags, luv_connect_cb);
  if (ret < 0) {
    luv_cleanup_req(L, lreq);
    return luv_pushfail(L, ret);
  }
  return 1;
}
#endif