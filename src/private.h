#ifndef LUV_PRIVATE_H
#define LUV_PRIVATE_H

#include <lua.h>
#if (LUA_VERSION_NUM < 503)
#include "compat-5.3.h"
#endif

#include "lreq.h"
#include "lthreadpool.h"
#include "luv.h"

/* From stream.c */
static uv_stream_t* luv_check_stream(lua_State* L, int index);
static void luv_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf);

/* From lhandle.c */
/* Traceback for lua_pcall */
static int luv_traceback(lua_State* L);

/* From lreq.c */
/* Used in the top of a setup function to check the arg
   and ref the callback to an integer.
*/
static int luv_check_continuation(lua_State* L, int index);

/* setup a luv_req_t.  The userdata is assumed to be at the
   top of the stack.
*/
static luv_req_t* luv_setup_req(lua_State* L, luv_ctx_t* ctx, int ref);
static luv_req_t* luv_setup_req_with_mt(lua_State* L, luv_ctx_t* ctx, int ref, const char* mt_name);
static void luv_fulfill_req(lua_State* L, luv_req_t* data, int nargs);
static void luv_cleanup_req(lua_State* L, luv_req_t* data);

/* From misc.c */
static void luv_prep_buf(lua_State* L, int idx, uv_buf_t* pbuf);
static uv_buf_t* luv_prep_bufs(lua_State* L, int index, size_t* count, int** refs);
static uv_buf_t* luv_check_bufs(lua_State* L, int index, size_t* count, luv_req_t* req_data);
static uv_buf_t* luv_check_bufs_noref(lua_State* L, int index, size_t* count);

/* From tcp.c */
static void parse_sockaddr(lua_State* L, struct sockaddr_storage* address);
static void luv_connect_cb(uv_connect_t* req, int status);

/* From fs.c */
static void luv_push_stats_table(lua_State* L, const uv_stat_t* s);

/* From util.c */
static int luv_optboolean(lua_State* L, int idx, int defaultval);

/* From thread.c */
static lua_State* luv_thread_acquire_vm(void);

/* From process.c */
static int luv_parse_signal(lua_State* L, int slot);

/* From work.c */
static int luv_thread_dumped(lua_State* L, int idx);
static const char* luv_getmtname(lua_State* L, int idx);
static int luv_thread_arg_set(lua_State* L, luv_thread_arg_t* args, int idx, int top, int flags);
static int luv_thread_arg_push(lua_State* L, luv_thread_arg_t* args, int flags);
static void luv_thread_arg_clear(lua_State* L, luv_thread_arg_t* args, int flags);
static int luv_thread_arg_error(lua_State* L);

static luv_acquire_vm acquire_vm_cb = NULL;
static luv_release_vm release_vm_cb = NULL;

#endif
