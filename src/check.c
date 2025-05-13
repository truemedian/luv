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
#include "check.h"

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "handle.h"
#include "luv.h"
#include "private.h"

LUV_DEFAPI luaL_Reg luv_check_methods[] = {
  {"start", luv_check_start},
  {"stop", luv_check_stop},
  {NULL, NULL},
};

LUV_DEFAPI luaL_Reg luv_check_functions[] = {
  {"new_check", luv_new_check},
  {"check_start", luv_check_start},
  {"check_stop", luv_check_stop},
  {NULL, NULL},
};

LUV_LIBAPI uv_check_t *luv_check_check(lua_State *const L, const int index) {
  const luv_handle_t *const lhandle = (const luv_handle_t *)luv_checkudata(L, index, "uv_check");
  uv_check_t *const check = luv_handle_of(uv_check_t, lhandle);

  luaL_argcheck(L, check->type == UV_CHECK, index, "expected uv_check handle");
  return check;
}

LUV_LUAAPI int luv_new_check(lua_State *const L) {
  const luv_ctx_t *const ctx = luv_context(L);

  luv_handle_t *const lhandle = luv_new_handle(L, UV_CHECK, ctx, 0);
  uv_check_t *const check = luv_handle_of(uv_check_t, lhandle);

  const int ret = uv_check_init(ctx->loop, check);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }

  return 1;
}

LUV_CBAPI void luv_check_cb(uv_check_t *const check) {
  luv_handle_t *const lhandle = luv_handle_from(check);
  lua_State *const L = lhandle->ctx->L;

  luv_callback_send(L, LUV_CB_EVENT, lhandle, 0);
}

LUV_LUAAPI int luv_check_start(lua_State *const L) {
  uv_check_t *const check = luv_check_check(L, 1);
  luv_handle_t *const lhandle = luv_handle_from(check);

  luv_callback_prep(LUV_CB_EVENT, lhandle, 2);

  const int ret = uv_check_start(check, luv_check_cb);
  return luv_result(L, ret);
}

LUV_LUAAPI int luv_check_stop(lua_State *const L) {
  uv_check_t *const check = luv_check_check(L, 1);
  const int ret = uv_check_stop(check);
  return luv_result(L, ret);
}
