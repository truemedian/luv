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

#ifndef LUV_ASYNC_H
#define LUV_ASYNC_H

#include <lua.h>
#include <uv.h>

#include "private.h"

LUV_LIBAPI luaL_Reg luv_async_methods[];
LUV_LIBAPI luaL_Reg luv_async_functions[];

/* ensure the userdata at the given index is a uv_async and return a pointer to it */
LUV_LIBAPI uv_async_t *luv_check_async(lua_State *L, int index);

/* new_async(callback) -> uv_async */
LUV_LUAAPI int luv_new_async(lua_State *L);

/* async:send(...) -> integer */
LUV_LUAAPI int luv_async_send(lua_State *L);

#endif  // LUV_ASYNC_H
