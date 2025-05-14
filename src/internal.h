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

#ifndef LUV_INTERNAL_H
#define LUV_INTERNAL_H

#include <lua.h>

#define LUV_LUAAPI extern
#define LUV_LIBAPI extern
#define LUV_DEFAPI
#define LUV_CBAPI static

#if __STDC_VERSION__ < 201112L
// noreturn not present until C11
#define noreturn
#elif __STDC_VERSION < 202311L
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

// likely() and unlikely() are hints to the compiler about the expected branch of a conditional statement.
#if defined(__GNUC__) || defined(__clang__)
#define luv_unlikely(cond) (__builtin_expect(((cond) != 0), 0))
#define luv_likely(cond) (__builtin_expect(((cond) != 0), 1))
#else
#define luv_unlikely(cond) (cond)
#define luv_likely(cond) (cond)
#endif

#define luv_error(L, ...)         \
  do {                            \
    luaL_error((L), __VA_ARGS__); \
    unreachable();                \
  } while (0)

#if defined(NDEBUG)
#define luv_assert(L, cond)                          \
  do {                                               \
    if (luv_unlikely(!(cond))) {                     \
      luv_error((L), "assertion failed: %s", #cond); \
    }                                                \
  } while (0)
#else
#define luv_assert(L, cond)                                                      \
  do {                                                                           \
    if (luv_unlikely(!(cond))) {                                                 \
      luv_error((L), "[%s:%d] assertion failed: %s", __FILE__, __LINE__, #cond); \
    }                                                                            \
  } while (0)
#endif

LUV_LIBAPI noreturn void luv_argerror(lua_State *L, int arg, const char *fmt, ...);

LUV_LIBAPI noreturn void luv_typeerror(lua_State *L, int arg, const char *tname);

LUV_LIBAPI int luv_pushfail(lua_State *L, int status);

LUV_LIBAPI int luv_pushresult(lua_State *L, int status, int type);

LUV_LIBAPI int luv_pushstatus(lua_State *L, int status);

LUV_LIBAPI int luv_iscallable(lua_State *L, int index);

LUV_LIBAPI void luv_checkcallable(lua_State *L, int index);

LUV_LIBAPI void *luv_newuserdata(lua_State *L, size_t sz);

LUV_LIBAPI void *luv_checkuserdata(lua_State *L, int idx, const char *tname);

#endif  // LUV_INTERNAL_H
