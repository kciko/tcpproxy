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

#ifndef TCPPROXY_listener_h_INCLUDED
#define TCPPROXY_listener_h_INCLUDED

#include "slist.h"
#include "tcp.h"

typedef struct {
  int fd_;
  tcp_endpoint_t local_end_;
  tcp_endpoint_t remote_end_;
} listener_t;

void listener_delete_element(void* e);

typedef slist_t listeners_t;

int listener_init(listeners_t* list);
void listener_clear(listeners_t* list);
int listener_add(listeners_t* list, const char* laddr, const char* lport, const char* raddr, const char* rport);
void listener_remove(listeners_t* list, int fd);
listener_t* listener_find(listeners_t* list, int fd);
void listener_print(listeners_t* list);

#endif
