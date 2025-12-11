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
#ifndef LUV_IDLE_H
#define LUV_IDLE_H

#include "internal.h"

LUV_LIBAPI void luv_idle_init(lua_State *L);
LUV_LIBAPI uv_idle_t *luv_check_idle(lua_State *L, int index);

LUV_LUAAPI int luv_new_idle(lua_State *L);
LUV_LUAAPI int luv_idle_start(lua_State *L);
LUV_LUAAPI int luv_idle_stop(lua_State *L);

luaL_Reg luv_idle_functions[] = {
    {"new_idle", luv_new_idle},
    {"idle_start", luv_idle_start},
    {"idle_stop", luv_idle_stop},
    {NULL, NULL},
};

#endif
