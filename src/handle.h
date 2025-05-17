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
#ifndef LUV_HANDLE_H
#define LUV_HANDLE_H

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "internal.h"
#include "luv.h"

enum luv_callback_id {
  // when the handle is closed
  LUV_CB_CLOSE,
  // when an event is emitted by this handle, each type of handle only ever handles a single event
  LUV_CB_EVENT,
  LUV_CB_MAX,
};

typedef struct luv_handle_s {
  // reference to the userdata holding this handle to ensure the userdata outlasts it's uses
  int ref;

  // references to a callable that should be invoked when something happens
  int callback[LUV_CB_MAX];

  // the context that created and is using this handle
  const luv_ctx_t* ctx;
} luv_handle_t;

// returns the name of the uv handle type with a "uv_" prefix
static const char* luv_handle_type_name(uv_handle_type type);

// obtain a pointer to a libuv handle given a luv_handle_t
#define luv_handle_of(utype, lhandle) ((utype*)((char*)(lhandle) + sizeof(luv_handle_t)))

// obtain a pointer to a luv_handle_t given a libuv handle
#define luv_handle_from(uhandle) ((luv_handle_t*)((char*)(uv_handle_t*)(uhandle) - sizeof(luv_handle_t)))

// obtain a pointer to the attached extra data given a luv_handle_t
#define luv_handle_extra(utype, lhandle) ((char*)(lhandle) + sizeof(luv_handle_t) + sizeof(utype))

// creates registers and pushes a uv handle metatable pre-filled with all handle and provided methods
static int luv_handle_metatable(lua_State* L, const char* name, const luaL_Reg* methods);

// create a luv_handle_t with an attached uv handle and extra data
static luv_handle_t* luv_handle_create(lua_State* L, uv_handle_type type, const luv_ctx_t* ctx, size_t extra_sz);

// remove all references this handle is holding to allow it to be garbage collected
static void luv_handle_unref(lua_State* L, luv_handle_t* lhandle);

// push the userdata associated with this handle to the stack
static void luv_handle_push(lua_State* L, luv_handle_t* lhandle);

// ensure this handle is still valid and throw an error otherwise
static void luv_handle_check_valid(lua_State* L, luv_handle_t* lhandle);

// set a callback for a specific event on this handle to the callable at `index`
static void luv_handle_set_callback(lua_State* L, enum luv_callback_id what, luv_handle_t* lhandle, int index);

// unset a callback for a specific event on this handle, usually after failure to start the handle
static void luv_handle_unset_callback(lua_State* L, enum luv_callback_id what, luv_handle_t* lhandle);

// call the callback registered to `what` with the top `nargs` of the stack or pop `nargs` off the stack
static void luv_handle_send_callback(lua_State* L, enum luv_callback_id what, luv_handle_t* lhandle, int nargs);

// check that the userdata at the given index is a handle and return a pointer to it
static luv_handle_t* luv_check_handle(lua_State* L, int index);

static int luv_handle_tostring(lua_State* L);
static int luv_handle_gc(lua_State* L);

static int luv_close(lua_State* L);
static int luv_is_active(lua_State* L);
static int luv_is_closing(lua_State* L);

static int luv_ref(lua_State* L);
static int luv_unref(lua_State* L);
static int luv_has_ref(lua_State* L);

static int luv_send_buffer_size(lua_State* L);
static int luv_recv_buffer_size(lua_State* L);

static int luv_fileno(lua_State* L);

static int luv_handle_get_type(lua_State* L);

static luaL_Reg luv_handle_methods[] = {
  {"close", luv_close},
  {"is_active", luv_is_active},
  {"is_closing", luv_is_closing},
  {"ref", luv_ref},
  {"unref", luv_unref},
  {"has_ref", luv_has_ref},
  {"send_buffer_size", luv_send_buffer_size},
  {"recv_buffer_size", luv_recv_buffer_size},
  {"fileno", luv_fileno},
  {"get_type", luv_handle_get_type},
  {NULL, NULL},
};

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