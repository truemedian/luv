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
#include "tty.h"

#include "handle.h"
#include "internal.h"

static luaL_Reg luv_tty_methods[] = {
    {"set_mode", luv_tty_set_mode},
    {"reset_mode", luv_tty_reset_mode},
    {"get_winsize", luv_tty_get_winsize},
#if LUV_UV_VERSION_GEQ(1, 33, 0)
    {"set_vterm_state", luv_tty_set_vterm_state},
    {"get_vterm_state", luv_tty_get_vterm_state},
#endif
    {NULL, NULL},
};

LUV_LIBAPI void luv_tty_init(lua_State *L) {
    luv_handle_metatable(L, UV_TTY, luv_tty_methods);
    lua_pop(L, 1);
}

LUV_LIBAPI uv_tty_t *luv_check_tty(lua_State *L, int index) {
    luv_handle_t *lhandle = (luv_handle_t *)luaL_checkudata(L, index, "uv_tty");
    uv_tty_t *handle = luv_handle_of(uv_tty_t, lhandle);
    luv_typecheck(L, handle->type == UV_TTY, index, "uv_tty");
    return handle;
}

LUV_LUAAPI int luv_new_tty(lua_State *L) {
    uv_loop_t *loop = luv_loop(L);

    luv_handle_t *lhandle = luv_new_handle(L, UV_TTY);
    uv_tty_t *handle = luv_handle_of(uv_tty_t, lhandle);

    uv_file fd = luaL_checkinteger(L, 1);
    luaL_checktype(L, 2, LUA_TBOOLEAN);
    int readable = lua_toboolean(L, 2);

    int ret = uv_tty_init(loop, handle, fd, readable);
    if (ret < 0)
        goto fail;

    return 1;

fail:
    lua_pop(L, 1);
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_tty_set_mode(lua_State *L) {
    uv_tty_t *handle = luv_check_tty(L, 1);

    uv_tty_mode_t mode;
    if (lua_isnumber(L, 2)) {
        mode = (uv_tty_mode_t)lua_tointeger(L, 2);

#if LUV_UV_VERSION_GEQ(1, 51, 0)
        luv_argcheck(L, mode >= UV_TTY_MODE_NORMAL && mode <= UV_TTY_MODE_RAW_VT, 2, "unknown tty mode value");
#elif LUV_UV_VERSION_GEQ(1, 2, 0)
        luv_argcheck(L, mode >= UV_TTY_MODE_NORMAL && mode <= UV_TTY_MODE_IO, 2, "unknown tty mode value");
#endif
    } else {
#if LUV_UV_VERSION_GEQ(1, 51, 0)
        const char *modes[] = {"normal", "raw", "io", "raw_vt", NULL};
#else
        const char *modes[] = {"normal", "raw", "io", NULL};
#endif
        int idx = luaL_checkoption(L, 2, NULL, modes);
        mode = (uv_tty_mode_t)idx;
    }

    int ret = uv_tty_set_mode(handle, mode);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_tty_reset_mode(lua_State *L) {
    int ret = uv_tty_reset_mode();
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_tty_get_winsize(lua_State *L) {
    uv_tty_t *handle = luv_check_tty(L, 1);
    int width, height;

    int ret = uv_tty_get_winsize(handle, &width, &height);
    if (ret < 0)
        goto fail;

    lua_pushinteger(L, width);
    lua_pushinteger(L, height);
    return 2;

fail:
    return luv_pushfail(L, ret);
}

#if LUV_UV_VERSION_GEQ(1, 33, 0)
LUV_LUAAPI int luv_tty_set_vterm_state(lua_State *L) {
    const char *option[] = {"supported", "unsupported", NULL};
    int idx = luaL_checkoption(L, 1, NULL, option);
    uv_tty_vtermstate_t state = (idx == 0) ? UV_TTY_SUPPORTED : UV_TTY_UNSUPPORTED;

    uv_tty_set_vterm_state(state);
    return 0;
}

LUV_LUAAPI int luv_tty_get_vterm_state(lua_State *L) {
    uv_tty_vtermstate_t state;
    int ret = uv_tty_get_vterm_state(&state);
    if (ret < 0)
        goto fail;

    switch (state) {
        case UV_TTY_SUPPORTED:
            lua_pushliteral(L, "supported");
            break;
        case UV_TTY_UNSUPPORTED:
            lua_pushliteral(L, "unsupported");
            break;
        default:
            luv_error(L, "unexpected vtermstate: %d", state);
    }

    return 1;

fail:
    return luv_pushfail(L, ret);
}

#endif
