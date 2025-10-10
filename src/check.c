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
#include "check.h"

#include "handle.h"
#include "internal.h"
#include "luv.h"

static luaL_Reg luv_check_methods[] = {
    {"start", luv_check_start},
    {"stop", luv_check_stop},
    {NULL, NULL},
};

LUV_LIBAPI void luv_check_init(lua_State *L) {
    luv_handle_metatable(L, UV_CHECK, luv_check_methods);
    lua_pop(L, 1);
}

LUV_LIBAPI uv_check_t *luv_check_check(lua_State *L, int index) {
    luv_handle_t *lhandle = (luv_handle_t *)luaL_checkudata(L, index, "uv_check");
    uv_check_t *handle = luv_handle_of(uv_check_t, lhandle);
    luv_typecheck(L, handle->type == UV_CHECK, index, "uv_check");
    return handle;
}

/// @function new_check
/// @comment Creates and initializes a new {{uv_check}} handle.
/// @return handle uv_check
LUV_LUAAPI int luv_new_check(lua_State *L) {
    uv_loop_t *loop = luv_loop(L);

    luv_handle_t *lhandle = luv_new_handle(L, UV_CHECK);
    uv_check_t *handle = luv_handle_of(uv_check_t, lhandle);

    (void)uv_check_init(loop, handle);
    return 1;
}

static void luv_check_cb(uv_check_t *handle) {
    luv_handle_t *lhandle = luv_handle_from(handle);
    lua_State *L = luv_handle_state(lhandle);

    luv_callback_send(L, LUV_CB_EVENT, lhandle, 0);
}

/// @function check_start
/// @method uv_check:start
/// @param self uv_check The {{uv_check}} handle.
/// @param callback function The callback to be run on the next loop iteration.
/// @comment Start the {{uv_check}} handle with the given callback.
LUV_LUAAPI int luv_check_start(lua_State *L) {
    uv_check_t *handle = luv_check_check(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_set(L, LUV_CB_EVENT, lhandle, 2);
    (void)uv_check_start(handle, luv_check_cb);
    return 0;
}

/// @function check_stop
/// @method uv_check:stop
/// @param self uv_check The {{uv_check}} handle.
/// @comment Stop the {{uv_check}} handle. The callback will no longer be called.
LUV_LUAAPI int luv_check_stop(lua_State *L) {
    uv_check_t *handle = luv_check_check(L, 1);
    luv_handle_t *lhandle = luv_handle_from(handle);

    luv_callback_unset(L, LUV_CB_EVENT, lhandle);
    (void)uv_check_stop(handle);
    return 0;
}
