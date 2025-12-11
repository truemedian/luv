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

#include "internal.h"

LUV_LIBAPI noreturn void luv_argerrorv(lua_State *L, int arg, const char *fmt, va_list va_args) {
    const char *extramsg = lua_pushvfstring(L, fmt, va_args);
    va_end(va_args);

    luaL_argerror(L, arg, extramsg);
    unreachable();
}

LUV_LIBAPI noreturn void luv_argerror(lua_State *L, int arg, const char *fmt, ...) {
    va_list va_args;
    va_start(va_args, fmt);
    luv_argerrorv(L, arg, fmt, va_args);
    unreachable();
}

LUV_LIBAPI noreturn void luv_typeerror(lua_State *L, int arg, const char *tname) {
    const char *typearg;
    if (luaL_getmetafield(L, arg, "__name") == LUA_TSTRING)
        typearg = lua_tostring(L, -1);
    else if (lua_type(L, arg) == LUA_TLIGHTUSERDATA)
        typearg = "light userdata";
    else
        typearg = luaL_typename(L, arg);

    luv_argerror(L, arg, "expected %s, got %s", tname, typearg);
    unreachable();
}

LUV_LIBAPI int luv_pushsuccess(lua_State *L, int status) {
    luv_assert(L, status >= 0, "status must be non-negative");

    lua_pushboolean(L, 1);
    return 1;
}

LUV_LIBAPI int luv_pushfail(lua_State *L, int status) {
    luv_assert(L, status < 0, "status must be negative");

    lua_pushnil(L);
    lua_pushfstring(L, "%s: %s", uv_err_name(status), uv_strerror(status));
    lua_pushstring(L, uv_err_name(status));
    return 3;
}

LUV_LIBAPI int luv_pushresult(lua_State *L, int status, int type) {
    if (status < 0)
        return luv_pushfail(L, status);

    if (type == LUA_TNUMBER)
        lua_pushinteger(L, status);
    else if (type == LUA_TBOOLEAN)
        lua_pushboolean(L, 1);
    else
        lua_pushnil(L);

    return 1;
}

LUV_LIBAPI int luv_pushstatus(lua_State *L, int status) {
    if (status < 0)
        lua_pushstring(L, uv_err_name(status));
    else
        lua_pushnil(L);

    return 1;
}
LUV_LIBAPI int luv_iscallable(lua_State *L, int index) {
    switch (lua_type(L, index)) {
        case LUA_TFUNCTION:
            return 1;
        case LUA_TUSERDATA:
        case LUA_TTABLE:
            switch (luaL_getmetafield(L, index, "__call")) {
                case LUA_TNIL:
                    return 0;
                case LUA_TFUNCTION:
                    lua_pop(L, 1);
                    return 1;
                default:
                    break;
            }

            lua_pop(L, 1);
            return 0;
        default:
            break;
    }

    return 0;
}
