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

#ifndef LUV_SIGNAL_H
#define LUV_SIGNAL_H

#include "internal.h"

LUV_LIBAPI void luv_signal_init(lua_State *L);
LUV_LIBAPI uv_signal_t *luv_check_signal(lua_State *L, int index);

LUV_LUAAPI int luv_new_signal(lua_State *L);
LUV_LUAAPI int luv_signal_start(lua_State *L);
#if LUV_UV_VERSION_GEQ(1, 12, 0)
LUV_LUAAPI int luv_signal_start_oneshot(lua_State *L);
#endif
LUV_LUAAPI int luv_signal_stop(lua_State *L);

luaL_Reg luv_signal_functions[] = {
    {"new_signal", luv_new_signal},
    {"signal_start", luv_signal_start},
#if LUV_UV_VERSION_GEQ(1, 12, 0)
    {"signal_start_oneshot", luv_signal_start_oneshot},
#endif
    {"signal_stop", luv_signal_stop},
    {NULL, NULL},
};

#endif /* LUV_SIGNAL_H */
