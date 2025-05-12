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
#include "private.h"

typedef struct {
  uv_async_t handle;
  luv_thread_args_t args;
} luv_async_t;

static uv_async_t* luv_check_async(lua_State* L, int index) {
  uv_async_t* handle = (uv_async_t*)luv_checkudata(L, index, "uv_async");
  luaL_argcheck(L, handle->type == UV_ASYNC && handle->data, index, "Expected uv_async_t");
  return handle;
}

static void luv_async_cb(uv_async_t* handle) {
  luv_async_t *async = (luv_async_t*)handle;
  luv_handle_t* data = (luv_handle_t*)handle->data;
  luv_ctx_t* ctx = data->ctx;

  lua_State* L = ctx->L;
  int n = luv_thread_args_push(L, ctx, &async->args);
  luv_call_callback(L, data, LUV_ASYNC, n);
  luv_thread_args_cleanup(L, &async->args, LUVF_THREAD_MODE_ASYNC);
}

static int luv_new_async(lua_State* L) {
  luv_async_t* async = (luv_async_t*)luv_newuserdata(L, sizeof(luv_async_t));
  async->args.consumer = L;

  luv_ctx_t* ctx = luv_context(L);
  luv_check_callable(L, 1);

  int ret = uv_async_init(ctx->loop, &async->handle, luv_async_cb);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_error(L, ret);
  }

  async->handle.data = luv_setup_handle(L, ctx);
  luv_check_callback(L, (luv_handle_t*)async->handle.data, LUV_ASYNC, 1);

  return 1;
}

static int luv_async_send(lua_State* L) {
  uv_async_t* handle = luv_check_async(L, 1);
  luv_handle_t *data = (luv_handle_t*)handle->data;
  luv_thread_args_t* arg = (luv_thread_args_t *)data->extra;
  
  int top = lua_gettop(L);
  luv_thread_args_check(L, 2, top);
  luv_thread_args_prepare(L, data->ctx, arg, 2, top, LUVF_THREAD_MODE_ASYNC);
  int ret = uv_async_send(handle);
  return luv_result(L, ret);
}
