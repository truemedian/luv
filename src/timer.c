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
#include "timer.h"

#include "handle.h"
#include "internal.h"

static luaL_Reg luv_timer_methods[] = {
    {"start", luv_timer_start},
    {"stop", luv_timer_stop},
    {"again", luv_timer_again},
    {"set_repeat", luv_timer_set_repeat},
    {"get_repeat", luv_timer_get_repeat},
#if LUV_UV_VERSION_GEQ(1, 40, 0)
    {"get_due_in", luv_timer_get_due_in},
#endif
    {NULL, NULL},
};

LUV_LIBAPI void luv_timer_init(lua_State *L) {
    luv_handle_metatable(L, UV_TIMER, luv_timer_methods);
    lua_pop(L, 1);
}

LUV_LIBAPI uv_timer_t *luv_check_timer(lua_State *L, int index) {
    luv_handle_t *lhandle = (luv_handle_t *)luaL_checkudata(L, index, "uv_timer");
    uv_timer_t *handle = luv_handle_of(uv_timer_t, lhandle);
    luv_typecheck(L, handle->type == UV_TIMER, index, "uv_timer");
    return handle;
}

LUV_LUAAPI int luv_new_timer(lua_State *L) {
    uv_loop_t *loop = luv_loop(L);

    luv_handle_t *lhandle = luv_new_handle(L, UV_TIMER);
    uv_timer_t *handle = luv_handle_of(uv_timer_t, lhandle);

    int ret = uv_timer_init(loop, handle);
    if (ret < 0)
        goto fail;
    return 1;

fail:
    lua_pop(L, 1);
    return luv_pushfail(L, ret);
}

static void luv_timer_cb(uv_timer_t *handle) {
    luv_handle_t *lhandle = luv_handle_from(handle);
    lua_State *L = luv_handle_state(lhandle);

    luv_callback_send(L, LUV_CB_EVENT, lhandle, 0);
}

LUV_LUAAPI int luv_timer_start(lua_State *L) {
    uv_timer_t *handle = luv_check_timer(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);
    lua_Integer timeout = luaL_checkinteger(L, 2);
    lua_Integer repeat = luaL_checkinteger(L, 3);
    luv_argcheck(L, timeout >= 0, 2, "timeout must be non-negative");
    luv_argcheck(L, repeat >= 0, 3, "repeat must be non-negative");
    luv_callback_set(L, LUV_CB_EVENT, lhandle, 4);

    int ret = uv_timer_start(handle, luv_timer_cb, (uint64_t)timeout, (uint64_t)repeat);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_timer_stop(lua_State *L) {
    uv_timer_t *handle = luv_check_timer(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    int ret = uv_timer_stop(handle);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_timer_again(lua_State *L) {
    uv_timer_t *handle = luv_check_timer(L, 1);
    int ret = uv_timer_again(handle);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_timer_set_repeat(lua_State *L) {
    uv_timer_t *handle = luv_check_timer(L, 1);
    lua_Integer repeat = luaL_checkinteger(L, 2);
    luv_argcheck(L, repeat >= 0, 2, "repeat must be non-negative");

    uv_timer_set_repeat(handle, (uint64_t)repeat);
    return 0;
}

LUV_LUAAPI int luv_timer_get_repeat(lua_State *L) {
    uv_timer_t *handle = luv_check_timer(L, 1);
    uint64_t repeat = uv_timer_get_repeat(handle);
    lua_pushinteger(L, repeat);
    return 1;
}

#if LUV_UV_VERSION_GEQ(1, 40, 0)
LUV_LUAAPI int luv_timer_get_due_in(lua_State *L) {
    uv_timer_t *handle = luv_check_timer(L, 1);
    uint64_t val = uv_timer_get_due_in(handle);
    lua_pushinteger(L, val);
    return 1;
}
#endif
