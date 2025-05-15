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
#include "idle.h"

#include <lauxlib.h>
#include <lua.h>
#include <stddef.h>
#include <uv.h>

#include "handle.h"
#include "internal.h"
#include "luv.h"

LUV_DEFAPI luaL_Reg luv_idle_methods[] = {
  {"start", luv_idle_start},
  {"stop", luv_idle_stop},
  {NULL, NULL},
};

LUV_DEFAPI luaL_Reg luv_idle_functions[] = {
  {"new_idle", luv_new_idle},
  {"idle_start", luv_idle_start},
  {"idle_stop", luv_idle_stop},
  {NULL, NULL},
};

LUV_LIBAPI uv_idle_t *luv_idle_check(lua_State *const L, const int index) {
  const luv_handle_t *const lhandle = (const luv_handle_t *)luv_checkuserdata(L, index, "uv_idle");
  uv_idle_t *const idle = luv_handle_of(uv_idle_t, lhandle);

  luaL_argcheck(L, idle->type == UV_IDLE, index, "expected uv_idle handle");
  return idle;
}

LUV_LUAAPI int luv_new_idle(lua_State *const L) {
  luv_ctx_t *const ctx = luv_context(L);

  luv_handle_t *const lhandle = luv_new_handle(L, UV_IDLE, ctx, 0);
  uv_idle_t *const idle = luv_handle_of(uv_idle_t, lhandle);

  const int ret = uv_idle_init(ctx->loop, idle);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_pushfail(L, ret);
  }

  return 1;
}

LUV_CBAPI void luv_idle_cb(uv_idle_t *const handle) {
  luv_handle_t *const lhandle = luv_handle_from(handle);
  lua_State *const L = lhandle->ctx->L;

  luv_callback_send(L, LUV_CB_EVENT, lhandle, 0);
}

LUV_LUAAPI int luv_idle_start(lua_State *const L) {
  uv_idle_t *const idle = luv_idle_check(L, 1);
  luv_handle_t *const lhandle = luv_handle_from(idle);

  luv_callback_prep(L, LUV_CB_EVENT, lhandle, 2);

  const int ret = uv_idle_start(idle, luv_idle_cb);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

LUV_LUAAPI int luv_idle_stop(lua_State *const L) {
  uv_idle_t *const idle = luv_idle_check(L, 1);
  const int ret = uv_idle_stop(idle);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}
