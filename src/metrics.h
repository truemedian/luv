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

#ifndef LUV_METRICS_H
#define LUV_METRICS_H

#include <lua.h>

#include "private.h"
#include "util.h"

LUV_LIBAPI luaL_Reg luv_metrics_functions[];

/* metrics_idle_time() -> integer */
LUV_LUAAPI int luv_metrics_idle_time(lua_State *L);

#if LUV_UV_VERSION_GEQ(1, 45, 0)

/* metrics_info() -> table */
LUV_LUAAPI int luv_metrics_info(lua_State *L);

#endif

#endif  // LUV_METRICS_H