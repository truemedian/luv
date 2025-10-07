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
#ifndef LUV_CONSTANTS_H
#define LUV_CONSTANTS_H

#include <lua.h>

#include "internal.h"

/* pushes a table of libuv and system constants onto the stack */
LUV_LIBAPI void luv_push_constant_table(lua_State *L);

/* returns the integer representation of the address family specified by string */
LUV_LIBAPI int luv_addrfamily_str2int(const char *string);

/* returns a static string representation of the address family specified by num */
LUV_LIBAPI const char *luv_addrfamily_int2str(int num);

/* returns the integer representation of the socket domain specified by string */
LUV_LIBAPI int luv_sockdomain_str2int(const char *string);

/* returns a static string representation of the socket domain specified by num */
LUV_LIBAPI const char *luv_sockdomain_int2str(int num);

/* returns the integer representation of the signal specified by string */
LUV_LIBAPI int luv_signal_str2int(const char *string);

/* returns a static string representation of the signal specified by num */
LUV_LIBAPI const char *luv_signal_int2str(int num);

/* returns the integer representation of the socket protocol specified by string */
LUV_LIBAPI int luv_protocol_str2int(const char *string);

/* returns a temporary string representation of the socket protocol specified by num */
LUV_LIBAPI const char *luv_protocol_int2str(int num);

#endif  // LUV_CONSTANTS_H