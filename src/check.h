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

#ifndef LUV_CHECK_H
#define LUV_CHECK_H

#include <lua.h>
#include <uv.h>

#include "private.h"

/* ensure the userdata at the given index is a uv_check and return a pointer to it */
LUV_LIBAPI uv_check_t *luv_check_check(lua_State *L, int index);

/* new_check() -> uv_check */
LUV_LUAAPI int luv_new_check(lua_State *L);

/* uv_check:start(callback) -> integer */
LUV_LUAAPI int luv_check_start(lua_State *L);

/* uv_check:stop() -> integer */
LUV_LUAAPI int luv_check_stop(lua_State *L);

#endif  // LUV_CHECK_H