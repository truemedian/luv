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

#include "signal.h"

#include "constants.h"
#include "handle.h"
#include "internal.h"

static luaL_Reg luv_signal_methods[] = {
    {"start", luv_signal_start},
#if LUV_UV_VERSION_GEQ(1, 12, 0)
    {"start_oneshot", luv_signal_start_oneshot},
#endif
    {"stop", luv_signal_stop},
    {NULL, NULL},
};

LUV_LIBAPI void luv_signal_init(lua_State *L) {
    luv_handle_metatable(L, UV_SIGNAL, luv_signal_methods);
    lua_pop(L, 1);
}

LUV_LIBAPI uv_signal_t *luv_check_signal(lua_State *L, int index) {
    luv_handle_t *lhandle = (luv_handle_t *)luaL_checkudata(L, index, "uv_signal");
    uv_signal_t *handle = luv_handle_of(uv_signal_t, lhandle);
    luv_typecheck(L, handle->type == UV_SIGNAL, index, "uv_signal");
    return handle;
}

LUV_LUAAPI int luv_new_signal(lua_State *L) {
    uv_loop_t *loop = luv_loop(L);

    luv_handle_t *lhandle = luv_new_handle(L, UV_SIGNAL);
    uv_signal_t *handle = luv_handle_of(uv_signal_t, lhandle);
    int ret = uv_signal_init(loop, handle);
    if (ret < 0)
        goto fail;

    return 1;

fail:
    lua_pop(L, 1);
    return luv_pushfail(L, ret);
}

static void luv_signal_cb(uv_signal_t *handle, int signum) {
    luv_handle_t *lhandle = luv_handle_from(handle);
    lua_State *L = luv_handle_state(lhandle);

    lua_pushstring(L, luv_signal_int2str(signum));
    luv_callback_send(L, LUV_CB_EVENT, lhandle, 1);
}

LUV_LUAAPI int luv_signal_start(lua_State *L) {
    uv_signal_t *handle = luv_check_signal(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    int signum;
    if (lua_isnumber(L, 2)) {
        signum = lua_tointeger(L, 2);
    } else if (lua_isstring(L, 2)) {
        signum = luv_signal_str2int(luaL_checkstring(L, 2));
        luv_argcheck(L, signum >= 0, 2, "invalid signal");
    } else {
        luv_typeerror(L, 2, "string or integer");
    }

    if (!lua_isnoneornil(L, 3))
        luv_callback_set(L, LUV_CB_EVENT, lhandle, 3);

    int ret = uv_signal_start(handle, luv_signal_cb, signum);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    return luv_pushfail(L, ret);
}

#if LUV_UV_VERSION_GEQ(1, 12, 0)
LUV_LUAAPI int luv_signal_start_oneshot(lua_State *L) {
    uv_signal_t *handle = luv_check_signal(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    int signum;
    if (lua_isnumber(L, 2)) {
        signum = lua_tointeger(L, 2);
    } else if (lua_isstring(L, 2)) {
        signum = luv_signal_str2int(luaL_checkstring(L, 2));
        luv_argcheck(L, signum >= 0, 2, "invalid signal");
    } else {
        luv_typeerror(L, 2, "string or integer");
    }

    if (!lua_isnoneornil(L, 3))
        luv_callback_set(L, LUV_CB_EVENT, lhandle, 3);

    int ret = uv_signal_start_oneshot(handle, luv_signal_cb, signum);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    return luv_pushfail(L, ret);
}
#endif

LUV_LUAAPI int luv_signal_stop(lua_State *L) {
    uv_signal_t *handle = luv_check_signal(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    int ret = uv_signal_stop(handle);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    return luv_pushfail(L, ret);
}
