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
#include "constants.h"

#include "internal.h"

#ifndef WIN32
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

struct constant {
    const char *name;
    int value;
};

static const struct constant constants[] = {
    // file access flags *_OK
    {"R_OK", R_OK},
    {"W_OK", W_OK},
    {"X_OK", X_OK},
    {"F_OK", F_OK},

    // file open flags O_*
    {"O_APPEND", UV_FS_O_APPEND},
    {"O_CREAT", UV_FS_O_CREAT},
    {"O_DIRECT", UV_FS_O_DIRECT},
    {"O_DSYNC", UV_FS_O_DSYNC},
    {"O_EXCL", UV_FS_O_EXCL},
    {"O_EXLOCK", UV_FS_O_EXLOCK},
    {"O_RDONLY", UV_FS_O_RDONLY},
    {"O_NOCTTY", UV_FS_O_NOCTTY},
    {"O_NOFOLLOW", UV_FS_O_NOFOLLOW},
    {"O_NONBLOCK", UV_FS_O_NONBLOCK},
    {"O_RDWR", UV_FS_O_RDWR},
    {"O_SYNC", UV_FS_O_SYNC},
    {"O_TRUNC", UV_FS_O_TRUNC},
    {"O_WRONLY", UV_FS_O_WRONLY},
#ifdef O_RSYNC
    {"O_RSYNC", O_RSYNC},
#endif

    // addrinfo flags AI_*
    {"AI_ADDRCONFIG", AI_ADDRCONFIG},
    {"AI_NUMERICHOST", AI_NUMERICHOST},
    {"AI_PASSIVE", AI_PASSIVE},
    {"AI_NUMERICSERV", AI_NUMERICSERV},
#ifdef AI_V4MAPPED
    {"AI_V4MAPPED", AI_V4MAPPED},
#endif
#ifdef AI_ALL
    {"AI_ALL", AI_ALL},
#endif

    // libuv defined constants
    {"UDP_REUSEADDR", UV_UDP_REUSEADDR},
    {"UDP_PARTIAL", UV_UDP_PARTIAL},
    {"UDP_IPV6ONLY", UV_UDP_IPV6ONLY},
    {"TCP_IPV6ONLY", UV_TCP_IPV6ONLY},
#if LUV_UV_VERSION_GEQ(1, 2, 0)
    {"TTY_MODE_NORMAL", UV_TTY_MODE_NORMAL},
    {"TTY_MODE_RAW", UV_TTY_MODE_RAW},
    {"TTY_MODE_IO", UV_TTY_MODE_IO},
#endif
#if LUV_UV_VERSION_GEQ(1, 35, 0)
    {"UDP_MMSG_CHUNK", UV_UDP_MMSG_CHUNK},
#endif
#if LUV_UV_VERSION_GEQ(1, 37, 0)
    {"UDP_RECVMMSG", UV_UDP_RECVMMSG},
#endif
#if LUV_UV_VERSION_GEQ(1, 40, 0)
    {"UDP_MMSG_FREE", UV_UDP_MMSG_FREE},
#endif
#if LUV_UV_VERSION_GEQ(1, 46, 0)
    {"PIPE_NO_TRUNCATE", UV_PIPE_NO_TRUNCATE},
#endif
#if LUV_UV_VERSION_GEQ(1, 48, 0)
    {"THREAD_PRIORITY_HIGHEST", UV_THREAD_PRIORITY_HIGHEST},
    {"THREAD_PRIORITY_ABOVE_NORMAL", UV_THREAD_PRIORITY_ABOVE_NORMAL},
    {"THREAD_PRIORITY_NORMAL", UV_THREAD_PRIORITY_NORMAL},
    {"THREAD_PRIORITY_BELOW_NORMAL", UV_THREAD_PRIORITY_BELOW_NORMAL},
    {"THREAD_PRIORITY_LOWEST", UV_THREAD_PRIORITY_LOWEST},
#endif
#if LUV_UV_VERSION_GEQ(1, 51, 0)
    {"TTY_MODE_RAW_VT", UV_TTY_MODE_RAW_VT},
#endif
    {NULL, 0},
};

static const struct constant address_families[] = {
    {"AF_UNSPEC", AF_UNSPEC},
#ifdef AF_INET6
    {"AF_INET", AF_INET},
#endif
#ifdef AF_INET6
    {"AF_INET6", AF_INET6},
#endif
#ifdef AF_UNIX
    {"AF_UNIX", AF_UNIX},
#endif
#ifdef AF_IPX
    {"AF_IPX", AF_IPX},
#endif
#ifdef AF_NETLINK
    {"AF_NETLINK", AF_NETLINK},
#endif
#ifdef AF_X25
    {"AF_X25", AF_X25},
#endif
#ifdef AF_AX25
    {"AF_AX25", AF_AX25},
#endif
#ifdef AF_ATMPVC
    {"AF_ATMPVC", AF_ATMPVC},
#endif
#ifdef AF_APPLETALK
    {"AF_APPLETALK", AF_APPLETALK},
#endif
#ifdef AF_PACKET
    {"AF_PACKET", AF_PACKET},
#endif
    {NULL, 0},
};

static const struct constant socket_domains[] = {
#ifdef SOCK_STREAM
    {"SOCK_STREAM", SOCK_STREAM},
#endif
#ifdef SOCK_DGRAM
    {"SOCK_DGRAM", SOCK_DGRAM},
#endif
#ifdef SOCK_SEQPACKET
    {"SOCK_SEQPACKET", SOCK_SEQPACKET},
#endif
#ifdef SOCK_RAW
    {"SOCK_RAW", SOCK_RAW},
#endif
#ifdef SOCK_RDM
    {"SOCK_RDM", SOCK_RDM},
#endif
    {NULL, 0},
};

static const struct constant signals[] = {
#ifdef SIGHUP
    {"SIGHUP", SIGHUP},
#endif
#ifdef SIGINT
    {"SIGINT", SIGINT},
#endif
#ifdef SIGQUIT
    {"SIGQUIT", SIGQUIT},
#endif
#ifdef SIGILL
    {"SIGILL", SIGILL},
#endif
#ifdef SIGTRAP
    {"SIGTRAP", SIGTRAP},
#endif
#ifdef SIGABRT
    {"SIGABRT", SIGABRT},
#endif
#ifdef SIGIOT
    {"SIGIOT", SIGIOT},
#endif
#ifdef SIGBUS
    {"SIGBUS", SIGBUS},
#endif
#ifdef SIGFPE
    {"SIGFPE", SIGFPE},
#endif
#ifdef SIGKILL
    {"SIGKILL", SIGKILL},
#endif
#ifdef SIGUSR1
    {"SIGUSR1", SIGUSR1},
#endif
#ifdef SIGSEGV
    {"SIGSEGV", SIGSEGV},
#endif
#ifdef SIGUSR2
    {"SIGUSR2", SIGUSR2},
#endif
#ifdef SIGPIPE
    {"SIGPIPE", SIGPIPE},
#endif
#ifdef SIGALRM
    {"SIGALRM", SIGALRM},
#endif
#ifdef SIGTERM
    {"SIGTERM", SIGTERM},
#endif
#ifdef SIGCHLD
    {"SIGCHLD", SIGCHLD},
#endif
#ifdef SIGSTKFLT
    {"SIGSTKFLT", SIGSTKFLT},
#endif
#ifdef SIGCONT
    {"SIGCONT", SIGCONT},
#endif
#ifdef SIGSTOP
    {"SIGSTOP", SIGSTOP},
#endif
#ifdef SIGTSTP
    {"SIGTSTP", SIGTSTP},
#endif
#ifdef SIGBREAK
    {"SIGBREAK", SIGBREAK},
#endif
#ifdef SIGTTIN
    {"SIGTTIN", SIGTTIN},
#endif
#ifdef SIGTTOU
    {"SIGTTOU", SIGTTOU},
#endif
#ifdef SIGURG
    {"SIGURG", SIGURG},
#endif
#ifdef SIGXCPU
    {"SIGXCPU", SIGXCPU},
#endif
#ifdef SIGXFSZ
    {"SIGXFSZ", SIGXFSZ},
#endif
#ifdef SIGVTALRM
    {"SIGVTALRM", SIGVTALRM},
#endif
#ifdef SIGPROF
    {"SIGPROF", SIGPROF},
#endif
#ifdef SIGWINCH
    {"SIGWINCH", SIGWINCH},
#endif
#ifdef SIGIO
    {"SIGIO", SIGIO},
#endif
#ifdef SIGPOLL
    {"SIGPOLL", SIGPOLL},
#endif
#ifdef SIGLOST
    {"SIGLOST", SIGLOST},
#endif
#ifdef SIGPWR
    {"SIGPWR", SIGPWR},
#endif
#ifdef SIGSYS
    {"SIGSYS", SIGSYS},
#endif
    {NULL, 0},
};

static const struct constant errnames[] = {
#define XX(code, _) {#code, UV_##code},
    UV_ERRNO_MAP(XX)
#undef XX
};

#ifdef _WIN32
#include <string.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#else
#include <strings.h>
#endif

LUV_LIBAPI void luv_push_constant_table(lua_State *L) {
    const int tbl_size =
        (sizeof(constants) / sizeof(struct constant)) + (sizeof(address_families) / sizeof(struct constant)) +
        (sizeof(socket_domains) / sizeof(struct constant)) + (sizeof(signals) / sizeof(struct constant));
    lua_createtable(L, 0, tbl_size);

    for (const struct constant *val = constants; val->name != NULL; val++) {
        lua_pushinteger(L, val->value);
        lua_setfield(L, -2, val->name);
    }

    for (const struct constant *val = address_families; val->name != NULL; val++) {
        lua_pushinteger(L, val->value);
        lua_setfield(L, -2, val->name);
    }

    for (const struct constant *val = socket_domains; val->name != NULL; val++) {
        lua_pushinteger(L, val->value);
        lua_setfield(L, -2, val->name);
    }

    for (const struct constant *val = signals; val->name != NULL; val++) {
        lua_pushinteger(L, val->value);
        lua_setfield(L, -2, val->name);
    }

    for (const struct constant *val = errnames; val->name != NULL; val++) {
        lua_pushinteger(L, val->value);
        lua_setfield(L, -2, val->name);
    }

    // these are floating point values
#if LUV_UV_VERSION_GEQ(1, 51, 0)
    lua_pushnumber(L, UV_FS_UTIME_NOW);
    lua_setfield(L, -2, "FS_UTIME_NOW");
    lua_pushnumber(L, UV_FS_UTIME_OMIT);
    lua_setfield(L, -2, "FS_UTIME_OMIT");
#endif
}

LUV_LIBAPI int luv_addrfamily_str2int(const char *string) {
    if (string == NULL || string[0] == '\0')
        return AF_UNSPEC;

    for (const struct constant *val = address_families; val->name != NULL; val++)
        if (strcasecmp(string, val->name) == 0)
            return val->value;

    return AF_UNSPEC;
}

LUV_LIBAPI const char *luv_addrfamily_int2str(const int num) {
    for (const struct constant *val = address_families; val->name != NULL; val++)
        if (num == val->value)
            return val->name;

    return NULL;
}

LUV_LIBAPI int luv_sockdomain_str2int(const char *string) {
    if (string == NULL || string[0] == '\0')
        return -1;

    for (const struct constant *val = socket_domains; val->name != NULL; val++)
        if (strcasecmp(string, val->name) == 0)
            return val->value;

    return -1;
}

LUV_LIBAPI const char *luv_sockdomain_int2str(const int num) {
    for (const struct constant *val = socket_domains; val->name != NULL; val++)
        if (num == val->value)
            return val->name;

    return NULL;
}

LUV_LIBAPI int luv_signal_str2int(const char *string) {
    if (string == NULL || string[0] == '\0')
        return -1;

    // strip off any leading "sig" prefix
    if (strncasecmp(string, "sig", 3) == 0)
        string += 3;

    // compare both names without the "sig" prefix
    for (const struct constant *val = signals; val->name != NULL; val++)
        if (strcasecmp(string, val->name + 3) == 0)
            return val->value;

    return -1;
}

LUV_LIBAPI const char *luv_signal_int2str(const int num) {
    for (const struct constant *val = signals; val->name != NULL; val++)
        if (num == val->value)
            return val->name;

    return NULL;
}

LUV_LIBAPI int luv_protocol_str2int(const char *string) {
    if (string == NULL || string[0] == '\0')
        return -1;

    struct protoent *proto = getprotobyname(string);
    if (proto == NULL)
        return -1;

    return proto->p_proto;
}

LUV_LIBAPI const char *luv_protocol_int2str(int num) {
    struct protoent *proto = getprotobynumber(num);
    if (proto == NULL)
        return NULL;

    return proto->p_name;
}
