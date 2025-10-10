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

#ifndef LUV_HANDLE_H
#define LUV_HANDLE_H

#include "internal.h"
#include "luv.h"

enum luv_callback_id {
    /* when the handle is closed */
    LUV_CB_CLOSE,
    /* when an event is emitted by this handle, each type of handle only ever handles a single event */
    LUV_CB_EVENT,
    LUV_CB_MAX,
};

typedef struct {
    /* references to the functions to be called for each callback */
    int callbacks[LUV_CB_MAX];

    /* reference in the lua state of the attached context to the userdata for this handle */
    int ref;

    /* the uv handle type immediately follows the above metadata */
    // uv_handle_t handle;
} luv_handle_t;

/* create and populate the metatable for the handle named name */
LUV_LIBAPI void luv_handle_metatable(lua_State *L, uv_handle_type type, const luaL_Reg *methods);

/* allocate and initialize a new luv handle */
LUV_LIBAPI luv_handle_t *luv_new_handle(lua_State *L, uv_handle_type type);

/* remove the reference to the handle and all callbacks, allowing it to be garbage collected and eventually freed.
 * this must not be called while the handle is still active because the lua userdata stores the libuv handle. */
LUV_LIBAPI void luv_handle_unref(lua_State *L, luv_handle_t *data);

/* push the userdata associated with this handle */
LUV_LIBAPI void luv_handle_push(lua_State *L, const luv_handle_t *lhandle);

/* set a callback for a luv handle */
LUV_LIBAPI void luv_callback_set(lua_State *L, enum luv_callback_id what, luv_handle_t *lhandle, int index);

/* unset a callback for a luv handle */
LUV_LIBAPI void luv_callback_unset(lua_State *L, enum luv_callback_id what, luv_handle_t *lhandle);

/* call a registered handle callback */
LUV_LIBAPI void luv_callback_send(lua_State *L, enum luv_callback_id what, const luv_handle_t *lhandle, int nargs);

/* ensure the userdata at the given index is a handle and return a pointer to it */
LUV_LIBAPI luv_handle_t *luv_check_handle(lua_State *L, int index);

/* obtain a pointer to the libuv handle type given a luv_handle_t */
#define luv_handle_of(utype, lhandle) ((utype *)((char *)(lhandle) + sizeof(luv_handle_t)))

/* obtain a pointer to the luv_handle_t given a libuv handle type */
#define luv_handle_from(uhandle) ((luv_handle_t *)((char *)(uv_handle_t *)(uhandle) - sizeof(luv_handle_t)))

static inline luv_loop_t *luv_handle_ctx(const luv_handle_t *lhandle) {
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);
    return (luv_loop_t *)handle->loop;
}

static inline lua_State *luv_handle_state(const luv_handle_t *lhandle) {
    return luv_ctx_state(luv_handle_ctx(lhandle));
}

LUV_LUAAPI int luv_handle__tostring(lua_State *L);
LUV_LUAAPI int luv_handle__gc(lua_State *L);
LUV_LUAAPI int luv_close(lua_State *L);
LUV_LUAAPI int luv_is_active(lua_State *L);
LUV_LUAAPI int luv_is_closing(lua_State *L);
LUV_LUAAPI int luv_ref(lua_State *L);
LUV_LUAAPI int luv_unref(lua_State *L);
LUV_LUAAPI int luv_has_ref(lua_State *L);
LUV_LUAAPI int luv_send_buffer_size(lua_State *L);
LUV_LUAAPI int luv_recv_buffer_size(lua_State *L);
LUV_LUAAPI int luv_fileno(lua_State *L);
LUV_LUAAPI int luv_handle_get_type(lua_State *L);

static luaL_Reg luv_handle_functions[] = {
    {"close", luv_close},
    {"is_active", luv_is_active},
    {"is_closing", luv_is_closing},
    {"ref", luv_ref},
    {"unref", luv_unref},
    {"has_ref", luv_has_ref},
    {"send_buffer_size", luv_send_buffer_size},
    {"recv_buffer_size", luv_recv_buffer_size},
    {"fileno", luv_fileno},
    {"handle_get_type", luv_handle_get_type},
    {NULL, NULL},
};

#endif