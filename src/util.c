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
