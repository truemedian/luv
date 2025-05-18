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
#include "request.h"

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "internal.h"

static const char* luv_request_type_name(uv_req_type type) {
#define XX(upper, lower)  \
  if (type == UV_##upper) \
    return "uv_" #lower;
  UV_REQ_TYPE_MAP(XX)
#undef XX
  return "uv_req";
};

static luv_request_t* luv_request_create(lua_State* L, uv_req_type type, const luv_ctx_t* ctx, int callback) {
  const size_t attached_size = sizeof(luv_request_t) + uv_req_size(type);
  luv_request_t* const lrequest = (luv_request_t*)luv_newuserdata(L, attached_size);

  const char* const tname = luv_request_type_name(type);
  luv_assert(L, luaL_getmetatable(L, tname) == LUA_TTABLE);
  lua_setmetatable(L, -2);

  // callback can either be nil or callable
  if (lua_isnoneornil(L, callback)) {
    lrequest->callback = LUA_NOREF;
  } else {
    luv_checkcallable(L, callback);

    lua_pushvalue(L, callback);
    lrequest->callback = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  lua_pushvalue(L, -1);
  lrequest->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  lrequest->ctx = ctx;

  lrequest->data = LUA_NOREF;
  lrequest->data_list = NULL;

  return lrequest;
};

static void luv_request_unref(lua_State* L, luv_request_t* lrequest) {
  luaL_unref(L, LUA_REGISTRYINDEX, lrequest->ref);
  luaL_unref(L, LUA_REGISTRYINDEX, lrequest->callback);

  if (lrequest->data == LUA_REFNIL) {
    for (int i = 0; lrequest->data_list[i] != LUA_NOREF; i++) {
      luaL_unref(L, LUA_REGISTRYINDEX, lrequest->data_list[i]);
    }

    free(lrequest->data_list);
  } else {
    luaL_unref(L, LUA_REGISTRYINDEX, lrequest->data);
  }

  lrequest->ref = LUA_NOREF;
  lrequest->callback = LUA_NOREF;
  lrequest->data = LUA_NOREF;
  lrequest->data_list = NULL;
};

static void luv_request_finish(lua_State* L, luv_request_t* lrequest, int nargs) {
  if (lrequest->callback == LUA_NOREF) {
    lua_pop(L, nargs);
    return;
  }

  lua_rawgeti(L, LUA_REGISTRYINDEX, lrequest->callback);
  if (nargs > 0)  // place the callback before the arguments
    lua_insert(L, -(nargs + 1));

  const luv_ctx_t* ctx = lrequest->ctx;
  ctx->cb_pcall(L, nargs, 0, 0);
}

static void luv_request_buffers_many(
  lua_State* const L,
  luv_request_t* const lrequest,
  luv_bufs_t* const bufs,
  const int index
) {
  size_t len = lua_rawlen(L, index);
  if (len == 0)
    luv_argerror(L, index, "expected string or table of strings, found empty table");

  if (len == 1) {
    if (lua_rawgeti(L, index, 1) != LUA_TSTRING)
      luv_argerror(L, index, "expected string or table of strings, found %s in table", luaL_typename(L, -1));

    return luv_request_buffers_single(L, lrequest, bufs, -1);
  }

  luv_bufs_prepare(*bufs, len);

  // if we have an associated request, create a list to hold data references
  if (lrequest != NULL) {
    lrequest->data_list = (int*)calloc(len + 1, sizeof(int));
    lrequest->data_list[len] = LUA_NOREF;
    lrequest->data = LUA_REFNIL;
  }

  uv_buf_t* const ptr = luv_bufs_ptr(*bufs);
  for (size_t i = 0; i < len; ++i) {
    if (lua_rawgeti(L, index, (lua_Integer)i + 1) != LUA_TSTRING)
      luv_argerror(L, index, "expected string or table of strings, found %s in table", luaL_typename(L, -1));

    uv_buf_t* const buf = &ptr[i];
    buf->base = (char*)lua_tolstring(L, -1, &buf->len);

    // if we have an associated request, store the data reference
    if (lrequest != NULL) {
      lua_pushvalue(L, -1);
      lrequest->data_list[i] = luaL_ref(L, LUA_REGISTRYINDEX);
    }

    lua_pop(L, 1);
  }
}

static void luv_request_buffers_single(
  lua_State* const L,
  luv_request_t* const lrequest,
  luv_bufs_t* const bufs,
  const int index
) {
  luv_bufs_prepare(*bufs, 1);

  uv_buf_t* const buf = luv_bufs_ptr(*bufs);
  buf->base = (char*)lua_tolstring(L, index, &buf->len);

  // if we have an associated request, store the data reference
  if (lrequest != NULL) {
    lua_pushvalue(L, index);
    lrequest->data = luaL_ref(L, LUA_REGISTRYINDEX);
  }
}

static void luv_request_buffers(lua_State* L, luv_request_t* lrequest, luv_bufs_t* bufs, int index) {
  switch (lua_type(L, index)) {
    case LUA_TTABLE:
      luv_request_buffers_many(L, lrequest, bufs, index);
      break;
    case LUA_TSTRING:
      luv_request_buffers_single(L, lrequest, bufs, index);
      break;
    default:
      luv_typeerror(L, index, "string or table of strings");
  }
}

static int luv_request_tostring(lua_State* L) {
  luv_request_t* const lrequest = luaL_checkudata(L, 1, "uv_req");
  uv_req_t* const req = luv_request_of(uv_req_t, lrequest);

  lua_pushstring(L, luv_request_type_name(req->type));
  lua_pushfstring(L, ": %p", lrequest);

  // mark the request as done if necessary
  if (lrequest->ref == LUA_NOREF) {
    lua_pushfstring(L, " (done)");
    lua_concat(L, 3);
  } else {
    lua_concat(L, 2);
  }

  return 1;
};

static int luv_request_gc(lua_State* L) {
  luv_request_t* const lrequest = luaL_checkudata(L, 1, "uv_req");
  uv_req_t* const req = luv_request_of(uv_req_t, lrequest);

  // already finished, nothing to do
  if (lrequest->ref == LUA_NOREF)
    return 0;

  // if we get here, we're being garbage collected while holding a reference to the userdata which means that the lua
  // state is closing and we have no control over keeping the request alive, we depend on the main luv loop finalizer to
  // prevent this userdata from being collected while the request is still active

  // release any callback reference so we can cancel this request as soon as possible
  luaL_unref(L, LUA_REGISTRYINDEX, lrequest->callback);
  lrequest->callback = LUA_NOREF;

  // otherwise the request is still active and we should to try to cancel it
  (void)uv_cancel(req);

  return 0;
}

static int luv_request_cancel(lua_State* L) {
  luv_request_t* const lrequest = luaL_checkudata(L, 1, "uv_req");
  uv_req_t* const req = luv_request_of(uv_req_t, lrequest);

  int ret = uv_cancel(req);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

static int luv_request_get_type(lua_State* L) {
  luv_request_t* const lrequest = luaL_checkudata(L, 1, "uv_req");
  uv_req_t* const req = luv_request_of(uv_req_t, lrequest);

  const uv_req_type type = req->type;

  /* our version of request_type_name starts with uv_, so remove that by jumping forward 3 characters */
  lua_pushstring(L, luv_request_type_name(type) + 3);
  lua_pushinteger(L, type);
  return 2;
}
