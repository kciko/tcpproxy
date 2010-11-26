/*
 *  tcpproxy
 *
 *  tcpproxy is a simple tcp connection proxy which combines the 
 *  features of rinetd and 6tunnel. tcpproxy supports IPv4 and 
 *  IPv6 and also supports connections from IPv6 to IPv4 
 *  endpoints and vice versa.
 *  
 *
 *  Copyright (C) 2010-2011 Christian Pointner <equinox@spreadspace.org>
 *                         
 *  This file is part of tcpproxy.
 *
 *  tcpproxy is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  any later version.
 *
 *  tcpproxy is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with tcpproxy. If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include "datatypes.h"

#include "tcp.h"

char* tcp_endpoint_to_string(tcp_endpoint_t e)
{
  void* ptr;
  u_int16_t port;
  size_t addrstr_len = 0;
  char* addrstr, *ret;
  char addrport_sep = ':';
  
  switch(((struct sockaddr *)&e)->sa_family)
  {
  case AF_INET:
    ptr = &((struct sockaddr_in *)&e)->sin_addr;
    port = ntohs(((struct sockaddr_in *)&e)->sin_port);
    addrstr_len = INET_ADDRSTRLEN + 1;
    addrport_sep = ':';
    break;
  case AF_INET6:
    ptr = &((struct sockaddr_in6 *)&e)->sin6_addr;
    port = ntohs(((struct sockaddr_in6 *)&e)->sin6_port);
    addrstr_len = INET6_ADDRSTRLEN + 1;
    addrport_sep = '.';
    break;
  default:
    asprintf(&ret, "unknown address type");
    return ret;
  }
  addrstr = malloc(addrstr_len);
  if(!addrstr)
    return NULL;
  inet_ntop (((struct sockaddr *)&e)->sa_family, ptr, addrstr, addrstr_len);
  asprintf(&ret, "%s%c%d", addrstr, addrport_sep ,port);
  free(addrstr);
  return ret;
}