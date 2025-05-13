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
#ifndef LUV_DNS_H
#define LUV_DNS_H

#ifndef WIN32
#include <netdb.h>
#else
#include <ws2def.h>
#endif

#include <lua.h>

#include "private.h"

LUV_LIBAPI void luv_pushaddrinfo(lua_State *L, const struct addrinfo *res);

LUV_LUAAPI int luv_getaddrinfo(lua_State *L);

LUV_LUAAPI int luv_getnameinfo(lua_State *L);

#endif