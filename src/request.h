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
#ifndef LUV_REQUEST_H
#define LUV_REQUEST_H

#include <lauxlib.h>
#include <lua.h>
#include <uv.h>

#include "internal.h"
#include "luv.h"

// how many uv_buf_t we prepare in advance before we must allocate
#define LUV_BUFS_PREPARED 16

typedef struct {
  // the number of buffers in bufs
  size_t bufs_count;

  union {
    // pointer to an array of buffers, used when bufs_count > LUV_BUFS_PREPARED
    uv_buf_t* ptr;

    // storage space for the prepared buffers, used when bufs_count <= LUV_BUFS_PREPARED
    uv_buf_t array[LUV_BUFS_PREPARED];
  };
} luv_bufs_t;

#define luv_bufs_prepare(bufs, count)                  \
  do {                                                 \
    (bufs).bufs_count = (count);                      \
    if ((count) > LUV_BUFS_PREPARED) {                 \
      (bufs).ptr = calloc((count), sizeof(uv_buf_t)); \
      if ((bufs).ptr == NULL)                         \
        luv_error(L, "out of memory");                 \
    }                                                  \
  } while (0)

#define luv_bufs_ptr(bufs) ((bufs).bufs_count > LUV_BUFS_PREPARED ? (bufs).ptr : (bufs).array)

#define luv_bufs_free(bufs)                       \
  do {                                            \
    if ((bufs).bufs_count > LUV_BUFS_PREPARED) { \
      free((bufs).ptr);                          \
    }                                             \
  } while (0)

typedef struct {
  // reference to the userdata holding this request to ensure the userdata outlasts it's uses
  int ref;

  // references to a callable that should be invoked when the request completes
  int callback;

  // reference to any data associated with this request
  int data;

  // list of references to data associated with this request, populated when data == LUA_REFNIL, terminated by LUA_NOREF
  int* data_list;

  // the context that created and is using this handle
  const luv_ctx_t* ctx;
} luv_request_t;

// returns the name of the uv request type with a "uv_" prefix
static const char* luv_request_type_name(uv_req_type type);

// obtain a pointer to a libuv request given a luv_request_t
#define luv_request_of(utype, lreq) ((utype*)((char*)(lreq) + sizeof(luv_request_t)))

/* obtain a pointer to the luv_request_t given a libuv request type */
#define luv_request_from(ureq) ((luv_request_t*)((char*)(ureq) - sizeof(luv_request_t)))

// create a luv_request_t with an attached uv request
static luv_request_t* luv_request_create(lua_State* L, uv_req_type type, const luv_ctx_t* ctx, int callback);

// remove all references this handle is holding to allow it to be garbage collected
static void luv_request_unref(lua_State* L, luv_request_t* lrequest);

// call the callback for this request with the top `nargs` of the stack or pop `nargs` off the stack
static void luv_request_finish(lua_State* L, luv_request_t* lrequest, int nargs);

// populate a luv_bufs_t with a string or table of strings, holds references when given an associated request
static void luv_request_buffers(lua_State* L, luv_request_t* lrequest, luv_bufs_t* bufs, int index);

static int luv_request_tostring(lua_State* L);
static int luv_request_gc(lua_State* L);

static int luv_request_cancel(lua_State* L);

static int luv_request_get_type(lua_State* L);

static luaL_Reg luv_request_methods[] = {
  {"cancel", luv_request_cancel},
  {"get_type", luv_request_get_type},
  {NULL, NULL},
};

static luaL_Reg luv_request_functions[] = {
  {"cancel", luv_request_cancel},
  {"request_get_type", luv_request_get_type},
  {NULL, NULL},
};

#endif
