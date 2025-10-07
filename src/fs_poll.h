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
#ifndef LUV_FS_POLL_H
#define LUV_FS_POLL_H

#include "internal.h"

LUV_LIBAPI void luv_fs_poll_init(lua_State *L);
LUV_LIBAPI uv_fs_poll_t *luv_check_fs_poll(lua_State *L, int index);

LUV_LUAAPI int luv_new_fs_poll(lua_State *L);
LUV_LUAAPI int luv_fs_poll_start(lua_State *L);
LUV_LUAAPI int luv_fs_poll_stop(lua_State *L);
LUV_LUAAPI int luv_fs_poll_getpath(lua_State *L);

luaL_Reg luv_fs_event_functions[] = {
    {"new_fs_poll", luv_new_fs_poll},
    {"fs_poll_start", luv_fs_poll_start},
    {"fs_poll_stop", luv_fs_poll_stop},
    {"fs_poll_getpath", luv_fs_poll_getpath},
    {NULL, NULL},
};

#endif