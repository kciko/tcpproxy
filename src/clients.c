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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "clients.h"
#include "tcp.h"
#include "log.h"

void clients_delete_element(void* e)
{
  if(!e)
    return;
  
  client_t* element = (client_t*)e;
  close(element->fd_[0]);
//  close(element->fd_[1]);

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

int clients_add(clients_t* list, int fd, const tcp_endpoint_t* remote_end)
{

  if(!list)
    return -1;

  client_t* element = malloc(sizeof(client_t));
  if(!element) {
    close(fd);
    return -2;
  }

  element->fd_[0] = fd;
  element->fd_[1] = 0;
// TODO: open new socket
//  element->fd_[1] = socket(...);

  if(slist_add(list, element) == NULL) {
    close(element->fd_[0]);
//    close(element->fd_[1]);
    free(element);
    return -2;
  }

  return 0;
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
    client_t* c = (client_t*)tmp->data_;
    if(c && (c->fd_[0] == fd || c->fd_[1] == fd))
      return c;
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
    client_t* c = (client_t*)tmp->data_;
    if(c) {
          // print useful info
      printf("client %d <-> %d: tba...\n", c->fd_[0], c->fd_[1]);
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
    client_t* c = (client_t*)tmp->data_;
    if(c) {
      FD_SET(c->fd_[0], set);
//      FD_SET(c->fd_[1], set);
      *max_fd = *max_fd > c->fd_[0] ? *max_fd : c->fd_[0];
//      *max_fd = *max_fd > c->fd_[1] ? *max_fd : c->fd_[1];
    }
    tmp = tmp->next_;
  }
}

void clients_write_fds(clients_t* list, fd_set* set, int* max_fd)
{
  if(!list)
    return;

      // TODO: add all clients with pending data
}

int clients_read(clients_t* list, fd_set* set)
{
  if(!list)
    return -1;
  
  slist_element_t* tmp = list->first_;
  while(tmp) {
    client_t* c = (client_t*)tmp->data_;
    tmp = tmp->next_;
    if(c) {
      int in, out;
      if(FD_ISSET(c->fd_[0], set)) {
        in = 0;
        out = 1;
      }
      else if(FD_ISSET(c->fd_[1], set)) {
        in = 1;
        out = 0;
      }
      else continue;

      u_int8_t* buffer[1024];
      int len = recv(c->fd_[in], buffer, sizeof(buffer), 0);
      if(len < 0) {
        log_printf(INFO, "Error on recv(): %s, removing client %d", strerror(errno), c->fd_[0]);
        slist_remove(list, c);
      }
      else if(!len) {
        log_printf(INFO, "client %d closed connection", c->fd_[0]);
        slist_remove(list, c);
      }       
      else {
        log_printf(INFO, "client %d: read %d bytes", c->fd_[0], len);
            // TODO: add data to write buffer of l->fd_[out]
      }
    }
  }
  
  return 0;
}

int clients_write(clients_t* list, fd_set* set)
{
  return 0;
}
