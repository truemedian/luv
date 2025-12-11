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
#ifndef LUV_TIMER_H
#define LUV_TIMER_H

#include "internal.h"

LUV_LIBAPI void luv_timer_init(lua_State *L);
LUV_LIBAPI uv_timer_t *luv_check_timer(lua_State *L, int index);

LUV_LUAAPI int luv_new_timer(lua_State *L);
LUV_LUAAPI int luv_timer_start(lua_State *L);
LUV_LUAAPI int luv_timer_stop(lua_State *L);
LUV_LUAAPI int luv_timer_again(lua_State *L);
LUV_LUAAPI int luv_timer_set_repeat(lua_State *L);
LUV_LUAAPI int luv_timer_get_repeat(lua_State *L);
#if LUV_UV_VERSION_GEQ(1, 40, 0)
LUV_LUAAPI int luv_timer_get_due_in(lua_State *L);
#endif

luaL_Reg luv_timer_functions[] = {
    {"new_timer", luv_new_timer},
    {"timer_start", luv_timer_start},
    {"timer_stop", luv_timer_stop},
    {"timer_again", luv_timer_again},
    {"timer_set_repeat", luv_timer_set_repeat},
    {"timer_get_repeat", luv_timer_get_repeat},
#if LUV_UV_VERSION_GEQ(1, 40, 0)
    {"timer_get_due_in", luv_timer_get_due_in},
#endif
    {NULL, NULL},
};

#endif
