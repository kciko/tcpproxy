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

#include "datatypes.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
/* #include <netdb.h> */
/* #include <sys/types.h> */
/* #include <sys/socket.h> */
/* #include <arpa/inet.h> */
/* #include <netinet/in.h> */
/* #include <sys/select.h> */

#include "clients.h"
#include "tcp.h"
#include "log.h"

void clients_delete_element(void* e)
{
  if(!e)
    return;
  
  client_t* element = (client_t*)e;
  close(element->fd_);

  free(e);
}

int clients_init(clients_t* list)
{
  return slist_init(list, &clients_delete_element);
}

void clients_clear(clients_t* list)
{
  slist_clear(list);
}

int clients_add(clients_t* list, int fd, tcp_endpoint_t remote_end)
{
  if(!list)
    return -1;

  int ret = 0;

// TODO: connect to remote end and setup write buffers

  return ret;
}

void clients_remove(clients_t* list, int fd)
{
  slist_remove(list, clients_find(list, fd));
}

client_t* clients_find(clients_t* list, int fd)
{
  if(!list)
    return NULL;

  slist_element_t* tmp = list->first_;
  while(tmp) {
    client_t* l = (client_t*)tmp->data_;
    if(l && l->fd_ == fd)
      return l;
    tmp = tmp->next_;
  }

  return NULL;
}

void clients_print(clients_t* list)
{
  if(!list)
    return;
  
  slist_element_t* tmp = list->first_;
  while(tmp) {
    client_t* l = (client_t*)tmp->data_;
    if(l) {
          // print useful info
      printf("clients #%d: tba...\n", l->fd_);
    }
    tmp = tmp->next_;
  }
}

void clients_read_fds(clients_t* list, fd_set* set, int* max_fd)
{
  if(!list)
    return;

  slist_element_t* tmp = list->first_;
  while(tmp) {
    client_t* l = (client_t*)tmp->data_;
    if(l) {
      FD_SET(l->fd_, set);
      *max_fd = *max_fd > l->fd_ ? *max_fd : l->fd_;
    }
    tmp = tmp->next_;
  }
}

void clients_write_fds(clients_t* list, fd_set* set, int* max_fd)
{
  if(!list)
    return;

      // TODO: add all clients with pending data
/*   slist_element_t* tmp = list->first_; */
/*   while(tmp) { */
/*     client_t* l = (client_t*)tmp->data_; */
/*     if(l) { */
/*       FD_SET(l->fd_, set); */
/*       *max_fd = *max_fd > l->fd_ ? *max_fd : l->fd_; */
/*     } */
/*     tmp = tmp->next_; */
/*   } */
}
