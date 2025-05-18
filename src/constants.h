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

#if defined(_WIN32)
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#ifndef S_ISREG
#define S_ISREG(x) (((x) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(x) (((x) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(x) (((x) & _S_IFMT) == _S_IFIFO)
#endif
#ifndef S_ISCHR
#define S_ISCHR(x) (((x) & _S_IFMT) == _S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK(x) 0
#endif
#ifndef S_ISLNK
#define S_ISLNK(x) (((x) & S_IFLNK) == S_IFLNK)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(x) 0
#endif
#else
#include <unistd.h>
#endif

#include <lua.h>

#include "internal.h"

// pushes a table of libuv and system constants onto the stack
static int luv_push_constant_table(lua_State* L);

// returns the integer representation of the address family specified by string
static int luv_addrfamily_str2int(const char* string);

// returns a static string representation of the address family specified by num
static const char* luv_addrfamily_int2str(int num);

// returns the integer representation of the socket domain specified by string
static int luv_sockdomain_str2int(const char* string);

// returns a static string representation of the socket domain specified by num
static const char* luv_sockdomain_int2str(int num);

// returns the integer representation of the signal specified by string
static int luv_signal_str2int(const char* string);

// returns a static string representation of the signal specified by num
static const char* luv_signal_int2str(int num);

// returns the integer representation of the socket protocol specified by string
static int luv_protocol_str2int(const char* string);

// returns a temporary string representation of the socket protocol specified by num
static const char* luv_protocol_int2str(int num);

#endif  // LUV_CONSTANTS_H