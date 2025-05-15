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

#include "fs_event.h"

#include <lauxlib.h>
#include <lua.h>
#include <stddef.h>
#include <uv.h>

#include "handle.h"
#include "internal.h"
#include "luv.h"

LUV_DEFAPI luaL_Reg luv_fs_event_methods[] = {
  {"start", luv_fs_event_start},
  {"stop", luv_fs_event_stop},
  {"getpath", luv_fs_event_getpath},
  {NULL, NULL},
};

LUV_DEFAPI luaL_Reg luv_fs_event_functions[] = {
  {"new_fs_event", luv_new_fs_event},
  {"fs_event_start", luv_fs_event_start},
  {"fs_event_stop", luv_fs_event_stop},
  {"fs_event_getpath", luv_fs_event_getpath},
  {NULL, NULL},
};

LUV_LIBAPI uv_fs_event_t *luv_check_fs_event(lua_State *const L, const int index) {
  const luv_handle_t *const lhandle = (const luv_handle_t *)luv_checkuserdata(L, index, "uv_fs_event");
  uv_fs_event_t *const fs_event = luv_handle_of(uv_fs_event_t, lhandle);

  luaL_argcheck(L, fs_event->type == UV_FS_EVENT, index, "expected uv_fs_event handle");
  return fs_event;
}

LUV_LUAAPI int luv_new_fs_event(lua_State *const L) {
  const luv_ctx_t *const ctx = luv_context(L);

  luv_handle_t *const lhandle = luv_new_handle(L, UV_FS_EVENT, ctx, 0);
  uv_fs_event_t *const fs_event = luv_handle_of(uv_fs_event_t, lhandle);

  const int ret = uv_fs_event_init(ctx->loop, fs_event);
  if (ret < 0) {
    lua_pop(L, 1);
    return luv_pushfail(L, ret);
  }

  return 1;
}

LUV_CBAPI void luv_fs_event_cb(uv_fs_event_t *fs_event, const char *filename, int events, int status) {
  luv_handle_t *const lhandle = luv_handle_from(fs_event);
  lua_State *L = lhandle->ctx->L;

  // err
  luv_pushstatus(L, status);

  // filename
  lua_pushstring(L, filename);

  // events
  lua_createtable(L, 0, 2);
  lua_pushboolean(L, (events & UV_RENAME) != 0);
  lua_setfield(L, -2, "rename");
  lua_pushboolean(L, (events & UV_CHANGE) != 0);
  lua_setfield(L, -2, "change");

  luv_callback_send(L, LUV_CB_EVENT, lhandle, 3);
}

LUV_LUAAPI int luv_fs_event_start(lua_State *const L) {
  uv_fs_event_t *const fs_event = luv_check_fs_event(L, 1);
  luv_handle_t *const lhandle = luv_handle_from(fs_event);

  const char *path = luaL_checkstring(L, 2);
  int flags = 0;
  if (!lua_isnoneornil(L, 3)) {
    luaL_checktype(L, 3, LUA_TTABLE);

    lua_getfield(L, 3, "watch_entry");
    if (lua_toboolean(L, -1) != 0)
      flags |= UV_FS_EVENT_WATCH_ENTRY;

    lua_getfield(L, 3, "stat");
    if (lua_toboolean(L, -1) != 0)
      flags |= UV_FS_EVENT_STAT;

    lua_getfield(L, 3, "recursive");
    if (lua_toboolean(L, -1) != 0)
      flags |= UV_FS_EVENT_RECURSIVE;

    lua_pop(L, 3);
  }

  luv_callback_prep(L, LUV_CB_EVENT, lhandle, 4);

  const int ret = uv_fs_event_start(fs_event, luv_fs_event_cb, path, flags);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

LUV_LUAAPI int luv_fs_event_stop(lua_State *const L) {
  uv_fs_event_t *fs_event = luv_check_fs_event(L, 1);
  const int ret = uv_fs_event_stop(fs_event);
  return luv_pushresult(L, ret, LUA_TNUMBER);
}

LUV_LUAAPI int luv_fs_event_getpath(lua_State *const L) {
  uv_fs_event_t *fs_event = luv_check_fs_event(L, 1);

  size_t len = 2 * (size_t)PATH_MAX;
  char buf[2 * PATH_MAX];

  const int ret = uv_fs_event_getpath(fs_event, buf, &len);
  if (ret < 0)
    return luv_pushfail(L, ret);

  lua_pushlstring(L, buf, len);
  return 1;
}
