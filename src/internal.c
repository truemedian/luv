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

#include <lauxlib.h>
#include <lua.h>

LUV_LIBAPI noreturn void luv_argerror(lua_State *L, int arg, const char *fmt, ...) {
  va_list va_args;
  va_start(va_args, fmt);
  const char *extramsg = lua_pushvfstring(L, fmt, va_args);
  va_end(va_args);

  luaL_argerror(L, arg, extramsg);
  unreachable();
}

LUV_LIBAPI noreturn void luv_typeerror(lua_State *L, int arg, const char *tname) {
  const char *typearg;
  if (luaL_getmetafield(L, arg, "__name") == LUA_TSTRING) {
    typearg = lua_tostring(L, -1);
  } else if (lua_type(L, arg) == LUA_TLIGHTUSERDATA) {
    typearg = "light userdata";
  } else {
    typearg = luaL_typename(L, arg);
  }

  luv_argerror(L, arg, "expected %s, got %s", tname, typearg);
}

LUV_LIBAPI int luv_pushfail(lua_State *L, int status) {
  luv_assert(L, status < 0);

  lua_pushnil(L);
  lua_pushfstring(L, "%s: %s", uv_err_name(status), uv_strerror(status));
  lua_pushstring(L, uv_err_name(status));
  return 3;
}

LUV_LIBAPI int luv_pushresult(lua_State *L, int status) {
  if (status < 0) {
    return luv_pushfail(L, status);
  }

  lua_pushinteger(L, status);
  return 1;
}

LUV_LIBAPI int luv_pushstatus(lua_State *L, int status) {
  if (status < 0) {
    lua_pushstring(L, uv_err_name(status));
  } else {
    lua_pushnil(L);
  }

  return 1;
}