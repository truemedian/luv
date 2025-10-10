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
#ifndef LUV_H
#define LUV_H
#include "internal.h"
#include "uv.h"

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

#ifndef PATH_MAX
#define PATH_MAX (8096)
#endif

#ifndef MAX_TITLE_LENGTH
#define MAX_TITLE_LENGTH (8192)
#endif

#ifndef MAX_THREAD_NAME_LENGTH
#define MAX_THREAD_NAME_LENGTH (8192)
#endif

/* Prototype of external callback routine.
 * The caller and the implementer exchanges data by the lua vm stack.
 * The caller push a lua function and nargs values onto the stack, then call it.
 * The implementer remove nargs(argument)+1(function) values from vm stack,
 * push all returned values by lua function onto the stack, and return an
 * integer as result code. If the result >= 0, that means the number of
 * values leave on the stack, or the callback routine error, nothing leave on
 * the stack, -result is the error value returned by lua_pcall.
 *
 * When LUVF_CALLBACK_NOEXIT is set, the implementer should not exit.
 * When LUVF_CALLBACK_NOTRACEBACK is set, the implementer will not do traceback.
 *
 * Need to notice that the implementer must balance the lua vm stack, and maybe
 * exit when memory allocation error.
 */
typedef int (*luv_CFpcall)(lua_State *L, int nargs, int nresults, int ref_errfunc);

/* Default implementation of event callback */
LUV_LIBAPI int luv_docall(lua_State *L, int nargs, int nresult, int ref_errfunc);

/* The structure which luv expects for a loop. Can be implicitly cast between `luv_loop_t*` and `uv_loop_t*` */
typedef struct {
    /* The libuv loop, must be first to allow for the implicit cast to `uv_loop_t*` */
    uv_loop_t loop;

    /* The thread currently invoking uv_run, if any */
    lua_State *runner;

    /* The thread currently running, for immediately invoked callbacks */
    lua_State *current;

    /* The mode used to run the loop (-1 if not running) */
    int mode;

    /* A registry reference to a function to be used as a error handler for docall.
     * This function is not automatically forwarded to threads. */
    int errfunc;

    /* Called for invoking a callback on the main thread */
    luv_CFpcall docall;

    /* Called for invoking a function on the libuv threadpool.
     * This function must not call `exit()` under any circumstance.
     * Doing so will cause an abort in libuv's threadpool cleanup routine. */
    luv_CFpcall work_docall;

    /* Called for invoking a function on a secondary thread */
    luv_CFpcall thread_docall;
} luv_loop_t;

LUV_LIBAPI int luv_loop_init(luv_loop_t *loop);

/* Retrieve all the luv context from a lua_State */
LUV_LIBAPI luv_loop_t *luv_context(lua_State *L);

LUV_LIBAPI uv_loop_t *luv_loop(lua_State *L);

/* Set or clear an external uv_loop_t in a lua_State
   When using a custom/external loop, this must be called before luaopen_luv
   (otherwise luv will create and use its own loop)
*/
LUV_LIBAPI void luv_set_loop(lua_State *L, luv_loop_t *loop);

/* This is the main hook to load the library.
   This can be called multiple times in a process as long
   as you use a different lua_State and thread for each.
*/
LUV_LIBAPI int luaopen_luv(lua_State *L);

typedef lua_State *(*luv_acquire_vm)(void);
typedef void (*luv_release_vm)(lua_State *L);
LUV_LIBAPI void luv_set_thread_cb(luv_acquire_vm acquire, luv_release_vm release);

#endif
