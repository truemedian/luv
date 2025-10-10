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

#include "prepare.h"

#include "handle.h"
#include "internal.h"

static luaL_Reg luv_prepare_methods[] = {
    {"start", luv_prepare_start},
    {"stop", luv_prepare_stop},
    {NULL, NULL},
};

LUV_LIBAPI void luv_prepare_init(lua_State *L) {
    luv_handle_metatable(L, UV_PREPARE, luv_prepare_methods);
    lua_pop(L, 1);
}

LUV_LIBAPI uv_prepare_t *luv_check_prepare(lua_State *L, int index) {
    luv_handle_t *lhandle = (luv_handle_t *)luaL_checkudata(L, index, "uv_prepare");
    uv_prepare_t *handle = luv_handle_of(uv_prepare_t, lhandle);
    luv_typecheck(L, handle->type == UV_PREPARE, index, "uv_prepare");
    return handle;
}

LUV_LUAAPI int luv_new_prepare(lua_State *L) {
    uv_loop_t *loop = luv_loop(L);

    luv_handle_t *lhandle = luv_new_handle(L, UV_PREPARE);
    uv_prepare_t *handle = luv_handle_of(uv_prepare_t, lhandle);

    (void)uv_prepare_init(loop, handle);
    return 1;
}

static void luv_prepare_cb(uv_prepare_t *handle) {
    luv_handle_t *lhandle = luv_handle_from(handle);
    lua_State *L = luv_handle_state(lhandle);

    luv_callback_send(L, LUV_CB_EVENT, lhandle, 0);
}

LUV_LUAAPI int luv_prepare_start(lua_State *L) {
    uv_prepare_t *handle = luv_check_prepare(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_set(L, LUV_CB_EVENT, lhandle, 2);
    (void)uv_prepare_start(handle, luv_prepare_cb);
    return 0;
}

LUV_LUAAPI int luv_prepare_stop(lua_State *L) {
    uv_prepare_t *handle = luv_check_prepare(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    (void)uv_prepare_stop(handle);
    return 0;
}
