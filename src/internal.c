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
 */
#include "internal.h"

#include <lauxlib.h>
#include <lua.h>
#include <stdarg.h>
#include <uv.h>

static noreturn void luv_argerror(lua_State* const L, const int arg, const char* const fmt, ...) {
  va_list va_args;
  va_start(va_args, fmt);
  const char* extramsg = lua_pushvfstring(L, fmt, va_args);
  va_end(va_args);

  luaL_argerror(L, arg, extramsg);
  unreachable();
}

static noreturn void luv_typeerror(lua_State* L, const int arg, const char* const tname) {
  const char* typearg;
  if (luaL_getmetafield(L, arg, "__name") == LUA_TSTRING)
    typearg = lua_tostring(L, -1);
  else if (lua_type(L, arg) == LUA_TLIGHTUSERDATA)
    typearg = "light userdata";
  else
    typearg = luaL_typename(L, arg);

  luv_argerror(L, arg, "expected %s, got %s", tname, typearg);
}

static int luv_checkopttype(lua_State* const L, const int arg, const int type) {
  if (lua_isnoneornil(L, arg))
    return 0;

  if (lua_type(L, 1) != type) {
    const char* const tname = lua_pushfstring(L, "%s or nil", lua_typename(L, type));
    luv_typeerror(L, arg, tname);
  }

  return 1;
}

static void* luv_newuserdata(lua_State* const L, const size_t alloc_sz) {
#if LUA_VERSION_NUM >= 504
  /* avoid allocating space for a uservalue we're not going to use*/
  return lua_newuserdatauv(L, alloc_sz, 0);
#else
  return lua_newuserdata(L, alloc_sz);
#endif
}

static int luv_pushfail(lua_State* const L, const int status) {
  luv_assert(L, status < 0);

  lua_pushnil(L);
  lua_pushfstring(L, "%s: %s", uv_err_name(status), uv_strerror(status));
  lua_pushstring(L, uv_err_name(status));
  return 3;
}

static int luv_pushresult(lua_State* const L, const int status, const int type) {
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

static int luv_pushstatus(lua_State* const L, const int status) {
  if (status < 0)
    lua_pushstring(L, uv_err_name(status));
  else
    lua_pushnil(L);

  return 1;
}

static int luv_iscallable(lua_State* const L, const int index) {
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

static void luv_checkcallable(lua_State* const L, const int index) {
  if (!luv_iscallable(L, index))
    luv_typeerror(L, index, "callable");
}
