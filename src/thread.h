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
#ifndef LUV_LTHREADPOOL_H
#define LUV_LTHREADPOOL_H

#include "luv.h"

#define LUV_THREAD_MAXNUM_ARG 9

typedef struct {
  int type;
  union {
    int boolean;
    struct {
      union {
        lua_Number number;
        lua_Integer integer;
      };
      int isinteger;
    } number;
    struct {
      const char* base;
      size_t len;
      int ref;
    } string;
    struct {
      size_t size;
      const void* data;
      int producer_ref; // this is our reference to the userdata, so the other side knows how to identify if it comes back
      int consumer_ref; // if we got this userdata from the other side, this is its reference on the other side
      const char* name;
      int name_ref;
      
      // for more complex userdata
      lua_CFunction constructor;
    } userdata;
  };
} luv_thread_arg_t;

typedef struct {
  int argc;
  int flags;          // control gc

  luv_thread_arg_t argv[LUV_THREAD_MAXNUM_ARG];
} luv_thread_args_t;

//luajit miss LUA_OK
#ifndef LUA_OK
#define LUA_OK 0
#endif

//LUV flags for thread or threadpool args
#define LUVF_THREAD_SIDE_MAIN      0x00
#define LUVF_THREAD_SIDE_CHILD     0x01
#define LUVF_THREAD_MODE_ASYNC     0x02
#define LUVF_THREAD_SIDE(i)        ((i)&0x01)
#define LUVF_THREAD_ASYNC(i)       ((i)&0x02)

#endif //LUV_LTHREADPOOL_H
