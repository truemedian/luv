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

#ifndef LUV_FS_EVENT_H
#define LUV_FS_EVENT_H

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "internal.h"

LUV_LIBAPI luaL_Reg luv_fs_event_methods[];
LUV_LIBAPI luaL_Reg luv_fs_event_functions[];

/* ensure the userdata at the given index is a uv_fs_event and return a pointer to it */
LUV_LIBAPI uv_fs_event_t *luv_check_fs_event(lua_State *L, int index);

/* new_fs_event() -> uv_fs_event */
LUV_LUAAPI int luv_new_fs_event(lua_State *L);

/* fs_event:start(path, flags, callback) -> integer */
LUV_LUAAPI int luv_fs_event_start(lua_State *L);

/* fs_event:stop() -> integer */
LUV_LUAAPI int luv_fs_event_stop(lua_State *L);

/* fs_event:getpath() -> string */
LUV_LUAAPI int luv_fs_event_getpath(lua_State *L);

#endif  // LUV_FS_EVENT_H