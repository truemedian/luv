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

#ifndef LUV_FS_H
#define LUV_FS_H

#include <lauxlib.h>
#include <lua.h>
#include <stdint.h>
#include <stdlib.h>
#include <uv.h>

#include "internal.h"
#include "luv.h"
#include "request.h"

typedef struct {
  uv_fs_t *req;
} luv_fs_scandir_t;

/* ensure the userdata at the given index is a uv_fs request and return a pointer to it */
static uv_fs_t *luv_check_fs(lua_State *L, int index);

LUV_LIBAPI int luv_fs_gc(lua_State *L);

/* push a table representing the given timespec to the stack */
LUV_LIBAPI void luv_push_timespec_table(lua_State *L, const uv_timespec_t *tmspec);

/* push a table representing the given stat to the stack */
LUV_LIBAPI void luv_push_stats_table(lua_State *L, const uv_stat_t *stat);

/* push a string representing the given dirent type to the stack */
LUV_LIBAPI void luv_push_dirent_type(lua_State *L, uv_dirent_type_t typ);

#if LUV_UV_VERSION_GEQ(1, 31, 0)
/* push a table representing the given statfs to the stack */
LUV_LIBAPI void luv_push_statfs_table(lua_State *const L, const uv_statfs_t *const stat);
#endif

/* ensure the number or string at the given index is a valid open mode flag and return it */
LUV_LIBAPI int luv_check_oflags(lua_State *const L, const int index);

/* ensure the number or string at the given index is a valid access mode flag and return it */
LUV_LIBAPI int luv_check_amode(lua_State *const L, const int index);

/* ensure the number or string at the given index is a valid file time and return it */
LUV_LIBAPI double luv_fs_check_ftime(lua_State *L, int index);

LUV_LUAAPI int luv_fs_close(lua_State *const L);
LUV_LUAAPI int luv_fs_open(lua_State *L);
LUV_LUAAPI int luv_fs_read(lua_State *L);
LUV_LUAAPI int luv_fs_unlink(lua_State *L);
LUV_LUAAPI int luv_fs_write(lua_State *L);
LUV_LUAAPI int luv_fs_mkdir(lua_State *L);
LUV_LUAAPI int luv_fs_mkdtemp(lua_State *L);
LUV_LUAAPI int luv_fs_rmdir(lua_State *L);
LUV_LUAAPI int luv_fs_scandir(lua_State *L);
LUV_LUAAPI int luv_fs_scandir_next(lua_State *L);
LUV_LUAAPI int luv_fs_stat(lua_State *L);
LUV_LUAAPI int luv_fs_fstat(lua_State *L);
LUV_LUAAPI int luv_fs_lstat(lua_State *L);
LUV_LUAAPI int luv_fs_rename(lua_State *L);
LUV_LUAAPI int luv_fs_fsync(lua_State *L);
LUV_LUAAPI int luv_fs_fdatasync(lua_State *L);
LUV_LUAAPI int luv_fs_ftruncate(lua_State *L);
LUV_LUAAPI int luv_fs_sendfile(lua_State *L);
LUV_LUAAPI int luv_fs_access(lua_State *L);
LUV_LUAAPI int luv_fs_chmod(lua_State *L);
LUV_LUAAPI int luv_fs_fchmod(lua_State *L);
LUV_LUAAPI int luv_fs_utime(lua_State *L);
LUV_LUAAPI int luv_fs_futime(lua_State *L);
LUV_LUAAPI int luv_fs_symlink(lua_State *L);
LUV_LUAAPI int luv_fs_readlink(lua_State *L);
LUV_LUAAPI int luv_fs_chown(lua_State *L);
LUV_LUAAPI int luv_fs_fchown(lua_State *L);

#if LUV_UV_VERSION_GEQ(1, 8, 0)

LUV_LUAAPI int luv_fs_realpath(lua_State *L);

#endif
#if LUV_UV_VERSION_GEQ(1, 14, 0)

LUV_LUAAPI int luv_fs_copyfile(lua_State *L);

#endif
#if LUV_UV_VERSION_GEQ(1, 21, 0)

LUV_LUAAPI int luv_fs_lchown(lua_State *L);

#endif
#if LUV_UV_VERSION_GEQ(1, 28, 0)

typedef struct {
  uv_dir_t *handle;
  int ref; /* handle has been closed if this is LUA_NOREF */
} luv_dir_t;

/* ensure the userdata at the given index is a luv_dir and return a pointer to it */
LUV_LIBAPI luv_dir_t *luv_check_dir(lua_State *L, int idx);

LUV_LUAAPI int luv_fs_opendir(lua_State *L);
LUV_LUAAPI int luv_fs_readdir(lua_State *L);
LUV_LUAAPI int luv_fs_closedir(lua_State *L);
LUV_LUAAPI int luv_fs_dir_tostring(lua_State *L);
LUV_LUAAPI int luv_fs_dir_gc(lua_State *L);

#endif
#if LUV_UV_VERSION_GEQ(1, 31, 0)

LUV_LUAAPI int luv_fs_statfs(lua_State *L);

#endif
#if LUV_UV_VERSION_GEQ(1, 34, 0)

LUV_LUAAPI int luv_fs_mkstemp(lua_State *L);

#endif
#if LUV_UV_VERSION_GEQ(1, 36, 0)

LUV_LUAAPI int luv_fs_lutime(lua_State *L);

#endif
#endif