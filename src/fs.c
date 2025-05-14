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

#include <lauxlib.h>
#include <lua.h>
#include <stdint.h>
#include <stdlib.h>
#include <uv.h>

#include "internal.h"
#include "luv.h"
#include "private.h"
#include "request.h"
#include "util.h"

#if LUV_UV_VERSION_GEQ(1, 28, 0)
typedef struct {
  uv_dir_t *handle;
  int ref; /* handle has been closed if this is LUA_NOREF */
} luv_dir_t;
#endif

typedef struct {
  uv_fs_t *req;
} luv_fs_scandir_t;

static uv_fs_t *luv_check_fs(lua_State *L, int index) {
  if (luaL_testudata(L, index, "uv_fs_scandir") != NULL) {
    luv_fs_scandir_t *scandir_req = (luv_fs_scandir_t *)lua_touserdata(L, index);
    return scandir_req->req;
  }
  uv_fs_t *req = (uv_fs_t *)luaL_checkudata(L, index, "uv_req");
  luaL_argcheck(L, req->type == UV_FS && req->data, index, "Expected uv_fs_t");
  return req;
}

LUV_LIBAPI int luv_fs_gc(lua_State *L) {
  uv_fs_t *req = luv_check_fs(L, 1);
  luv_req_t *lreq = luv_request_from(req);
  uv_fs_req_cleanup(req);
  luv_cleanup_req(L, lreq);
  return 0;
}

static void luv_push_timespec_table(lua_State *L, const uv_timespec_t *tmspec) {
  lua_createtable(L, 0, 2);
  lua_pushinteger(L, tmspec->tv_sec);
  lua_setfield(L, -2, "sec");
  lua_pushinteger(L, tmspec->tv_nsec);
  lua_setfield(L, -2, "nsec");
}

static void luv_push_stats_table(lua_State *L, const uv_stat_t *stat) {
  lua_createtable(L, 0, 23);
  lua_pushinteger(L, (lua_Integer)stat->st_dev);
  lua_setfield(L, -2, "dev");
  lua_pushinteger(L, (lua_Integer)stat->st_mode);
  lua_setfield(L, -2, "mode");
  lua_pushinteger(L, (lua_Integer)stat->st_nlink);
  lua_setfield(L, -2, "nlink");
  lua_pushinteger(L, (lua_Integer)stat->st_uid);
  lua_setfield(L, -2, "uid");
  lua_pushinteger(L, (lua_Integer)stat->st_gid);
  lua_setfield(L, -2, "gid");
  lua_pushinteger(L, (lua_Integer)stat->st_rdev);
  lua_setfield(L, -2, "rdev");
  lua_pushinteger(L, (lua_Integer)stat->st_ino);
  lua_setfield(L, -2, "ino");
  lua_pushinteger(L, (lua_Integer)stat->st_size);
  lua_setfield(L, -2, "size");
  lua_pushinteger(L, (lua_Integer)stat->st_blksize);
  lua_setfield(L, -2, "blksize");
  lua_pushinteger(L, (lua_Integer)stat->st_blocks);
  lua_setfield(L, -2, "blocks");
  lua_pushinteger(L, (lua_Integer)stat->st_flags);
  lua_setfield(L, -2, "flags");
  lua_pushinteger(L, (lua_Integer)stat->st_gen);
  lua_setfield(L, -2, "gen");
  luv_push_timespec_table(L, &stat->st_atim);
  lua_setfield(L, -2, "atime");
  luv_push_timespec_table(L, &stat->st_mtim);
  lua_setfield(L, -2, "mtime");
  luv_push_timespec_table(L, &stat->st_ctim);
  lua_setfield(L, -2, "ctime");
  luv_push_timespec_table(L, &stat->st_birthtim);
  lua_setfield(L, -2, "birthtime");

  const char *type = NULL;
  if (S_ISREG(stat->st_mode)) {
    type = "file";
  } else if (S_ISDIR(stat->st_mode)) {
    type = "directory";
  } else if (S_ISLNK(stat->st_mode)) {
    type = "link";
  } else if (S_ISFIFO(stat->st_mode)) {
    type = "fifo";
  }
#ifdef S_ISSOCK
  else if (S_ISSOCK(stat->st_mode)) {
    type = "socket";
  }
#endif
  else if (S_ISCHR(stat->st_mode)) {
    type = "char";
  } else if (S_ISBLK(stat->st_mode)) {
    type = "block";
  }

  if (type) {
    lua_pushstring(L, type);
    lua_setfield(L, -2, "type");
  }
}

static void luv_push_dirent_type(lua_State *L, uv_dirent_type_t typ) {
  switch (typ) {
    case UV_DIRENT_FILE:
      lua_pushstring(L, "file");
      break;
    case UV_DIRENT_DIR:
      lua_pushstring(L, "directory");
      break;
    case UV_DIRENT_LINK:
      lua_pushstring(L, "link");
      break;
    case UV_DIRENT_FIFO:
      lua_pushstring(L, "fifo");
      break;
    case UV_DIRENT_SOCKET:
      lua_pushstring(L, "socket");
      break;
    case UV_DIRENT_CHAR:
      lua_pushstring(L, "char");
      break;
    case UV_DIRENT_BLOCK:
      lua_pushstring(L, "block");
      break;
    case UV_DIRENT_UNKNOWN:
    default:
      lua_pushstring(L, "unknown");
      break;
  }
}

static int luv_push_dirent(lua_State *L, const uv_dirent_t *ent, int table) {
  const char *type;
  if (table) {
    lua_newtable(L);
  }
  lua_pushstring(L, ent->name);
  if (table) {
    lua_setfield(L, -2, "name");
  }
  switch (ent->type) {
    case UV_DIRENT_UNKNOWN:
      return 1;
    case UV_DIRENT_FILE:
      type = "file";
      break;
    case UV_DIRENT_DIR:
      type = "directory";
      break;
    case UV_DIRENT_LINK:
      type = "link";
      break;
    case UV_DIRENT_FIFO:
      type = "fifo";
      break;
    case UV_DIRENT_SOCKET:
      type = "socket";
      break;
    case UV_DIRENT_CHAR:
      type = "char";
      break;
    case UV_DIRENT_BLOCK:
      type = "block";
      break;
    default:
      type = "unknown";
      break;
  }
  lua_pushstring(L, type);
  if (table) lua_setfield(L, -2, "type");

  return table ? 1 : 2;
}

LUV_LIBAPI int luv_check_oflags(lua_State *const L, const int index) {
  if (lua_isnumber(L, index)) {
    return lua_tointeger(L, index);
  }

  if (!lua_isstring(L, index)) {
    luv_typeerror(L, index, "string or integer");
  }

  const char *const string = lua_tostring(L, index);
  int read_write = ' ';
  int flags = 0;

  for (const char *chr = string; *chr != '\0'; chr++) {
    switch (*chr) {
      case 'r':
      case 'R':
        if (read_write != ' ') {
          luv_argerror(L, index, "mode '%c' is mutually exclusive with '%c'", *chr, read_write);
        }

        read_write = 'r';
        break;
      case 'w':
      case 'W':
        if (read_write != ' ') {
          luv_argerror(L, index, "mode '%c' is mutually exclusive with '%c'", *chr, read_write);
        }

        read_write = 'w';
        flags |= O_TRUNC | O_CREAT;
        break;
      case 'a':
      case 'A':
        if (read_write != ' ') {
          luv_argerror(L, index, "mode '%c' is mutually exclusive with '%c'", *chr, read_write);
        }

        read_write = 'a';
        flags |= O_APPEND | O_CREAT;
        break;
      case '+':
        if (read_write == ' ') {
          luv_argerror(L, index, "mode '%c' must be specified after 'r', 'w', or 'a'", *chr);
        }

        read_write = '+';
        break;
#ifdef O_SYNC
      case 's':
      case 'S':
        if (read_write == ' ' || read_write == 'r') {
          luv_argerror(L, index, "mode '%c' must be specified after 'w', 'a' or 'r+'", *chr);
        }

        flags |= O_SYNC;
        break;
#endif
#ifdef O_DIRECT
      case 'd':
      case 'D':
        if (read_write == ' ' || read_write == 'r') {
          luv_argerror(L, index, "mode '%c' must be specified after 'w', 'a' or 'r+'", *chr);
        }

        flags |= O_DIRECT;
        break;
#endif
      case 'x':
      case 'X':
        if ((flags & O_CREAT) == 0) {
          luv_argerror(L, index, "mode '%c' must be specified after 'w' or 'a'", *chr);
        }

        flags |= O_EXCL;
        break;
      default:
        luv_argerror(L, index, "unknown file open flag '%c'", *chr);
    }
  }

  if (read_write == ' ') {
    luv_argerror(L, index, "mode must include 'r', 'w', or 'a'");
  } else if (read_write == '+') {
    flags |= O_RDWR;
  } else if (read_write == 'r') {
    flags |= O_RDONLY;
  } else if (read_write == 'w' || read_write == 'a') {
    flags |= O_WRONLY;
  }

  return flags;
}

LUV_LIBAPI int luv_check_amode(lua_State *const L, const int index) {
  if (lua_isnumber(L, index)) {
    return lua_tointeger(L, index);
  }

  if (!lua_isstring(L, index)) {
    luv_typeerror(L, index, "string or integer");
  }

  const char *const string = lua_tostring(L, index);
  int mode = 0;

  for (const char *chr = string; *chr != '\0'; chr++) {
    switch (*chr) {
      case 'r':
      case 'R':
        mode |= R_OK;
        break;
      case 'w':
      case 'W':
        mode |= W_OK;
        break;
      case 'x':
      case 'X':
        mode |= X_OK;
        break;
      default:
        luv_argerror(L, index, "unknown access mode '%c'", *chr);
    }
  }

  return mode;
}

#if LUV_UV_VERSION_GEQ(1, 31, 0)
static void luv_push_statfs_table(lua_State *L, const uv_statfs_t *stat) {
  lua_createtable(L, 0, 8);
  lua_pushinteger(L, (lua_Integer)stat->f_type);
  lua_setfield(L, -2, "type");
  lua_pushinteger(L, (lua_Integer)stat->f_bsize);
  lua_setfield(L, -2, "bsize");
  lua_pushinteger(L, (lua_Integer)stat->f_blocks);
  lua_setfield(L, -2, "blocks");
  lua_pushinteger(L, (lua_Integer)stat->f_bfree);
  lua_setfield(L, -2, "bfree");
  lua_pushinteger(L, (lua_Integer)stat->f_bavail);
  lua_setfield(L, -2, "bavail");
  lua_pushinteger(L, (lua_Integer)stat->f_files);
  lua_setfield(L, -2, "files");
  lua_pushinteger(L, (lua_Integer)stat->f_ffree);
  lua_setfield(L, -2, "ffree");
};
#endif

static int fs_req_has_dest_path(uv_fs_t *req) {
  switch (req->fs_type) {
    case UV_FS_RENAME:
    case UV_FS_SYMLINK:
    case UV_FS_LINK:
#if LUV_UV_VERSION_GEQ(1, 14, 0)
    case UV_FS_COPYFILE:
#endif
      return 1;
    default:
      return 0;
  }
}

/* Processes a result and pushes the data onto the stack
   returns the number of items pushed */
static int push_fs_result(lua_State *L, uv_fs_t *fs_req) {
  luv_req_t *const data = luv_request_from(fs_req);

  if (fs_req->fs_type == UV_FS_ACCESS) {
    lua_pushboolean(L, fs_req->result >= 0);
    return 1;
  }

  if (fs_req->result < 0) {
    if (fs_req->fs_type == UV_FS_SCANDIR) {
      // We need to unref the luv_fs_scandir_t userdata to allow it to be garbage collected.
      // The scandir callback can only be called once, so we now know that the
      // req can be safely garbage collected.
      luaL_unref(L, LUA_REGISTRYINDEX, data->data);
      data->data = LUA_NOREF;
    }
    lua_pushnil(L);
    if (fs_req_has_dest_path(fs_req)) {
      lua_rawgeti(L, LUA_REGISTRYINDEX, data->data);
      const char *dest_path = lua_tostring(L, -1);
      lua_pop(L, 1);
      lua_pushfstring(
        L,
        "%s: %s: %s -> %s",
        uv_err_name((int)fs_req->result),
        uv_strerror((int)fs_req->result),
        fs_req->path,
        dest_path
      );
    } else if (fs_req->path) {
      lua_pushfstring(
        L, "%s: %s: %s", uv_err_name((int)fs_req->result), uv_strerror((int)fs_req->result), fs_req->path
      );
    } else {
      lua_pushfstring(L, "%s: %s", uv_err_name((int)fs_req->result), uv_strerror((int)fs_req->result));
    }
    return 2;
  }

  switch (fs_req->fs_type) {
    case UV_FS_CLOSE:
    case UV_FS_RENAME:
    case UV_FS_UNLINK:
    case UV_FS_RMDIR:
    case UV_FS_MKDIR:
    case UV_FS_FTRUNCATE:
    case UV_FS_FSYNC:
    case UV_FS_FDATASYNC:
    case UV_FS_LINK:
    case UV_FS_SYMLINK:
    case UV_FS_CHMOD:
    case UV_FS_FCHMOD:
    case UV_FS_CHOWN:
    case UV_FS_FCHOWN:
    case UV_FS_UTIME:
    case UV_FS_FUTIME:
#if LUV_UV_VERSION_GEQ(1, 14, 0)
    case UV_FS_COPYFILE:
#endif
#if LUV_UV_VERSION_GEQ(1, 21, 0)
    case UV_FS_LCHOWN:
#endif
#if LUV_UV_VERSION_GEQ(1, 36, 0)
    case UV_FS_LUTIME:
#endif
      lua_pushboolean(L, 1);
      return 1;

    case UV_FS_OPEN:
    case UV_FS_SENDFILE:
    case UV_FS_WRITE:
      lua_pushinteger(L, fs_req->result);
      return 1;

    case UV_FS_STAT:
    case UV_FS_LSTAT:
    case UV_FS_FSTAT:
      luv_push_stats_table(L, &fs_req->statbuf);
      return 1;

#if LUV_UV_VERSION_GEQ(1, 31, 0)
    case UV_FS_STATFS:
      luv_push_statfs_table(L, fs_req->ptr);
      return 1;
#endif

    case UV_FS_MKDTEMP:
      lua_pushstring(L, fs_req->path);
      return 1;
#if LUV_UV_VERSION_GEQ(1, 34, 0)
    case UV_FS_MKSTEMP:
      lua_pushinteger(L, fs_req->result);
      lua_pushstring(L, fs_req->path);
      return 2;
#endif

    case UV_FS_READLINK:
#if LUV_UV_VERSION_GEQ(1, 8, 0)
    case UV_FS_REALPATH:
#endif
      lua_pushstring(L, (const char *)fs_req->ptr);
      return 1;

    case UV_FS_READ:
      lua_pushlstring(L, (const char *)fs_req->data, fs_req->result);
      return 1;

    case UV_FS_SCANDIR:
      // The luv_fs_scandir_t userdata is stored in data_ref.
      // We want to return this instead of the uv_req_t because the
      // luv_fs_scandir_t userdata has a gc method.
      lua_rawgeti(L, LUA_REGISTRYINDEX, data->data);
      // We now want to unref the userdata to allow it to be garbage collected.
      // The scandir callback can only be called once, so we now know that the
      // req can be safely garbage collected.
      luaL_unref(L, LUA_REGISTRYINDEX, data->data);
      data->data = LUA_NOREF;
      return 1;

#if LUV_UV_VERSION_GEQ(1, 28, 0)
    case UV_FS_OPENDIR: {
      int nentries;
      uv_dir_t *dir = (uv_dir_t *)fs_req->ptr;

      lua_rawgeti(L, LUA_REGISTRYINDEX, data->data);
      nentries = (int)luaL_checkinteger(L, -1);
      lua_pop(L, 1);
      luaL_unref(L, LUA_REGISTRYINDEX, data->data);
      data->data = LUA_NOREF;

      luv_dir_t *luv_dir = lua_newuserdata(L, sizeof(*luv_dir));
      luaL_getmetatable(L, "uv_dir");
      lua_setmetatable(L, -2);

      luv_dir->handle = dir;
      luv_dir->handle->dirents = lua_newuserdata(L, sizeof(uv_dirent_t) * nentries);
      luv_dir->ref = luaL_ref(L, LUA_REGISTRYINDEX);
      luv_dir->handle->nentries = nentries;

      return 1;
    }
    case UV_FS_READDIR: {
      luaL_unref(L, LUA_REGISTRYINDEX, data->data);
      data->data = LUA_NOREF;

      if (fs_req->result > 0) {
        uv_dir_t *dir = (uv_dir_t *)fs_req->ptr;
        lua_newtable(L);
        for (size_t i = 0; i < fs_req->result; i++) {
          luv_push_dirent(L, dir->dirents + i, 1);
          lua_rawseti(L, -2, (lua_Integer)i + 1);
        }
      } else {
        lua_pushnil(L);
      }

      return 1;
    }
    case UV_FS_CLOSEDIR:
      lua_pushboolean(L, 1);
      return 1;
#endif

    default:
      lua_pushnil(L);
      lua_pushfstring(L, "UNKNOWN FS TYPE %d\n", fs_req->fs_type);
      return 2;
  }
}

static void luv_fs_cb(uv_fs_t *fs_req) {
  luv_req_t *lreq = luv_request_from(fs_req);
  lua_State *L = lreq->ctx->L;

  int nargs = push_fs_result(L, fs_req);
  if (nargs == 2 && lua_isnil(L, -nargs)) {
    // If it was an error, convert to (err, value) format.
    lua_remove(L, -nargs);
    nargs--;
  } else {
    // Otherwise insert a nil in front to convert to (err, value) format.
    lua_pushnil(L);
    lua_insert(L, -nargs - 1);
    nargs++;
  }

  if (fs_req->fs_type == UV_FS_SCANDIR) {
    luv_fulfill_req(L, lreq, nargs);
  } else {
    // cleanup the uv_fs_t before the callback is called to avoid
    // a race condition when fs_close is called from within
    // a fs_readdir callback, see https://github.com/luvit/luv/issues/384
    uv_fs_req_cleanup(fs_req);
    luv_fulfill_req(L, lreq, nargs);
    luv_cleanup_req(L, lreq);
  }
}

LUV_LIBAPI int luv_fs_request_failure(lua_State *const L, luv_req_t *const lreq, const int ret) {
  luv_assert(L, ret < 0);

  // remove the metatable from the request userdata to avoid garbage collection
  lua_pushnil(L);
  lua_setmetatable(L, -2);

  uv_fs_t *const fs_req = luv_request_of(uv_fs_t, lreq);

  lua_pushnil(L);
  if (lua_rawgeti(L, LUA_REGISTRYINDEX, lreq->data) == LUA_TSTRING) {
    const char *const dest_path = lua_tostring(L, -1);
    lua_pushfstring(L, "%s: %s: %s -> %s", uv_err_name(ret), uv_strerror(ret), fs_req->path, dest_path);
  } else if (fs_req->path) {
    lua_pushfstring(L, "%s: %s: %s", uv_err_name(ret), uv_strerror(ret), fs_req->path);
  } else {
    lua_pushfstring(L, "%s: %s", uv_err_name(ret), uv_strerror(ret));
  }
  lua_pop(L, 1);

  lua_pushstring(L, uv_err_name(ret));

  uv_fs_req_cleanup(fs_req);
  luv_cleanup_req(L, lreq);

  return 3;
}

LUV_LIBAPI void luv_fs_request_cleanup(lua_State *const L, luv_req_t *const lreq, const int ud_index) {
  // remove the metatable from the request userdata to avoid garbage collection
  lua_pushvalue(L, ud_index);
  lua_pushnil(L);
  lua_setmetatable(L, -2);
  lua_pop(L, 1);

  uv_fs_t *const fs_req = luv_request_of(uv_fs_t, lreq);

  uv_fs_req_cleanup(fs_req);
  luv_cleanup_req(L, lreq);
}

/* create a new fs request */
#define FS_ENTER(L, cbidx)                                         \
  luv_ctx_t *const ctx = luv_context(L);                           \
  luv_req_t *const lreq = luv_new_request(L, UV_FS, ctx, (cbidx)); \
  uv_fs_t *const fs_req = luv_request_of(uv_fs_t, lreq);           \
  const int synchronous = lreq->callback == LUA_NOREF;             \
  const uv_fs_cb callback = synchronous ? NULL : luv_fs_cb;        \
  const int ud_index = lua_gettop(L);

/* handle any error and then either return the fs request userdata or the result and clean up the request */
#define FS_LEAVE(L)                              \
  if (ret < 0) {                                 \
    return luv_fs_request_failure(L, lreq, ret); \
  }                                              \
  if (synchronous) {                             \
    const int nargs = push_fs_result(L, fs_req); \
    luv_fs_request_cleanup(L, lreq, ud_index);   \
    return nargs;                                \
  }                                              \
  return 1;

/* either return the fs request userdata or the result and clean up the request */
#define FS_LEAVE_NOERROR(L)                      \
  if (synchronous) {                             \
    const int nargs = push_fs_result(L, fs_req); \
    luv_fs_request_cleanup(L, lreq, ud_index);   \
    return nargs;                                \
  }                                              \
  return 1;

LUV_LUAAPI int luv_fs_close(lua_State *const L) {
  const uv_file file = (uv_file)luaL_checkinteger(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_close(ctx->loop, fs_req, file, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_open(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int flags = luv_check_oflags(L, 2);
  int mode = (int)luaL_checkinteger(L, 3);
  FS_ENTER(L, 4);
  const int ret = uv_fs_open(ctx->loop, fs_req, path, flags, mode, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_read(lua_State *L) {
  uv_file file = (uv_file)luaL_checkinteger(L, 1);
  int64_t len = (int64_t)luaL_checkinteger(L, 2);
  int64_t offset = -1;

  // offset is optional
  if (lua_isnumber(L, 3)) {
    offset = (int64_t)luaL_checkinteger(L, 3);
  }
  FS_ENTER(L, 3 + lua_isnumber(L, 3));

  uv_buf_t buf;
  buf.base = (char *)malloc(len);
  buf.len = len;

  if (buf.base == NULL) {
    luv_error(L, "out of memory");
  }

  fs_req->data = buf.base;
  const int ret = uv_fs_read(ctx->loop, fs_req, file, &buf, 1, offset, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_unlink(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_unlink(ctx->loop, fs_req, path, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_write(lua_State *L) {
  uv_file file = (uv_file)luaL_checkinteger(L, 1);
  int64_t offset = -1;

  // offset is optional
  if (lua_isnumber(L, 3)) {
    offset = (int64_t)luaL_checkinteger(L, 3);
  }
  FS_ENTER(L, 3 + lua_isnumber(L, 3));

  luv_bufs_t bufs;
  luv_request_bufs(L, lreq, &bufs, 2);

  const int ret = uv_fs_write(ctx->loop, fs_req, file, bufs.bufs, bufs.bufs_count, offset, callback);
  luv_request_bufs_free(&bufs);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_mkdir(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int mode = (int)luaL_checkinteger(L, 2);
  FS_ENTER(L, 3);
  const int ret = uv_fs_mkdir(ctx->loop, fs_req, path, mode, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_mkdtemp(lua_State *L) {
  const char *tpl = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_mkdtemp(ctx->loop, fs_req, tpl, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_rmdir(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_rmdir(ctx->loop, fs_req, path, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_scandir(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);

  // store a reference to the userdata so that it cannot be garbage collected before the callback is called
  // this reference must be unref'd in the callback to prevent a reference loop that would prevent the userdata from
  // being garbage collected. this is only necessary for asynchronous calls.
  if (!synchronous) {
    lua_pushvalue(L, ud_index);
    lreq->data = luaL_ref(L, LUA_REGISTRYINDEX);
  }

  const int ret = uv_fs_scandir(ctx->loop, fs_req, path, 0, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_scandir_next(lua_State *L) {
  uv_dirent_t ent;
  uv_fs_t *fs_req = luv_check_fs(L, 1);
  const int ret = uv_fs_scandir_next(fs_req, &ent);
  if (ret == UV_EOF) {
    return 0;
  }

  if (ret < 0) {
    return luv_pushfail(L, ret);
  }

  lua_pushstring(L, ent.name);
  luv_push_dirent_type(L, ent.type);
  return 2;
}

LUV_LUAAPI int luv_fs_stat(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_stat(ctx->loop, fs_req, path, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_fstat(lua_State *L) {
  uv_file file = (uv_file)luaL_checkinteger(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_fstat(ctx->loop, fs_req, file, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_lstat(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_lstat(ctx->loop, fs_req, path, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_rename(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  const char *new_path = luaL_checkstring(L, 2);
  FS_ENTER(L, 3);

  // store the dest path so we can use it in the error message
  lua_pushvalue(L, 2);
  lreq->data = luaL_ref(L, LUA_REGISTRYINDEX);

  const int ret = uv_fs_rename(ctx->loop, fs_req, path, new_path, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_fsync(lua_State *L) {
  uv_file file = (uv_file)luaL_checkinteger(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_fsync(ctx->loop, fs_req, file, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_fdatasync(lua_State *L) {
  uv_file file = (uv_file)luaL_checkinteger(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_fdatasync(ctx->loop, fs_req, file, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_ftruncate(lua_State *L) {
  uv_file file = (uv_file)luaL_checkinteger(L, 1);
  int64_t offset = (int64_t)luaL_checkinteger(L, 2);
  FS_ENTER(L, 3);
  const int ret = uv_fs_ftruncate(ctx->loop, fs_req, file, offset, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_sendfile(lua_State *L) {
  uv_file out_fd = (uv_file)luaL_checkinteger(L, 1);
  uv_file in_fd = (uv_file)luaL_checkinteger(L, 2);
  int64_t in_offset = (int64_t)luaL_checkinteger(L, 3);
  size_t length = (size_t)luaL_checkinteger(L, 4);
  FS_ENTER(L, 5);
  const int ret = uv_fs_sendfile(ctx->loop, fs_req, out_fd, in_fd, in_offset, length, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_access(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int amode = luv_check_amode(L, 2);
  FS_ENTER(L, 3);
  const int ret = uv_fs_access(ctx->loop, fs_req, path, amode, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_chmod(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  int mode = (int)luaL_checkinteger(L, 2);
  FS_ENTER(L, 3);
  const int ret = uv_fs_chmod(ctx->loop, fs_req, path, mode, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_fchmod(lua_State *L) {
  uv_file file = (uv_file)luaL_checkinteger(L, 1);
  int mode = (int)luaL_checkinteger(L, 2);
  FS_ENTER(L, 3);
  const int ret = uv_fs_fchmod(ctx->loop, fs_req, file, mode, callback);
  FS_LEAVE(L);
}

LUV_LIBAPI double luv_fs_check_ftime(lua_State *L, int index) {
#if LUV_UV_VERSION_GEQ(1, 51, 0)
  if (lua_isnoneornil(L, index)) {
    return UV_FS_UTIME_OMIT;
  }

  if (lua_isnumber(L, index)) {
    return lua_tonumber(L, index);
  }

  const char *const special_value_strings[] = {"now", "omit", NULL};
  const double special_values[] = {UV_FS_UTIME_NOW, UV_FS_UTIME_OMIT};
  const int special_value_index = luaL_checkoption(L, index, NULL, special_value_strings);
  return special_values[special_value_index];
#else
  return luaL_checknumber(L, index);
#endif
}

LUV_LUAAPI int luv_fs_utime(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  double atime = luv_fs_check_ftime(L, 2);
  double mtime = luv_fs_check_ftime(L, 3);
  FS_ENTER(L, 4);
  const int ret = uv_fs_utime(ctx->loop, fs_req, path, atime, mtime, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_futime(lua_State *L) {
  uv_file file = (uv_file)luaL_checkinteger(L, 1);
  double atime = luv_fs_check_ftime(L, 2);
  double mtime = luv_fs_check_ftime(L, 3);
  FS_ENTER(L, 4);
  const int ret = uv_fs_futime(ctx->loop, fs_req, file, atime, mtime, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_link(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  const char *new_path = luaL_checkstring(L, 2);
  FS_ENTER(L, 3);

  // store the dest path so we can use it in the error message
  lua_pushvalue(L, 2);
  lreq->data = luaL_ref(L, LUA_REGISTRYINDEX);

  const int ret = uv_fs_link(ctx->loop, fs_req, path, new_path, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_symlink(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  const char *new_path = luaL_checkstring(L, 2);

  int flags = 0;
  if (lua_istable(L, 3)) {
    lua_getfield(L, 3, "dir");
    if (lua_toboolean(L, -1)) {
      flags |= UV_FS_SYMLINK_DIR;
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "junction");
    if (lua_toboolean(L, -1)) {
      flags |= UV_FS_SYMLINK_JUNCTION;
    }
    lua_pop(L, 1);
  } else if (lua_isnumber(L, 3)) {
    flags = lua_tointeger(L, 3);
  } else if (!luv_is_callable(L, 3) && !lua_isnoneornil(L, 3)) {
    luv_typeerror(L, 3, "callable or table or number or nil");
  }

  FS_ENTER(L, 4 - luv_is_callable(L, 3));

  // store the dest path so we can use it in the error message
  lua_pushvalue(L, 2);
  lreq->data = luaL_ref(L, LUA_REGISTRYINDEX);

  const int ret = uv_fs_symlink(ctx->loop, fs_req, path, new_path, flags, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_readlink(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_readlink(ctx->loop, fs_req, path, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_chown(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  uv_uid_t uid = (uv_uid_t)luaL_checkinteger(L, 2);
  uv_uid_t gid = (uv_uid_t)luaL_checkinteger(L, 3);
  FS_ENTER(L, 4);
  const int ret = uv_fs_chown(ctx->loop, fs_req, path, uid, gid, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_fchown(lua_State *L) {
  uv_file file = (uv_file)luaL_checkinteger(L, 1);
  uv_uid_t uid = (uv_uid_t)luaL_checkinteger(L, 2);
  uv_uid_t gid = (uv_uid_t)luaL_checkinteger(L, 3);
  FS_ENTER(L, 4);
  const int ret = uv_fs_fchown(ctx->loop, fs_req, file, uid, gid, callback);
  FS_LEAVE(L);
}

#if LUV_UV_VERSION_GEQ(1, 8, 0)
LUV_LUAAPI int luv_fs_realpath(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_realpath(ctx->loop, fs_req, path, callback);
  FS_LEAVE(L);
}
#endif

#if LUV_UV_VERSION_GEQ(1, 14, 0)
LUV_LUAAPI int luv_fs_copyfile(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  const char *new_path = luaL_checkstring(L, 2);

  int flags = 0;
  if (lua_istable(L, 3)) {
    lua_getfield(L, 3, "excl");
    if (lua_toboolean(L, -1)) {
      flags |= UV_FS_COPYFILE_EXCL;
    }
    lua_pop(L, 1);

#if LUV_UV_VERSION_GEQ(1, 20, 0)
    lua_getfield(L, 3, "ficlone");
    if (lua_toboolean(L, -1)) {
      flags |= UV_FS_COPYFILE_FICLONE;
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "ficlone_force");
    if (lua_toboolean(L, -1)) {
      flags |= UV_FS_COPYFILE_FICLONE_FORCE;
    }
    lua_pop(L, 1);
#endif

  } else if (lua_isnumber(L, 3)) {
    flags = lua_tointeger(L, 3);
  } else if (!luv_is_callable(L, 3) && !lua_isnoneornil(L, 3)) {
    luv_typeerror(L, 3, "callable or table or number or nil");
  }

  FS_ENTER(L, 4 - luv_is_callable(L, 3));

  // store the dest path so we can use it in the error message
  lua_pushvalue(L, 2);
  lreq->data = luaL_ref(L, LUA_REGISTRYINDEX);

  const int ret = uv_fs_copyfile(ctx->loop, fs_req, path, new_path, flags, callback);
  FS_LEAVE(L);
}
#endif

#if LUV_UV_VERSION_GEQ(1, 21, 0)
LUV_LUAAPI int luv_fs_lchown(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  uv_uid_t uid = (uv_uid_t)luaL_checkinteger(L, 2);
  uv_uid_t gid = (uv_uid_t)luaL_checkinteger(L, 3);
  FS_ENTER(L, 4);
  const int ret = uv_fs_lchown(ctx->loop, fs_req, path, uid, gid, callback);
  FS_LEAVE(L);
}
#endif

#if LUV_UV_VERSION_GEQ(1, 28, 0)
LUV_LUAAPI luv_dir_t *luv_check_dir(lua_State *L, int idx) {
  luv_dir_t *dir = (luv_dir_t *)luaL_checkudata(L, idx, "uv_dir");
  return dir;
}

LUV_LUAAPI int luv_fs_opendir(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  lua_Integer nentries = luaL_optinteger(L, 3, 1);
  FS_ENTER(L, 2);

  // store nentries in the req
  lua_pushinteger(L, nentries);
  lreq->data = luaL_ref(L, LUA_REGISTRYINDEX);

  const int ret = uv_fs_opendir(ctx->loop, fs_req, path, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_readdir(lua_State *L) {
  luv_dir_t *dir = luv_check_dir(L, 1);
  FS_ENTER(L, 2);

  // store the dirent userdata to ensure it doesn't get garbage collected
  lua_pushvalue(L, 1);
  lreq->data = luaL_ref(L, LUA_REGISTRYINDEX);

  const int ret = uv_fs_readdir(ctx->loop, fs_req, dir->handle, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_closedir(lua_State *L) {
  luv_dir_t *dir = luv_check_dir(L, 1);
  FS_ENTER(L, 2);

  luaL_unref(L, LUA_REGISTRYINDEX, dir->ref);
  dir->ref = LUA_NOREF;

  const int ret = uv_fs_closedir(ctx->loop, fs_req, dir->handle, callback);
  FS_LEAVE(L);
}

LUV_LUAAPI int luv_fs_dir_tostring(lua_State *L) {
  luv_dir_t *dir = luv_check_dir(L, 1);
  lua_pushfstring(L, "uv_dir: %p", dir);
  return 1;
}

LUV_LUAAPI int luv_fs_dir_gc(lua_State *L) {
  luv_dir_t *dir = luv_check_dir(L, 1);
  if (dir->ref == LUA_NOREF) {
    return 0;
  }

  uv_fs_t req;
  luv_ctx_t *ctx = luv_context(L);

  luaL_unref(L, LUA_REGISTRYINDEX, dir->ref);
  dir->ref = LUA_NOREF;

  uv_fs_closedir(ctx->loop, &req, dir->handle, NULL);
  uv_fs_req_cleanup(&req);
  return 0;
}
#endif

#if LUV_UV_VERSION_GEQ(1, 31, 0)
LUV_LUAAPI int luv_fs_statfs(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_statfs(ctx->loop, fs_req, path, callback);
  FS_LEAVE(L);
}
#endif

#if LUV_UV_VERSION_GEQ(1, 34, 0)
LUV_LUAAPI int luv_fs_mkstemp(lua_State *L) {
  const char *tpl = luaL_checkstring(L, 1);
  FS_ENTER(L, 2);
  const int ret = uv_fs_mkstemp(ctx->loop, fs_req, tpl, callback);
  FS_LEAVE(L);
}
#endif

#if LUV_UV_VERSION_GEQ(1, 36, 0)
LUV_LUAAPI int luv_fs_lutime(lua_State *L) {
  const char *path = luaL_checkstring(L, 1);
  double atime = luv_fs_check_ftime(L, 2);
  double mtime = luv_fs_check_ftime(L, 3);
  FS_ENTER(L, 4);
  const int ret = uv_fs_lutime(ctx->loop, fs_req, path, atime, mtime, callback);
  FS_LEAVE(L);
}
#endif
