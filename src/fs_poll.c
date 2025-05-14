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
#include "fs_poll.h"

#include <lauxlib.h>
#include <lua.h>
#include <stddef.h>
#include <uv.h>

#include "handle.h"
#include "luv.h"
#include "private.h"

LUV_DEFAPI luaL_Reg luv_fs_poll_methods[] = {
  {"start", luv_fs_poll_start},
  {"stop", luv_fs_poll_stop},
  {"getpath", luv_fs_poll_getpath},
  {NULL, NULL},
};

LUV_DEFAPI luaL_Reg luv_fs_poll_functions[] = {
  {"new_fs_poll", luv_new_fs_poll},
  {"fs_poll_start", luv_fs_poll_start},
  {"fs_poll_stop", luv_fs_poll_stop},
  {"fs_poll_getpath", luv_fs_poll_getpath},
  {NULL, NULL},
};

LUV_LIBAPI uv_fs_poll_t *luv_check_fs_poll(lua_State *const L, const int index) {
  const luv_handle_t *const lhandle = (const luv_handle_t *)luv_checkuserdata(L, index, "uv_fs_poll");
  uv_fs_poll_t *const fs_poll = luv_handle_of(uv_fs_poll_t, lhandle);

  luaL_argcheck(L, fs_poll->type == UV_FS_POLL, index, "expected uv_fs_poll handle");
  return fs_poll;
}

LUV_LUAAPI int luv_new_fs_poll(lua_State *L) {
  const luv_ctx_t *const ctx = luv_context(L);

  luv_handle_t *const lhandle = luv_new_handle(L, UV_FS_POLL, ctx, 0);
  uv_fs_poll_t *const fs_poll = luv_handle_of(uv_fs_poll_t, lhandle);

  const int ret = uv_fs_poll_init(ctx->loop, fs_poll);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_pushfail(L, ret);
  }

  return 1;
}

LUV_CBAPI void luv_fs_poll_cb(
  uv_fs_poll_t *const fs_poll,
  const int status,
  const uv_stat_t *const prev,
  const uv_stat_t *const curr
) {
  luv_handle_t *lhandle = luv_handle_from(fs_poll);
  lua_State *L = lhandle->ctx->L;

  // err
  luv_pushstatus(L, status);

  // prev
  if (prev != NULL) {
    luv_push_stats_table(L, prev);
  } else {
    lua_pushnil(L);
  }

  // curr
  if (curr != NULL) {
    luv_push_stats_table(L, curr);
  } else {
    lua_pushnil(L);
  }

  luv_callback_send(L, LUV_CB_EVENT, lhandle, 3);
}

LUV_LUAAPI int luv_fs_poll_start(lua_State *L) {
  uv_fs_poll_t *const fs_poll = luv_check_fs_poll(L, 1);
  luv_handle_t *const lhandle = luv_handle_from(fs_poll);

  const char *path = luaL_checkstring(L, 2);
  const unsigned int interval = (unsigned int)luaL_checkinteger(L, 3);

  luv_callback_prep(L, LUV_CB_EVENT, lhandle, 4);

  const int ret = uv_fs_poll_start(fs_poll, luv_fs_poll_cb, path, interval);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

LUV_LUAAPI int luv_fs_poll_stop(lua_State *L) {
  uv_fs_poll_t *handle = luv_check_fs_poll(L, 1);
  const int ret = uv_fs_poll_stop(handle);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

LUV_LUAAPI int luv_fs_poll_getpath(lua_State *const L) {
  uv_fs_poll_t *fs_poll = luv_check_fs_poll(L, 1);

  size_t len = 2 * PATH_MAX;
  char buf[2 * PATH_MAX];

  const int ret = uv_fs_poll_getpath(fs_poll, buf, &len);
  if (ret < 0) {
    return luv_pushfail(L, ret);
  }

  lua_pushlstring(L, buf, len);
  return 1;
}