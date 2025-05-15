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

#ifndef LUV_STREAM_H
#define LUV_STREAM_H

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "internal.h"

LUV_LIBAPI luaL_Reg luv_stream_methods[];
LUV_LIBAPI luaL_Reg luv_stream_functions[];

/* create and populate the metatable for the stream named name */
LUV_LIBAPI int luv_stream_metatable(lua_State *L, const char *name, const luaL_Reg *methods);

/* ensure the userdata at the given index is a stream and return a pointer to it */
LUV_LIBAPI uv_stream_t *luv_check_stream(lua_State *L, int index);

LUV_LUAAPI int luv_shutdown(lua_State *L);

LUV_LUAAPI int luv_listen(lua_State *L);

LUV_LUAAPI int luv_accept(lua_State *L);

LUV_LUAAPI int luv_read_start(lua_State *L);

LUV_LUAAPI int luv_read_stop(lua_State *L);

LUV_LUAAPI int luv_write(lua_State *L);

LUV_LUAAPI int luv_write2(lua_State *L);

LUV_LUAAPI int luv_try_write(lua_State *L);

LUV_LUAAPI int luv_is_readable(lua_State *L);

LUV_LUAAPI int luv_is_writable(lua_State *L);

LUV_LUAAPI int luv_stream_set_blocking(lua_State *L);

#if LUV_UV_VERSION_GEQ(1, 19, 0)
LUV_LUAAPI int luv_stream_get_write_queue_size(lua_State *L);
#endif

#if LUV_UV_VERSION_GEQ(1, 42, 0)
LUV_LUAAPI int luv_try_write2(lua_State *L);
#endif

#endif  // LUV_STREAM_H