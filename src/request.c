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
#include "request.h"

#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>
#include <uv.h>

#include "luv.h"
#include "private.h"

LUV_LIBAPI luv_req_t *luv_new_request(
  lua_State *const L,
  const uv_req_type type,
  const luv_ctx_t *const ctx,
  const int callback_idx
) {
  const size_t attached_size = uv_req_size(type);

  luv_req_t *const lreq = (luv_req_t *)luv_newuserdata(L, sizeof(luv_req_t) + attached_size);

  switch (type) {
#define XX(upper, lower)                \
  case UV_##upper:                      \
    luaL_getmetatable(L, "uv_" #lower); \
    break;
    UV_REQ_TYPE_MAP(XX)
#undef XX
    default:
      luv_error(L, "unknown request type");
  }

  // callback can either be nil or callable
  if (lua_isnoneornil(L, callback_idx)) {
    lreq->callback = LUA_NOREF;
  } else {
    luv_check_callable(L, callback_idx);

    lua_pushvalue(L, callback_idx);
    lreq->callback = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  lua_pushvalue(L, -1);
  lreq->ref = luaL_ref(L, LUA_REGISTRYINDEX);
  lreq->ctx = ctx;

  lreq->data = LUA_NOREF;
  lreq->data_list = NULL;

  return lreq;
}

LUV_LIBAPI void luv_fulfill_req(lua_State *const L, luv_req_t *const lreq, const int nargs) {
  if (lreq->callback == LUA_NOREF) {
    lua_pop(L, nargs);
  } else {
    // fetch the callback and insert it before any arguments
    lua_rawgeti(L, LUA_REGISTRYINDEX, lreq->callback);
    if (nargs != 0) {
      lua_insert(L, -1 - nargs);
    }

    lreq->ctx->cb_pcall(L, nargs, 0, 0);
  }
}

LUV_LIBAPI void luv_cleanup_req(lua_State *const L, luv_req_t *const lreq) {
  // remove the metatable from the request userdata to avoid garbage collection
  if (lua_rawgeti(L, LUA_REGISTRYINDEX, lreq->ref) != LUA_TNIL) {
    lua_pushnil(L);
    lua_setmetatable(L, -2);
  }
  lua_pop(L, 1);

  luaL_unref(L, LUA_REGISTRYINDEX, lreq->ref);
  luaL_unref(L, LUA_REGISTRYINDEX, lreq->callback);
  if (lreq->data == LUA_REFNIL) {
    for (unsigned int i = 0; lreq->data_list[i] != LUA_NOREF; i++) {
      luaL_unref(L, LUA_REGISTRYINDEX, lreq->data_list[i]);
    }

    free(lreq->data_list);
  } else {
    luaL_unref(L, LUA_REGISTRYINDEX, lreq->data);
  }

  free(lreq);
}

static void luv_prepare_bufs(lua_State *const L, luv_req_t *const lreq, luv_bufs_t *const bufs, const int index) {
  if (bufs->bufs_count > LUV_BUFS_PREPARED) {
    bufs->bufs = (uv_buf_t *)calloc(bufs->bufs_count, sizeof(uv_buf_t));
  } else {
    bufs->bufs = &bufs->buf_buffer[0];
  }

  if (lreq != NULL) {
    lreq->data_list = (int *)calloc(bufs->bufs_count + 1, sizeof(int));
    lreq->data_list[bufs->bufs_count] = LUA_NOREF;
    lreq->data = LUA_REFNIL;
  }

  for (size_t i = 0; i < bufs->bufs_count; ++i) {
    lua_rawgeti(L, index, (lua_Integer)i + 1);
    if (!lua_isstring(L, -1))
      luv_argerror(L, index, "expected string or table of strings, found %s in table", luaL_typename(L, -1));

    uv_buf_t *const buf = &bufs->bufs[i];
    buf->base = (char *)lua_tolstring(L, -1, &buf->len);

    if (lreq != NULL) {
      lua_pushvalue(L, -1);
      lreq->data_list[i] = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    lua_pop(L, 1);
  }
}

static void luv_prepare_buf(lua_State *const L, luv_req_t *const lreq, luv_bufs_t *const bufs, const int index) {
  bufs->bufs_count = 1;
  bufs->bufs = &bufs->buf_buffer[0];

  uv_buf_t *const buf = &bufs->buf_buffer[0];
  buf->base = (char *)lua_tolstring(L, index, &buf->len);

  if (lreq != NULL) {
    lua_pushvalue(L, index);
    lreq->data = luaL_ref(L, LUA_REGISTRYINDEX);
  }
}

LUV_LIBAPI void luv_request_bufs(lua_State *const L, luv_req_t *const lreq, luv_bufs_t *const bufs, const int index) {
  if (lua_istable(L, index)) {
    bufs->bufs_count = lua_rawlen(L, index);
    if (bufs->bufs_count == 0)
      luv_argerror(L, index, "expected string or table of strings, found empty table");

    if (bufs->bufs_count == 1) {
      lua_rawgeti(L, index, 1);
      if (!lua_isstring(L, -1))
        luv_argerror(L, index, "expected string or table of strings, found %s in table", luaL_typename(L, -1));

      luv_prepare_buf(L, lreq, bufs, -1);
      lua_pop(L, 1);
      return;
    }

    luv_prepare_bufs(L, lreq, bufs, index);
  } else if (lua_isstring(L, index)) {
    luv_prepare_buf(L, lreq, bufs, index);
  } else {
    luv_typeerror(L, index, "string or table of strings");
  }
}

LUV_LIBAPI void luv_request_bufs_free(luv_bufs_t *const bufs) {
  if (bufs->bufs_count > LUV_BUFS_PREPARED)
    free(bufs->bufs);
}