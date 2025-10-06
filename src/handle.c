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
#include "handle.h"

#include "internal.h"
#include "luv.h"

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

static const char *const luv_handle_type_names[UV_HANDLE_TYPE_MAX] = {
    [UV_FILE] = "uv_file",
#define XX(uc, lc) [UV_##uc] = "uv_" #lc,
    UV_HANDLE_TYPE_MAP(XX)
#undef XX
};

static const char *luv_handle_type_name(uv_handle_type type) {
    if (type >= 0 && type < UV_HANDLE_TYPE_MAX) {
        return luv_handle_type_names[type];
    }
    return NULL;
}

/* provides us an address which we guarantee is unique to us to mark handle metatables */
static char handle_key;

LUV_LIBAPI void luv_handle_metatable(lua_State *L, uv_handle_type type, const luaL_Reg *methods) {
    const char *name = luv_handle_type_name(type);
    luv_assert(L, name, "invalid handle type");

    luaL_newmetatable(L, name);

    lua_pushboolean(L, 1);
    lua_rawsetp(L, -2, &handle_key);

    lua_pushstring(L, name);
    lua_setfield(L, -2, "__name");

    lua_pushcfunction(L, luv_handle__tostring);
    lua_setfield(L, -2, "__tostring");

    lua_pushcfunction(L, luv_handle__gc);
    lua_setfield(L, -2, "__gc");

    luaL_newlib(L, luv_handle_methods);
    luaL_setfuncs(L, methods, 0);
    lua_setfield(L, -2, "__index");
}

LUV_LIBAPI luv_handle_t *luv_new_handle(lua_State *L, uv_handle_type type) {
    const char *name = luv_handle_type_name(type);
    luv_assert(L, name, "invalid handle type");

    size_t attached_size = uv_handle_size(type);
    luv_handle_t *lhandle = (luv_handle_t *)lua_newuserdata(L, sizeof(luv_handle_t) + attached_size);

    luv_assert(L, luaL_getmetatable(L, name) == LUA_TTABLE, "missing handle metatable");
    lua_setmetatable(L, -2);

    lua_pushvalue(L, -1);
    lhandle->ref = luaL_ref(L, LUA_REGISTRYINDEX);

    lhandle->callbacks[0] = LUA_NOREF;
    lhandle->callbacks[1] = LUA_NOREF;

    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    memset((void *)handle, 0, attached_size);
    handle->type = type;
    handle->data = (void *)&handle_key;

    return lhandle;
}

LUV_LIBAPI void luv_handle_unref(lua_State *L, luv_handle_t *lhandle) {
    luaL_unref(L, LUA_REGISTRYINDEX, lhandle->ref);
    luaL_unref(L, LUA_REGISTRYINDEX, lhandle->callbacks[0]);
    luaL_unref(L, LUA_REGISTRYINDEX, lhandle->callbacks[1]);

    lhandle->ref = LUA_NOREF;
    lhandle->callbacks[0] = LUA_NOREF;
    lhandle->callbacks[1] = LUA_NOREF;
}

LUV_LIBAPI void luv_handle_push(lua_State *L, const luv_handle_t *lhandle) {
    lua_rawgeti(L, LUA_REGISTRYINDEX, lhandle->ref);
}

LUV_LIBAPI void luv_callback_set(lua_State *L, const enum luv_callback_id what, luv_handle_t *lhandle, int index) {
    luv_checkcallable(L, index);
    luaL_unref(L, LUA_REGISTRYINDEX, lhandle->callbacks[what]);
    lua_pushvalue(L, index);
    lhandle->callbacks[what] = luaL_ref(L, LUA_REGISTRYINDEX);
}

LUV_LIBAPI void luv_callback_unset(lua_State *L, enum luv_callback_id what, luv_handle_t *lhandle) {
    luaL_unref(L, LUA_REGISTRYINDEX, lhandle->callbacks[what]);
    lhandle->callbacks[what] = LUA_NOREF;
}

LUV_LIBAPI void luv_callback_send(lua_State *L, enum luv_callback_id what, const luv_handle_t *lhandle, int nargs) {
    luv_loop_t *ctx = luv_handle_ctx(lhandle);

    int ref = lhandle->callbacks[what];
    if (ref == LUA_NOREF) {
        lua_pop(L, nargs);
        return;
    }

    // fetch the callback and insert it before any arguments
    lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
    if (nargs != 0)
        lua_insert(L, -1 - nargs);

    int status = ctx->docall(L, nargs, 0, ctx->errfunc);
    if (status != LUA_OK)
        lua_error(L);
}

LUV_LIBAPI luv_handle_t *luv_check_handle(lua_State *L, int index) {
    void *userdata = lua_touserdata(L, index);
    if (userdata == NULL)
        luv_typeerror(L, index, "uv_handle");

    if (lua_getmetatable(L, index) == 0)
        luv_typeerror(L, index, "uv_handle");

    lua_rawgetp(L, -1, &handle_key);
    if (lua_toboolean(L, -1) == 0)
        luv_typeerror(L, index, "uv_handle");

    lua_pop(L, 2);
    return (luv_handle_t *)userdata;
}

// Show the libuv type instead of generic "userdata"
LUV_LUAAPI int luv_handle__tostring(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    const char *name = luv_handle_type_name(handle->type);
    if (name == NULL)
        name = "uv_handle";

    if (lhandle->ref == LUA_NOREF) {
        lua_pushfstring(L, "%s: %p (closed)", name, handle);
    } else {
        lua_pushfstring(L, "%s: %p", name, handle);
    }

    return 1;
}

LUV_LUAAPI int luv_handle__gc(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    // either already closed or currently closing
    int closed = (lhandle->ref == LUA_NOREF) || (uv_is_closing(handle) != 0);

    // if the handle isn't already closed then we're closing the lua state. we must be careful here because we cannot
    // revive a value. we depend entirely on the uv_loop being garbage collected at the same time, which will block
    // until all handles are closed.
    if (!closed)
        uv_close(handle, NULL);

    // release any callback references we may be holding
    luv_handle_unref(L, lhandle);
    return 0;
}

static void luv_close_cb(uv_handle_t *handle) {
    luv_handle_t *lhandle = luv_handle_from(handle);
    lua_State *L = luv_handle_state(lhandle);

    // push the userdata associated with this handle
    luv_handle_push(L, lhandle);

    // mark the handle as closed and release references
    luv_handle_unref(L, lhandle);

    // call the close callback if one was registered
    luv_callback_send(L, LUV_CB_CLOSE, lhandle, 1);
}

/// @function close
/// @method uv_handle:close
/// @comment Request handle to be closed.
///
/// In-progress requests, like `uv_connect` or `uv_write`, are cancelled and have their callbacks called asynchronously
/// with `ECANCELED`.
/// @param self uv_handle The `uv_handle` to close.
/// @param callback function|nil A function to be called when the handle is closed.
LUV_LUAAPI int luv_close(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    if (uv_is_closing(handle) != 0)
        luv_error(L, "handle %p is already closing", handle);

    if (!lua_isnoneornil(L, 2))
        luv_callback_set(L, LUV_CB_CLOSE, lhandle, 2);

    uv_close(handle, luv_close_cb);
    return 0;
}

/// @function is_active
/// @method uv_handle:is_active
/// @comment Check if handle is active. What "active" means depends on the type of handle:
/// - A {{uv_async}} handle is always active and cannot be deactivated, except by closing it with {{uv_handle:close}}.
/// - A {{uv_pipe}}, {{uv_tcp}}, {{uv_tty}}; basically any handle that deals with I/O, is active when it is doing
/// something
///   that involves I/O, like reading, writing, connecting, accepting new connections, etc.
/// - A {{uv_check}}, {{uv_idle}}, {{uv_timer}} is active when it has been started with {{uv_check:start}},
/// {{uv_idle:start}},
///   or {{uv_timer:start}}, respectively until it is stopped with its corresponding stop function or closed with
///   {{uv_handle:close}}.
/// @param self uv_handle The `uv_handle` to check.
/// @return active boolean `true` if the handle is active, `false` otherwise.
LUV_LUAAPI int luv_is_active(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    int ret = uv_is_active(handle);
    if (ret < 0)
        goto fail;

    lua_pushboolean(L, ret);
    return 1;

fail:
    return luv_pushfail(L, ret);
}

/// @function is_closing
/// @method uv_handle:is_closing
/// @comment Check if handle is closed or closing.
/// @param self uv_handle The `uv_handle` to check.
/// @return closing boolean `true` if the handle is closed or closing, `false` otherwise.
LUV_LUAAPI int luv_is_closing(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    int ret = uv_is_closing(handle);
    if (ret < 0)
        goto fail;

    lua_pushboolean(L, ret);
    return 1;

fail:
    return luv_pushfail(L, ret);
}

/// @function ref
/// @method uv_handle:ref
/// @comment Reference the given handle. References are idempotent, that is, if a handle is already referenced calling
/// this function again will have no effect.
/// @param self uv_handle The `uv_handle` to reference.
LUV_LUAAPI int luv_ref(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    uv_ref(handle);
    return 0;
}

/// @function unref
/// @method uv_handle:unref
/// @comment Un-reference the given handle. References are idempotent, that is, if a handle is not referenced calling
/// this function again will have no effect.
/// @param self uv_handle The `uv_handle` to un-reference.
LUV_LUAAPI int luv_unref(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    uv_unref(handle);
    return 0;
}

/// @function has_ref
/// @method uv_handle:has_ref
/// @comment Check if the given handle is referenced.
/// @param self uv_handle The `uv_handle` to check.
/// @return referenced boolean `true` if the handle is referenced, `false` otherwise.
LUV_LUAAPI int luv_has_ref(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    int ret = uv_has_ref(handle);
    if (ret < 0)
        goto fail;

    lua_pushboolean(L, ret);
    return 1;

fail:
    return luv_pushfail(L, ret);
}

/// @function send_buffer_size
/// @method uv_handle:send_buffer_size
/// @comment Get or set the size of the send buffer for a socket.
///
/// > [!NOTE]
/// > Linux will set double the size and return double the size of the original set value.
/// @param self uv_handle The `uv_handle` to get or set the send buffer size.
/// @param size integer|nil The size to set the send buffer to. If `nil` or `0` the current size is returned.
/// @return size integer When size is `nil` or `0`, the current size of the send buffer. Otherwise the value passed in
/// as `size`.
/// @return[Fail] size nil
/// @return[Fail] string errmsg The error message, e.g. `"EINVAL: Invalid argument"`.
/// @return[Fail] string errname The error name, e.g. `"EINVAL"`.
/// message.
LUV_LUAAPI int luv_send_buffer_size(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);
    int value = (int)luaL_optinteger(L, 2, 0);

    int ret = uv_send_buffer_size(handle, &value);
    if (ret < 0)
        goto fail;

    lua_pushinteger(L, value);
    return 1;

fail:
    return luv_pushfail(L, ret);
}

/// @function recv_buffer_size
/// @method uv_handle:recv_buffer_size
/// @comment Get or set the size of the receive buffer for a socket.
///
/// > [!NOTE]
/// > Linux will set double the size and return double the size of the original set value.
/// @param self uv_handle The `uv_handle` to get or set the receive buffer size.
/// @param size integer|nil The size to set the receive buffer to. If `nil` or `0` the current size is returned.
/// @return size integer When size is `nil` or `0`, the current size of the receive buffer. Otherwise the value passed
/// in as `size`.
/// @return[Fail] size nil
/// @return[Fail] string errmsg The error message, e.g. `"EINVAL: Invalid argument"`.
/// @return[Fail] string errname The error name, e.g. `"EINVAL"`.
LUV_LUAAPI int luv_recv_buffer_size(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);
    int value = (int)luaL_optinteger(L, 2, 0);

    int ret = uv_recv_buffer_size(handle, &value);
    if (ret < 0)
        goto fail;

    lua_pushinteger(L, value);
    return 1;

fail:
    return luv_pushfail(L, ret);
}

/// @function fileno
/// @method uv_handle:fileno
/// @comment Gets the platform dependent file descriptor equivalent.
///
/// The following handles are supported: TCP, pipes, TTY, UDP and poll. Passing any other handle type will fail with
/// `EINVAL`. If a handle doesn't have an attached file descriptor yet or the handle itself has been closed, this
/// function will return `EBADF`.
///
/// > [!WARNING]
/// > Be very careful when using this function. libuv assumes it’s in control of the file descriptor so any change to it
/// > may lead to malfunction.
/// @param self uv_handle The `uv_handle` to get the file descriptor for.
/// @return[Success] fd integer The file descriptor.
/// @return[Fail] size nil
/// @return[Fail] string errmsg The error message, e.g. `"EINVAL: Invalid argument"`.
/// @return[Fail] string errname The error name, e.g. `"EINVAL"`.
LUV_LUAAPI int luv_fileno(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    uv_os_fd_t fd;
    int ret = uv_fileno(handle, &fd);
    if (ret < 0)
        goto fail;

    lua_pushinteger(L, (lua_Integer)fd);
    return 1;

fail:
    return luv_pushfail(L, ret);
}

/// @function get_type
/// @method uv_handle:get_type
/// @comment Returns the name of the struct for a given handle (e.g. "pipe" for `uv_pipe`) and the libuv enum integer
/// for the handle's type (A `uv_handle_type`).
/// @param self uv_handle The `uv_handle` to get the type for.
/// @return name string The name of the struct for the handle (e.g. "pipe" for `uv_pipe`).
/// @return type integer The libuv enum integer for the handle's type (A `uv_handle_type`).
LUV_LUAAPI int luv_handle_get_type(lua_State *L) {
    luv_handle_t *lhandle = luv_check_handle(L, 1);
    uv_handle_t *handle = luv_handle_of(uv_handle_t, lhandle);

    uv_handle_type type = handle->type;
    const char *name = luv_handle_type_name(type);
    if (name == NULL)
        name = "uv_handle";

    // return the type name without the "uv_" prefix and the enum value
    lua_pushstring(L, name + 3);
    lua_pushinteger(L, type);
    return 2;
}
