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

#ifndef TCPPROXY_clients_h_INCLUDED
#define TCPPROXY_clients_h_INCLUDED

#include <sys/select.h>

#include "slist.h"
#include "tcp.h"

#define BUFFER_LENGTH 1048576

typedef struct {
  int fd_[2];
  u_int8_t write_buf_[2][BUFFER_LENGTH];
  u_int32_t write_buf_len_[2];
} client_t;

void clients_delete_element(void* e);

typedef slist_t clients_t;

int clients_init(clients_t* list);
void clients_clear(clients_t* list);
int clients_add(clients_t* list, int fd, const tcp_endpoint_t remote_end, const tcp_endpoint_t source_end);
void clients_remove(clients_t* list, int fd);
client_t* clients_find(clients_t* list, int fd);
void clients_print(clients_t* list);

void clients_read_fds(clients_t* list, fd_set* set, int* max_fd);
void clients_write_fds(clients_t* list, fd_set* set, int* max_fd);

int clients_read(clients_t* list, fd_set* set);
int clients_write(clients_t* list, fd_set* set);

#endif