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
#include "fs_event.h"

#include "handle.h"
#include "internal.h"

static luaL_Reg luv_fs_event_methods[] = {
    {"start", luv_fs_event_start},
    {"stop", luv_fs_event_stop},
    {"getpath", luv_fs_event_getpath},
    {NULL, NULL},
};

LUV_LIBAPI void luv_fs_event_init(lua_State *L) {
    luv_handle_metatable(L, UV_FS_EVENT, luv_fs_event_methods);
    lua_pop(L, 1);
}

LUV_LIBAPI uv_fs_event_t *luv_check_fs_event(lua_State *L, int index) {
    luv_handle_t *lhandle = (luv_handle_t *)luaL_checkudata(L, index, "uv_fs_event");
    uv_fs_event_t *handle = luv_handle_of(uv_fs_event_t, lhandle);
    luv_typecheck(L, handle->type == UV_FS_EVENT, index, "uv_fs_event");
    return handle;
}

LUV_LUAAPI int luv_new_fs_event(lua_State *L) {
    uv_loop_t *loop = luv_loop(L);

    luv_handle_t *lhandle = luv_new_handle(L, UV_FS_EVENT);
    uv_fs_event_t *handle = luv_handle_of(uv_fs_event_t, lhandle);

    int ret = uv_fs_event_init(loop, handle);
    if (ret < 0)
        goto fail;
    return 1;

fail:
    lua_pop(L, 1);
    return luv_pushfail(L, ret);
}

static void luv_fs_event_cb(uv_fs_event_t *handle, const char *filename, int events, int status) {
    luv_handle_t *lhandle = luv_handle_from(handle);
    lua_State *L = luv_handle_state(lhandle);

    // err
    luv_pushstatus(L, status);

    // filename
    lua_pushstring(L, filename);

    // events
    lua_createtable(L, 0, 2);
    if (events & UV_RENAME) {
        lua_pushboolean(L, 1);
        lua_setfield(L, -2, "rename");
    }

    if (events & UV_CHANGE) {
        lua_pushboolean(L, 1);
        lua_setfield(L, -2, "change");
    }

    luv_callback_send(L, LUV_CB_EVENT, lhandle, 3);
}

LUV_LUAAPI int luv_fs_event_start(lua_State *L) {
    uv_fs_event_t *handle = luv_check_fs_event(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    const char *path = luaL_checkstring(L, 2);

    int flags = 0;
    if (lua_istable(L, 3)) {
        lua_getfield(L, 3, "recursive");
        if (lua_toboolean(L, -1))
            flags |= UV_FS_EVENT_RECURSIVE;
        lua_pop(L, 1);
    } else if (!lua_isnoneornil(L, 3)) {
        luv_typeerror(L, 3, "table or nil");
    }

    luv_callback_set(L, LUV_CB_EVENT, lhandle, 4);
    int ret = uv_fs_event_start(handle, luv_fs_event_cb, path, flags);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_fs_event_stop(lua_State *L) {
    uv_fs_event_t *handle = luv_check_fs_event(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    int ret = uv_fs_event_stop(handle);
    if (ret < 0)
        goto fail;

    return luv_pushsuccess(L, ret);

fail:
    return luv_pushfail(L, ret);
}

LUV_LUAAPI int luv_fs_event_getpath(lua_State *L) {
    uv_fs_event_t *handle = luv_check_fs_event(L, 1);

    char buf[2 * PATH_MAX];
    char *bufp = buf;
    size_t len = sizeof(buf);

    int ret = uv_fs_event_getpath(handle, bufp, &len);
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

    ret = uv_fs_event_getpath(handle, bufp, &len);
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
