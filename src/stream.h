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
#ifndef LUV_STREAM_H
#define LUV_STREAM_H

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "handle.h"
#include "internal.h"
#include "luv.h"

// creates registers and pushes a uv stream metatable pre-filled with all stream and provided methods
static int luv_stream_metatable(lua_State* L, const char* name, const luaL_Reg* methods);

// check that the userdata at the given index is a stream and return a pointer to it
static luv_handle_t* luv_check_stream(lua_State* L, int index);

static int luv_shutdown(lua_State* L);
static int luv_listen(lua_State* L);
static int luv_accept(lua_State* L);

static int luv_read_start(lua_State* L);
static int luv_read_stop(lua_State* L);

static int luv_write(lua_State* L);
static int luv_write2(lua_State* L);
static int luv_try_write(lua_State* L);

static int luv_is_readable(lua_State* L);
static int luv_is_writable(lua_State* L);
static int luv_stream_set_blocking(lua_State* L);

#if LUV_UV_VERSION_GEQ(1, 19, 0)
static int luv_stream_get_write_queue_size(lua_State* L);
#endif

#if LUV_UV_VERSION_GEQ(1, 42, 0)
static int luv_try_write2(lua_State* L);
#endif

static luaL_Reg luv_stream_methods[] = {
  {"shutdown", luv_shutdown},
  {"listen", luv_listen},
  {"accept", luv_accept},
  {"read_start", luv_read_start},
  {"read_stop", luv_read_stop},
  {"write", luv_write},
  {"write2", luv_write2},
  {"try_write", luv_try_write},
  {"is_readable", luv_is_readable},
  {"is_writable", luv_is_writable},
  {"set_blocking", luv_stream_set_blocking},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"get_write_queue_size", luv_stream_get_write_queue_size},
#endif
#if LUV_UV_VERSION_GEQ(1, 42, 0)
  {"try_write2", luv_try_write2},
#endif
  {NULL, NULL},
};

static luaL_Reg luv_stream_functions[] = {
  {"shutdown", luv_shutdown},
  {"listen", luv_listen},
  {"accept", luv_accept},
  {"read_start", luv_read_start},
  {"read_stop", luv_read_stop},
  {"write", luv_write},
  {"write2", luv_write2},
  {"try_write", luv_try_write},
  {"is_readable", luv_is_readable},
  {"is_writable", luv_is_writable},
  {"stream_set_blocking", luv_stream_set_blocking},
#if LUV_UV_VERSION_GEQ(1, 19, 0)
  {"stream_get_write_queue_size", luv_stream_get_write_queue_size},
#endif
#if LUV_UV_VERSION_GEQ(1, 42, 0)
  {"try_write2", luv_try_write2},
#endif
};

#endif
