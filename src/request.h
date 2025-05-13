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
#ifndef LUV_REQ_H
#define LUV_REQ_H

#include <lua.h>
#include <uv.h>

#include "luv.h"
#include "private.h"

typedef struct {
  /* reference of the function to be called when the request is complete */
  int callback;

  /* reference in the lua state of the attached context to the userdata for this request */
  int ref;

  /* reference to any data necessary to complete the request */
  int data;

  /* set when data == LUA_REFNIL, list of references to any data necessary to complete the request and terminated with a
   * LUA_NOREF */
  int *data_list;

  /* the luv context that created and is managing this request */
  const luv_ctx_t *ctx;

  /* the uv request type immediately follows the above metadata */
  // uv_req_t req;
} luv_req_t;

/* obtain a pointer to the libuv request type given a luv_req_t */
#define luv_request_of(utype, lreq) ((utype *)((char *)(lreq) + sizeof(luv_req_t)))

/* obtain a pointer to the luv_req_t given a libuv request type */
#define luv_request_from(ureq) ((luv_req_t *)((char *)(ureq) - sizeof(luv_req_t)))

/* allocate and initialize a new luv request */
LUV_LIBAPI luv_req_t *luv_new_request(lua_State *L, uv_req_type type, const luv_ctx_t *ctx, int callback_idx);

/* fulfill a luv request by calling the associated callback with the given arguments */
LUV_LIBAPI void luv_fulfill_req(lua_State *L, luv_req_t *data, int nargs);

/* remove the reference to the request and all callbacks, and free all associated data */
LUV_LIBAPI void luv_cleanup_req(lua_State *L, luv_req_t *data);

#endif  // LUV_REQ_H