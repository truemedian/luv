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

#ifndef LUV_PIPE_H
#define LUV_PIPE_H

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "internal.h"

LUV_LIBAPI luaL_Reg luv_pipe_methods[];
LUV_LIBAPI luaL_Reg luv_pipe_functions[];

/* ensure the userdata at the given index is a uv_pipe and return a pointer to it */
LUV_LIBAPI uv_pipe_t *luv_check_pipe(lua_State *L, int index);

/* reused with tcp for connect_cb */
LUV_LIBAPI void luv_connect_cb(uv_connect_t *connect, int status);

LUV_LUAAPI int luv_new_pipe(lua_State *L);
LUV_LUAAPI int luv_pipe_open(lua_State *L);
LUV_LUAAPI int luv_pipe_bind(lua_State *L);
LUV_LUAAPI int luv_pipe_connect(lua_State *L);
LUV_LUAAPI int luv_pipe_getsockname(lua_State *L);
LUV_LUAAPI int luv_pipe_pending_instances(lua_State *L);
LUV_LUAAPI int luv_pipe_pending_count(lua_State *L);
LUV_LUAAPI int luv_pipe_pending_type(lua_State *L);

#if LUV_UV_VERSION_GEQ(1, 3, 0)

LUV_LUAAPI int luv_pipe_getpeername(lua_State *L);

#endif
#if LUV_UV_VERSION_GEQ(1, 16, 0)

LUV_LUAAPI int luv_pipe_chmod(lua_State *L);

#endif
#if LUV_UV_VERSION_GEQ(1, 41, 0)

LUV_LUAAPI int luv_pipe(lua_State *L);

#endif
#if LUV_UV_VERSION_GEQ(1, 46, 0)

LUV_LIBAPI unsigned int luv_pipe_optflags(lua_State *L, int index, unsigned int flags);
LUV_LUAAPI int luv_pipe_bind2(lua_State *L);
LUV_LUAAPI int luv_pipe_connect2(lua_State *L);

#endif

#endif  // LUV_PIPE_H
