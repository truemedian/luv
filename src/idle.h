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

#ifndef LUV_IDLE_H
#define LUV_IDLE_H

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "internal.h"

LUV_LIBAPI luaL_Reg luv_idle_methods[];
LUV_LIBAPI luaL_Reg luv_idle_functions[];

/* ensure the userdata at the given index is a uv_idle and return a pointer to it */
LUV_LIBAPI uv_idle_t *luv_idle_check(lua_State *L, int index);

/* new_idle(callback) -> uv_idle */
LUV_LUAAPI int luv_new_idle(lua_State *L);

/* uv_idle:start(callback) -> integer */
LUV_LUAAPI int luv_idle_start(lua_State *L);

/* uv_idle:stop() -> integer */
LUV_LUAAPI int luv_idle_stop(lua_State *L);

#endif  // LUV_IDLE_H