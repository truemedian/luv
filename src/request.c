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
  if (lreq == NULL) {
    luaL_error(L, "out of memory");
    return NULL;
  }

  switch (type) {
#define XX(upper, lower)                \
  case UV_##upper:                      \
    luaL_getmetatable(L, "uv_" #lower); \
    break;
    UV_REQ_TYPE_MAP(XX)
#undef XX
    default:
      luaL_error(L, "unknown request type");
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

LUV_LIBAPI void luv_fulfill_req(lua_State *const L, luv_req_t *const data, const int nargs) {
  if (data->callback == LUA_NOREF) {
    lua_pop(L, nargs);
  } else {
    // fetch the callback and insert it before any arguments
    lua_rawgeti(L, LUA_REGISTRYINDEX, data->callback);
    if (nargs != 0) {
      lua_insert(L, -1 - nargs);
    }

    data->ctx->cb_pcall(L, nargs, 0, 0);
  }
}

LUV_LIBAPI void luv_cleanup_req(lua_State *const L, luv_req_t *const data) {
  luaL_unref(L, LUA_REGISTRYINDEX, data->ref);
  luaL_unref(L, LUA_REGISTRYINDEX, data->callback);
  if (data->data == LUA_REFNIL) {
    for (unsigned int i = 0; data->data_list[i] != LUA_NOREF; i++) {
      luaL_unref(L, LUA_REGISTRYINDEX, data->data_list[i]);
    }

    free(data->data_list);
  } else {
    luaL_unref(L, LUA_REGISTRYINDEX, data->data);
  }

  free(data);
}
