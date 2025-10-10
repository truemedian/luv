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

#include "handle.h"
#include "internal.h"

static luaL_Reg luv_idle_methods[] = {
    {"start", luv_idle_start},
    {"stop", luv_idle_stop},
    {NULL, NULL},
};

LUV_LIBAPI void luv_idle_init(lua_State *L) {
    luv_handle_metatable(L, UV_IDLE, luv_idle_methods);
    lua_pop(L, 1);
}

LUV_LIBAPI uv_idle_t *luv_check_idle(lua_State *L, int index) {
    luv_handle_t *lhandle = (luv_handle_t *)luaL_checkudata(L, index, "uv_idle");
    uv_idle_t *handle = luv_handle_of(uv_idle_t, lhandle);
    luv_typecheck(L, handle->type == UV_IDLE, index, "uv_idle");
    return handle;
}

LUV_LUAAPI int luv_new_idle(lua_State *L) {
    uv_loop_t *loop = luv_loop(L);

    luv_handle_t *lhandle = luv_new_handle(L, UV_IDLE);
    uv_idle_t *handle = luv_handle_of(uv_idle_t, lhandle);

    (void)uv_idle_init(loop, handle);
    return 1;
}

static void luv_idle_cb(uv_idle_t *handle) {
    luv_handle_t *lhandle = luv_handle_from(handle);
    lua_State *L = luv_handle_lua(handle);

    luv_callback_send(L, LUV_CB_EVENT, lhandle, 0);
}

LUV_LUAAPI int luv_idle_start(lua_State *L) {
    uv_idle_t *handle = luv_idle_check(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_set(L, LUV_CB_EVENT, lhandle, 2);
    (void)uv_idle_start(handle, luv_idle_cb);
    return 0;
}

LUV_LUAAPI int luv_idle_stop(lua_State *L) {
    uv_idle_t *handle = luv_idle_check(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    (void)uv_idle_stop(handle);
    return 0;
}
