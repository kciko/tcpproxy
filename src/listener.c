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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "listener.h"
#include "tcp.h"

void listener_delete_element(void* e)
{
  if(!e)
    return;
  
  listener_t* element = (listener_t*)e;
  close(element->fd_);

  free(e);
}

int listener_init(listeners_t* list)
{
  return slist_init(list, &listener_delete_element);
}

void listener_clear(listeners_t* list)
{
  slist_clear(list);
}

int listener_add(listeners_t* list, char* laddr, char* lport, char* raddr, char* rport)
{
  if(!list)
    return -1;
 
  listener_t* element = malloc(sizeof(listener_t));
  if(!element)
    return -2;
  
// TODO: open listen socket and resolv local and remote address

  static int fds = 6;

  element->fd_ = fds++;
  memset(&(element->local_end_), 0, sizeof(element->local_end_));
  memset(&(element->remote_end_), 0, sizeof(element->remote_end_));

  if(slist_add(list, element) == NULL)
    return -2;

  return 0;
}

void listener_remove(listeners_t* list, int fd)
{
  slist_remove(list, listener_find(list, fd));
}

listener_t* listener_find(listeners_t* list, int fd)
{
  if(!list)
    return NULL;

  slist_element_t* tmp = list->first_;
  while(tmp) {
    listener_t* l = (listener_t*)tmp->data_;
    if(l && l->fd_ == fd)
      return l;
    tmp = tmp->next_;
  }

  return NULL;
}

void listener_print(listeners_t* list)
{
  if(!list)
    return;
  
  slist_element_t* tmp = list->first_;
  while(tmp) {
    listener_t* l = (listener_t*)tmp->data_;
    if(l) {
      char* ls = tcp_endpoint_to_string(l->local_end_);
      char* rs = tcp_endpoint_to_string(l->remote_end_);
      printf("listener #%d: %s -> %s\n", l->fd_, ls ? ls : "(null)", rs ? rs : "(null)");
      if(ls) free(ls);
      if(rs) free(rs);
    }
    tmp = tmp->next_;
  }
}
