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
#include "dns.h"

#ifndef WIN32
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#else
#include <ws2def.h>
#endif

#include <lauxlib.h>
#include <lua.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "constants.h"
#include "luv.h"
#include "private.h"
#include "request.h"
#include "util.h"

LUV_LIBAPI luaL_Reg luv_dns_functions[] = {
  {"getaddrinfo", luv_getaddrinfo},
  {"getnameinfo", luv_getnameinfo},
  {NULL, NULL},
};

LUV_LIBAPI void luv_pushaddrinfo(lua_State *const L, const struct addrinfo *const res) {
  char ip_buffer[INET6_ADDRSTRLEN];
  lua_Integer count = 0;

  lua_newtable(L);
  for (const struct addrinfo *curr = res; curr != NULL; curr = curr->ai_next) {
    if (curr->ai_family == AF_INET || curr->ai_family == AF_INET6) {
      lua_createtable(L, 0, 6);

      if (curr->ai_family == AF_INET) {
        struct sockaddr_in *const addr_in = (struct sockaddr_in *)curr->ai_addr;

        uv_inet_ntop(curr->ai_family, &addr_in->sin_addr, ip_buffer, INET6_ADDRSTRLEN);

        lua_pushstring(L, ip_buffer);
        lua_setfield(L, -2, "addr");

        const int port = ntohs(addr_in->sin_port);
        if (port != 0) {
          lua_pushinteger(L, port);
          lua_setfield(L, -2, "port");
        }
      } else {
        struct sockaddr_in6 *const addr_in = (struct sockaddr_in6 *)curr->ai_addr;

        uv_inet_ntop(curr->ai_family, &addr_in->sin6_addr, ip_buffer, INET6_ADDRSTRLEN);

        lua_pushstring(L, ip_buffer);
        lua_setfield(L, -2, "addr");

        const int port = ntohs(addr_in->sin6_port);
        if (port != 0) {
          lua_pushinteger(L, port);
          lua_setfield(L, -2, "port");
        }
      }

      lua_pushstring(L, luv_addrfamily_int2str(curr->ai_family));
      lua_setfield(L, -2, "family");

      lua_pushstring(L, luv_sockdomain_int2str(curr->ai_socktype));
      lua_setfield(L, -2, "socktype");

      lua_pushstring(L, luv_protocol_int2str(curr->ai_protocol));
      lua_setfield(L, -2, "protocol");

      if (curr->ai_canonname != NULL) {
        lua_pushstring(L, curr->ai_canonname);
        lua_setfield(L, -2, "canonname");
      }

      lua_rawseti(L, -2, ++count);
    }
  }
}

LUV_CBAPI void luv_getaddrinfo_cb(uv_getaddrinfo_t *req, int status, struct addrinfo *res) {
  luv_req_t *lreq = luv_request_from(req);
  lua_State *L = lreq->ctx->L;
  int nargs;

  if (status < 0) {
    luv_status(L, status);
    nargs = 1;
  } else {
    lua_pushnil(L);
    luv_pushaddrinfo(L, res);
    nargs = 2;
  }

  luv_fulfill_req(L, lreq, nargs);
  luv_cleanup_req(L, lreq);
  if (res != NULL) {
    uv_freeaddrinfo(res);
  }
}

LUV_LUAAPI int luv_getaddrinfo(lua_State *const L) {
  const char *node = luaL_optstring(L, 1, NULL);
  const char *service = luaL_optstring(L, 2, NULL);
  struct addrinfo hints_s;
  struct addrinfo *hints = NULL;

  if (!lua_isnoneornil(L, 3)) {
    luaL_checktype(L, 3, LUA_TTABLE);
    hints = &hints_s;

    // Initialize the hints
    memset((void *)hints, 0, sizeof(*hints));

    // Process the `family` hint.
    lua_getfield(L, 3, "family");
    if (lua_isnumber(L, -1) != 0) {
      hints->ai_family = lua_tointeger(L, -1);
    } else if (lua_isstring(L, -1) != 0) {
      hints->ai_family = luv_addrfamily_str2int(lua_tostring(L, -1));
    } else if (lua_isnil(L, -1) != 0) {
      hints->ai_family = AF_UNSPEC;
    } else {
      luaL_argerror(L, 3, "string or number or nil expected for 'family' hint");
    }

    lua_pop(L, 1);

    // Process `socktype` hint
    lua_getfield(L, 3, "socktype");
    if (lua_isnumber(L, -1) != 0) {
      hints->ai_socktype = lua_tointeger(L, -1);
    } else if (lua_isstring(L, -1) != 0) {
      hints->ai_socktype = luv_sockdomain_str2int(lua_tostring(L, -1));
    } else if (lua_isnil(L, -1) == 0) {
      return luaL_argerror(L, 3, "string or number or nil expected for 'socktype' hint");
    }
    lua_pop(L, 1);

    // Process the `protocol` hint
    lua_getfield(L, 3, "protocol");
    if (lua_isnumber(L, -1) != 0) {
      hints->ai_protocol = lua_tointeger(L, -1);
    } else if (lua_isstring(L, -1) != 0) {
      hints->ai_protocol = luv_protocol_str2int(lua_tostring(L, -1));
    } else if (lua_isnil(L, -1) == 0) {
      return luaL_argerror(L, 3, "string or number or nil expected for 'protocol' hint");
    }

    if (hints->ai_protocol < 0) {
      return luaL_argerror(L, 3, lua_pushfstring(L, "invalid protocol: %s", lua_tostring(L, -1)));
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "addrconfig");
    if (lua_toboolean(L, -1) != 0) {
      hints->ai_flags |= AI_ADDRCONFIG;
    }
    lua_pop(L, 1);

#ifdef AI_V4MAPPED
    lua_getfield(L, 3, "v4mapped");
    if (lua_toboolean(L, -1) != 0) {
      hints->ai_flags |= AI_V4MAPPED;
    }
    lua_pop(L, 1);
#endif

#ifdef AI_ALL
    lua_getfield(L, 3, "all");
    if (lua_toboolean(L, -1) != 0) {
      hints->ai_flags |= AI_ALL;
    }
    lua_pop(L, 1);
#endif

    lua_getfield(L, 3, "numerichost");
    if (lua_toboolean(L, -1) != 0) {
      hints->ai_flags |= AI_NUMERICHOST;
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "passive");
    if (lua_toboolean(L, -1) != 0) {
      hints->ai_flags |= AI_PASSIVE;
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "numericserv");
    if (lua_toboolean(L, -1) != 0) {
      hints->ai_flags |= AI_NUMERICSERV;
      /* On OS X upto at least OSX 10.9, getaddrinfo crashes
       * if AI_NUMERICSERV is set and the servname is NULL or "0".
       * This workaround avoids a segfault in libsystem.
       */
      if (service == NULL || strcmp(service, "0") == 0) {
        service = "00";
      }
    }
    lua_pop(L, 1);

    lua_getfield(L, 3, "canonname");
    if (lua_toboolean(L, -1) != 0) {
      hints->ai_flags |= AI_CANONNAME;
    }
    lua_pop(L, 1);
  }

#if !LUV_UV_VERSION_GEQ(1, 3, 0)
  // in libuv < 1.3.0, the callback cannot be NULL
  luaL_argcheck(L, lua_isnoneornil(L, 4) == 0, 4, "callback must be provided");
#endif
  luv_ctx_t *ctx = luv_context(L);
  luv_req_t *lreq = luv_new_request(L, UV_GETADDRINFO, ctx, 4);
  uv_getaddrinfo_t *getaddrinfo = luv_request_of(uv_getaddrinfo_t, lreq);

  uv_getaddrinfo_cb callback = lreq->callback == LUA_NOREF ? NULL : luv_getaddrinfo_cb;
  const int ret = uv_getaddrinfo(ctx->loop, getaddrinfo, callback, node, service, hints);
  if (ret < 0) {
    lua_pop(L, 1);
    luv_cleanup_req(L, lreq);
    return luv_error(L, ret);
  }

#if LUV_UV_VERSION_GEQ(1, 3, 0)
  // synchronous mode
  if (lreq->callback == LUA_NOREF) {
    lua_pop(L, 1);
    luv_pushaddrinfo(L, getaddrinfo->addrinfo);
    uv_freeaddrinfo(getaddrinfo->addrinfo);
    luv_cleanup_req(L, lreq);
  }
#endif

  return 1;
}

LUV_CBAPI void luv_getnameinfo_cb(uv_getnameinfo_t *req, int status, const char *hostname, const char *service) {
  luv_req_t *data = (luv_req_t *)req->data;
  lua_State *L = data->ctx->L;
  int nargs;

  if (status < 0) {
    luv_status(L, status);
    nargs = 1;
  } else {
    lua_pushnil(L);
    lua_pushstring(L, hostname);
    lua_pushstring(L, service);
    nargs = 3;
  }

  luv_fulfill_req(L, (luv_req_t *)req->data, nargs);
  luv_cleanup_req(L, (luv_req_t *)req->data);
  req->data = NULL;
}

LUV_LUAAPI int luv_getnameinfo(lua_State *const L) {
  luaL_checktype(L, 1, LUA_TTABLE);

  const char *ip_str = NULL;
  int port = 0;

  lua_getfield(L, 1, "ip");
  if (lua_isstring(L, -1) != 0) {
    ip_str = lua_tostring(L, -1);
  } else if (lua_isnoneornil(L, -1) == 0) {
    luaL_argerror(L, 1, "string or nil expected in 'ip' field");
  }
  lua_pop(L, 1);

  lua_getfield(L, 1, "port");
  if (lua_isnumber(L, -1) != 0) {
    port = (int)lua_tointeger(L, -1);
  } else if (lua_isnoneornil(L, -1) == 0) {
    luaL_argerror(L, 1, "number or nil expected in 'port' field");
  }
  lua_pop(L, 1);

  struct sockaddr_storage addr;
  memset((void *)&addr, 0, sizeof(addr));

  if ((ip_str != NULL) || (port != 0)) {
    if (ip_str == NULL) {
      ip_str = "0.0.0.0";
    }

    if (uv_ip4_addr(ip_str, port, (struct sockaddr_in *)&addr) == 0) {
      addr.ss_family = AF_INET;
    } else if (uv_ip6_addr(ip_str, port, (struct sockaddr_in6 *)&addr) == 0) {
      addr.ss_family = AF_INET6;
    } else {
      return luaL_argerror(L, 1, "invalid ip address or port");
    }
  }

  lua_getfield(L, 1, "family");
  if (lua_isnumber(L, -1) != 0) {
    addr.ss_family = lua_tointeger(L, -1);
  } else if (lua_isstring(L, -1) != 0) {
    addr.ss_family = luv_addrfamily_str2int(lua_tostring(L, -1));
  } else if (lua_isnil(L, -1) != 0) {
    addr.ss_family = AF_UNSPEC;
  } else {
    luaL_argerror(L, 1, "number or string or nil expected in 'family' field");
  }
  lua_pop(L, 1);

#if !LUV_UV_VERSION_GEQ(1, 3, 0)
  // in libuv < 1.3.0, the callback cannot be NULL
  luaL_argcheck(L, !lua_isnoneornil(L, 2), 2, "callback must be provided");
#endif

  luv_ctx_t *ctx = luv_context(L);
  luv_req_t *lreq = luv_new_request(L, UV_GETNAMEINFO, ctx, 2);
  uv_getnameinfo_t *getnameinfo = luv_request_of(uv_getnameinfo_t, lreq);

  uv_getnameinfo_cb callback = lreq->callback == LUA_NOREF ? NULL : luv_getnameinfo_cb;
  const int ret = uv_getnameinfo(ctx->loop, getnameinfo, callback, (struct sockaddr *)&addr, 0);
  if (ret < 0) {
    luv_cleanup_req(L, lreq);
    lua_pop(L, 1);
    return luv_error(L, ret);
  }

  // synchronous mode
#if LUV_UV_VERSION_GEQ(1, 3, 0)
  if (lreq->callback == LUA_NOREF) {
    lua_pop(L, 1);
    lua_pushstring(L, getnameinfo->host);
    lua_pushstring(L, getnameinfo->service);
    luv_cleanup_req(L, lreq);
    return 2;
  }
#endif

  return 1;
}
