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
#include "fs_poll.h"

#include "handle.h"
#include "internal.h"

static luaL_Reg luv_fs_poll_methods[] = {
    {"start", luv_fs_poll_start},
    {"stop", luv_fs_poll_stop},
    {"getpath", luv_fs_poll_getpath},
    {NULL, NULL},
};

LUV_LIBAPI void luv_fs_poll_init(lua_State *L) {
    luv_handle_metatable(L, UV_FS_POLL, luv_fs_poll_methods);
    lua_pop(L, 1);
}

LUV_LIBAPI uv_fs_poll_t *luv_check_fs_poll(lua_State *L, int index) {
    luv_handle_t *lhandle = (luv_handle_t *)luaL_checkudata(L, index, "uv_fs_poll");
    uv_fs_poll_t *handle = luv_handle_of(uv_fs_poll_t, lhandle);
    luv_typecheck(L, handle->type == UV_FS_POLL, index, "uv_fs_poll");
    return handle;
}

LUV_LUAAPI int luv_new_fs_poll(lua_State *L) {
    uv_loop_t *loop = luv_loop(L);

    luv_handle_t *lhandle = luv_new_handle(L, UV_FS_POLL);
    uv_fs_poll_t *handle = luv_handle_of(uv_fs_poll_t, lhandle);

    int ret = uv_fs_poll_init(loop, handle);
    if (ret < 0)
        goto fail;

    return 1;

fail:
    lua_pop(L, 1);
    return luv_pushfail(L, ret);
}

static void luv_fs_poll_cb(uv_fs_poll_t *handle, int status, const uv_stat_t *prev, const uv_stat_t *curr) {
    luv_handle_t *lhandle = luv_handle_from(handle);
    lua_State *L = luv_handle_state(lhandle);

    // err
    luv_pushstatus(L, status);

    // prev
    if (prev) {
        luv_push_stats_table(L, prev);
    } else {
        lua_pushnil(L);
    }

    // curr
    if (curr) {
        luv_push_stats_table(L, curr);
    } else {
        lua_pushnil(L);
    }

    luv_callback_send(L, LUV_CB_EVENT, lhandle, 3);
}

LUV_LUAAPI int luv_fs_poll_start(lua_State *L) {
    uv_fs_poll_t *handle = luv_check_fs_poll(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    const char *path = luaL_checkstring(L, 2);
    lua_Integer interval = luaL_checkinteger(L, 3);
    luv_argcheck(L, interval > 0, 3, "interval must be positive");
    luv_argcheck(L, interval <= UINT_MAX, 3, "interval too large");

    luv_callback_set(L, LUV_CB_EVENT, lhandle, 4);
    int ret = uv_fs_poll_start(handle, luv_fs_poll_cb, path, (unsigned int)interval);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_fs_poll_stop(lua_State *L) {
    uv_fs_poll_t *handle = luv_check_fs_poll(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    int ret = uv_fs_poll_stop(handle);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_fs_poll_getpath(lua_State *L) {
    uv_fs_poll_t *handle = luv_check_fs_poll(L, 1);

    char buf[2 * PATH_MAX];
    char *bufp = buf;
    size_t len = sizeof(buf);

    int ret = uv_fs_poll_getpath(handle, bufp, &len);
    if (ret == ENOBUFS)
        goto retry;
    if (ret < 0)
        goto fail;

    lua_pushlstring(L, buf, len);
    return 1;

retry:
    // prepare in case malloc fails
    ret = UV_ENOMEM;

    bufp = malloc(len);
    if (bufp == NULL)
        goto fail;

    ret = uv_fs_poll_getpath(handle, bufp, &len);
    if (ret < 0)
        goto fail_retry;

    lua_pushlstring(L, bufp, len);
    free(bufp);
    return 1;

fail_retry:
    free(bufp);

fail:
    return luv_pushfail(L, ret);
}
