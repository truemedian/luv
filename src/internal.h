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
#ifndef LUV_INTERNAL_H
#define LUV_INTERNAL_H

#include <lauxlib.h>
#include <lua.h>

#define LUV_UV_VERSION_GEQ(major, minor, patch) (((major) << 16 | (minor) << 8 | (patch)) <= UV_VERSION_HEX)

#define LUV_UV_VERSION_LEQ(major, minor, patch) (((major) << 16 | (minor) << 8 | (patch)) >= UV_VERSION_HEX)

#if __STDC_VERSION__ < 201112L
// noreturn not present until C11
#define noreturn
#elif __STDC_VERSION__ < 202311L
// noreturn provided by stdnoreturn.h until C23
#include <stdnoreturn.h>
#else
// noreturn is a function attribute in C23
#define noreturn [[noreturn]]
#endif

// unreachable() is a macro that indicates that the code should never be reached.
#if defined(__GNUC__) || defined(__clang__)
#define unreachable() (__builtin_unreachable())
#elif defined(_MSC_VER)
#define unreachable() (__assume(0))
#else
#define unreachable() (abort())
#endif

// luv_likely() and luv_unlikely() are hints to the compiler about the expected branch of a conditional statement.
#if defined(__GNUC__) || defined(__clang__)
#define luv_unlikely(cond) (__builtin_expect(((cond) != 0), 0))
#define luv_likely(cond) (__builtin_expect(((cond) != 0), 1))
#else
#define luv_unlikely(cond) (cond)
#define luv_likely(cond) (cond)
#endif

// throw an error with the given format string and arguments
#define luv_error(L, ...)         \
  do {                            \
    luaL_error((L), __VA_ARGS__); \
    unreachable();                \
  } while (0)

#if defined(NDEBUG)
// ensure the condition is true, throw an error otherwise
#define luv_assert(L, cond)                          \
  do {                                               \
    if (luv_unlikely(!(cond))) {                     \
      luv_error((L), "assertion failed: %s", #cond); \
    }                                                \
  } while (0)
#else
// ensure the condition is true, throw an error otherwise
#define luv_assert(L, cond)                                                      \
  do {                                                                           \
    if (luv_unlikely(!(cond))) {                                                 \
      luv_error((L), "[%s:%d] assertion failed: %s", __FILE__, __LINE__, #cond); \
    }                                                                            \
  } while (0)
#endif

// throw an error about a function argument with a given format string and arguments
static noreturn void luv_argerror(lua_State* L, int arg, const char* fmt, ...);

// throw an error about the type of a function argument being incorrect
static noreturn void luv_typeerror(lua_State* L, int arg, const char* tname);

// returns true if the value at the given arg is the expected type, false if it is nil, and throws an error otherwise
static int luv_checkopttype(lua_State* L, int arg, int type);

// create a new userdata with the given size
static void* luv_newuserdata(lua_State* L, size_t alloc_sz);

// push a unsuccessful uv status as nil, errmsg, errname. status must be negative
static int luv_pushfail(lua_State* L, int status);

// push a uv status as either a number or boolean depending on type; or a failure
static int luv_pushresult(lua_State* L, int status, int type);

// push a uv status as either a string representing the error name or nil on success
static int luv_pushstatus(lua_State* L, int status);

// returns true if the value at the given index is either a function or callable table/userdata
static int luv_iscallable(lua_State* L, int index);

// ensure the value at the given index is callable (see above)
static void luv_checkcallable(lua_State* L, int index);

#endif