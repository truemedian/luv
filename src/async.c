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

#include "async.h"

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "handle.h"
#include "lthreadpool.h"
#include "luv.h"
#include "private.h"

LUV_DEFAPI luaL_Reg luv_async_methods[] = {
  {"send", luv_async_send},
  {NULL, NULL},
};

LUV_DEFAPI luaL_Reg luv_async_functions[] = {
  {"new_async", luv_new_async},
  {"async_send", luv_async_send},
  {NULL, NULL},
};

LUV_LIBAPI uv_async_t *luv_check_async(lua_State *const L, const int index) {
  luv_handle_t *const lhandle = (luv_handle_t *)luv_checkudata(L, index, "uv_async");
  uv_async_t *const async = luv_handle_of(uv_async_t, lhandle);

  luaL_argcheck(L, async->type == UV_ASYNC, index, "expected uv_async handle");
  return async;
}

LUV_CBAPI void luv_async_cb(uv_async_t *const async) {
  luv_handle_t *lhandle = luv_handle_from(async);
  luv_thread_arg_t *args = (luv_thread_arg_t *)luv_handle_extra(uv_async_t, lhandle);
  lua_State *L = lhandle->ctx->L;

  const int argc = luv_thread_arg_push(L, args, LUVF_THREAD_SIDE_MAIN);
  luv_callback_send(L, LUV_CB_EVENT, lhandle, argc);
  luv_thread_arg_clear(L, args, LUVF_THREAD_SIDE_MAIN);
}

LUV_LUAAPI int luv_new_async(lua_State *const L) {
  const luv_ctx_t *const ctx = luv_context(L);

  luv_handle_t *const lhandle = luv_new_handle(L, UV_ASYNC, ctx, sizeof(luv_thread_arg_t));
  uv_async_t *const async = luv_handle_of(uv_async_t, lhandle);

  const int ret = uv_async_init(ctx->loop, async, luv_async_cb);
  if (ret < 0) {
    lua_pop(L, 1);
    luv_handle_unref(L, lhandle);
    return luv_error(L, ret);
  }

  luv_callback_prep(L, LUV_CB_EVENT, lhandle, 1);
  return 1;
}

LUV_LUAAPI int luv_async_send(lua_State *const L) {
  uv_async_t *const async = luv_check_async(L, 1);
  luv_handle_t *const lhandle = luv_handle_from(async);
  luv_thread_arg_t *const args = (luv_thread_arg_t *)luv_handle_extra(uv_async_t, lhandle);

  luv_thread_arg_set(L, args, 2, lua_gettop(L), LUVF_THREAD_MODE_ASYNC | LUVF_THREAD_SIDE_CHILD);
  const int ret = uv_async_send(async);
  luv_thread_arg_clear(L, args, LUVF_THREAD_SIDE_CHILD);
  return luv_result(L, ret);
}
