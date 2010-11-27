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

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "listener.h"
#include "tcp.h"
#include "log.h"

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

int listener_add(listeners_t* list, const char* laddr, const char* lport, const char* raddr, const char* rport)
{
  if(!list)
    return -1;

// TODO: what if more than one address is returned here? 
  struct addrinfo* re = tcp_resolve_endpoint(raddr, rport, ANY);
  if(!re)
    return -1;

  struct addrinfo* le = tcp_resolve_endpoint(laddr, lport, ANY);
  if(!le) {
    freeaddrinfo(re);
    return -1;
  }

  struct addrinfo* l = le;
  int ret = 0;
  while(l) {
    listener_t* element = malloc(sizeof(listener_t));
    if(!element) {
      ret = -2;
      break;
    }
    memset(&(element->remote_end_), 0, sizeof(element->remote_end_));
    memcpy(&(element->remote_end_), re->ai_addr, re->ai_addrlen);

    memset(&(element->local_end_), 0, sizeof(element->local_end_));
    memcpy(&(element->local_end_), l->ai_addr, l->ai_addrlen);

    element->fd_ = socket(l->ai_family, SOCK_STREAM, 0);
    if(element->fd_ < 0) {
      log_printf(ERROR, "Error on opening tcp socket: %s", strerror(errno));
      free(element);
      ret = -1;
      break;
    }

    if(l->ai_family == AF_INET6) {
      int on = 1;
      if(setsockopt(element->fd_, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on)))
        log_printf(WARNING, "failed to set IPV6_V6ONLY socket option: %s", strerror(errno));
    }

    ret = bind(element->fd_, l->ai_addr, l->ai_addrlen);
    if(ret) {
      log_printf(ERROR, "Error on bind(): %s", strerror(errno));
      close(element->fd_);
      free(element);
      break;
    }

    ret = listen(element->fd_, 0);
    if(ret) {
      log_printf(ERROR, "Error on listen(): %s", strerror(errno));
      close(element->fd_);
      free(element);
      break;
    }
  
    char* ls = tcp_endpoint_to_string(element->local_end_);
    char* rs = tcp_endpoint_to_string(element->remote_end_);
    log_printf(NOTICE, "listening on: %s (remote: %s)", ls ? ls:"(null)", rs ? rs:"(null)");
    if(ls) free(ls);
    if(rs) free(rs);

    if(slist_add(list, element) == NULL) {
      close(element->fd_);
      free(element);
      ret = -2;
      break;
    }

    l = l->ai_next;
  }
  freeaddrinfo(re);
  freeaddrinfo(le);

  return ret;
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
