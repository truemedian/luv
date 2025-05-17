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

static int luv_error(lua_State* L, int status) {
  assert(status < 0);
  lua_pushnil(L);
  lua_pushfstring(L, "%s: %s", uv_err_name(status), uv_strerror(status));
  lua_pushstring(L, uv_err_name(status));
  return 3;
}

static int luv_result(lua_State* L, int status) {
  if (status < 0)
    return luv_error(L, status);
  lua_pushinteger(L, status);
  return 1;
}

static void luv_status(lua_State* L, int status) {
  if (status < 0) {
    lua_pushstring(L, uv_err_name(status));
  } else {
    lua_pushnil(L);
  }
}

static int luv_iscallable(lua_State* L, int index) {
  if (luaL_getmetafield(L, index, "__call") != LUA_TNIL) {
    // getmetatable(x).__call must be a function for x() to work
    int callable = lua_isfunction(L, -1);
    lua_pop(L, 1);
    return callable;
  }
  return lua_isfunction(L, index);
}

static void luv_checkcallable(lua_State* L, int index) {
  if (!luv_iscallable(L, index))
    luv_typeerror(L, index, "function or callable table");
}

static int luv_typeerror(lua_State* L, int index, const char* tname) {
  const char* typearg;

  if (luaL_getmetafield(L, index, "__name") == LUA_TSTRING)
    typearg = lua_tostring(L, -1);
  else if (lua_type(L, index) == LUA_TLIGHTUSERDATA)
    typearg = "light userdata";
  else
    typearg = luaL_typename(L, index);

  const char* extramsg = lua_pushfstring(L, "expected %s, got %s", tname, typearg);
  luaL_argerror(L, index, extramsg);
}

#if LUV_UV_VERSION_GEQ(1, 10, 0)
static int luv_translate_sys_error(lua_State* L) {
  int status = luaL_checkinteger(L, 1);
  status = uv_translate_sys_error(status);
  if (status < 0) {
    luv_error(L, status);
    lua_remove(L, -3);
    return 2;
  }
  return 0;
}
#endif

static int luv_optboolean(lua_State* L, int idx, int val) {
  idx = lua_absindex(L, idx);
  luaL_argcheck(L, lua_isboolean(L, idx) || lua_isnoneornil(L, idx), idx, "Expected boolean or nil");

  if (lua_isboolean(L, idx))
    val = lua_toboolean(L, idx);
  return val;
}
