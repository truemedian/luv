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
#include "metrics.h"

#include <lua.h>
#include <stdint.h>
#include <uv.h>

#include "luv.h"
#include "private.h"
#include "util.h"

LUV_LIBAPI luaL_Reg luv_metrics_functions[] = {
  {"metrics_idle_time", luv_metrics_idle_time},
#if LUV_UV_VERSION_GEQ(1, 45, 0)
  {"metrics_info", luv_metrics_info},
#endif
  {NULL, NULL},
};

LUV_LUAAPI int luv_metrics_idle_time(lua_State *L) {
#if LUV_UV_VERSION_GEQ(1, 39, 0)
  const uint64_t idle_time = uv_metrics_idle_time(luv_loop(L));
  lua_pushinteger(L, (lua_Integer)idle_time);
#else
  lua_pushinteger(L, 0);
#endif
  return 1;
}

#if LUV_UV_VERSION_GEQ(1, 45, 0)
LUV_LUAAPI int luv_metrics_info(lua_State *L) {
  uv_metrics_t metrics;
  const int ret = uv_metrics_info(luv_loop(L), &metrics);
  if (ret < 0) {
    return luv_error(L, ret);
  }

  lua_createtable(L, 0, 3);

  lua_pushinteger(L, (lua_Integer)metrics.loop_count);
  lua_setfield(L, -2, "loop_count");

  lua_pushinteger(L, (lua_Integer)metrics.events);
  lua_setfield(L, -2, "events");

  lua_pushinteger(L, (lua_Integer)metrics.events_waiting);
  lua_setfield(L, -2, "events_waiting");

  return 1;
}
#endif
