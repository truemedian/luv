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
#ifndef LUV_TTY_H
#define LUV_TTY_H

#include "internal.h"

LUV_LIBAPI void luv_tty_init(lua_State *L);
LUV_LIBAPI uv_tty_t *luv_check_tty(lua_State *L, int index);

LUV_LUAAPI int luv_new_tty(lua_State *L);
LUV_LUAAPI int luv_tty_set_mode(lua_State *L);
LUV_LUAAPI int luv_tty_reset_mode(lua_State *L);
LUV_LUAAPI int luv_tty_get_winsize(lua_State *L);

#if LUV_UV_VERSION_GEQ(1, 33, 0)
LUV_LUAAPI int luv_tty_set_vterm_state(lua_State *L);
LUV_LUAAPI int luv_tty_get_vterm_state(lua_State *L);
#endif

luaL_Reg luv_tty_functions[] = {
    {"new_tty", luv_new_tty},
    {"set_tty_mode", luv_tty_set_mode},
    {"reset_tty_mode", luv_tty_reset_mode},
    {"get_tty_winsize", luv_tty_get_winsize},
#if LUV_UV_VERSION_GEQ(1, 33, 0)
    {"set_tty_vterm_state", luv_tty_set_vterm_state},
    {"get_tty_vterm_state", luv_tty_get_vterm_state},
#endif
    {NULL, NULL},
};

#endif