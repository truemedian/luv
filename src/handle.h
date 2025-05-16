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

#include <lauxlib.h>
#include <lua.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

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

  /* the luv context that created and is managing this handle */
  const luv_ctx_t *ctx;

  /* the uv handle type immediately follows the above metadata */
  // uv_handle_t handle;

  /* any extra associated data immediately follows the full libuv handle */
  // auto extra;
} luv_handle_t;

LUV_LIBAPI luaL_Reg luv_handle_methods[];
LUV_LIBAPI luaL_Reg luv_handle_functions[];

/* create and populate the metatable for the handle named name */
LUV_LIBAPI int luv_handle_metatable(lua_State *L, const char *name, const luaL_Reg *methods);

/* allocate and initialize a new luv handle */
LUV_LIBAPI luv_handle_t *luv_new_handle(lua_State *L, uv_handle_type type, const luv_ctx_t *ctx, size_t extra_sz);

/* remove the reference to the handle and all callbacks, allowing it to be garbage collected and eventually freed */
LUV_LIBAPI void luv_handle_unref(lua_State *L, luv_handle_t *data);

/* push the userdata associated with this handle */
LUV_LIBAPI void luv_handle_push(lua_State *L, luv_handle_t *data);

/* set a callback for a luv handle */
LUV_LIBAPI void luv_callback_prep(lua_State *L, enum luv_callback_id what, luv_handle_t *data, int index);

/* call a registered handle callback */
LUV_LIBAPI void luv_callback_send(lua_State *L, enum luv_callback_id what, const luv_handle_t *data, int nargs);

/* ensure the userdata at the given index is a handle and return a pointer to it */
static luv_handle_t *luv_check_handle(lua_State *L, int index);

/* obtain a pointer to the libuv handle type given a luv_handle_t */
#define luv_handle_of(utype, lhandle) ((utype *)((char *)(lhandle) + sizeof(luv_handle_t)))

/* obtain a pointer to the luv_handle_t given a libuv handle type */
#define luv_handle_from(uhandle) ((luv_handle_t *)((char *)(uv_handle_t *)(uhandle) - sizeof(luv_handle_t)))

/* obtain a pointer to any attached extra data given a luv_handle_t */
#define luv_handle_extra(utype, lhandle) ((char *)(lhandle) + sizeof(luv_handle_t) + sizeof(utype))

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

#endif  // LUV_HANDLE_H