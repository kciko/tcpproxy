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
#include <netinet/in.h>

#include "clients.h"
#include "tcp.h"
#include "log.h"

void clients_delete_element(void* e)
{
  if(!e)
    return;
  
  client_t* element = (client_t*)e;
  close(element->fd_[0]);
  close(element->fd_[1]);

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

int clients_add(clients_t* list, int fd, const tcp_endpoint_t* remote_end, const tcp_endpoint_t* source_end)
{

  if(!list)
    return -1;

  client_t* element = malloc(sizeof(client_t));
  if(!element) {
    close(fd);
    return -2;
  }

  element->write_buf_len_[0] = 0;
  element->write_buf_len_[1] = 0;
  element->fd_[0] = fd;

  element->fd_[1] = socket(remote_end->ss_family, SOCK_STREAM, 0);
  if(element->fd_[1] < 0) { 
    log_printf(INFO, "Error on socket(): %s, not adding client %d", strerror(errno), element->fd_[0]);
    close(element->fd_[0]);
    free(element);
    return -1;
  }

  if(source_end->ss_family != AF_UNSPEC) {
    socklen_t socklen = sizeof(*source_end);
    if(source_end->ss_family == AF_INET)
      socklen = sizeof(struct sockaddr_in);
    else if (source_end->ss_family == AF_INET6)
      socklen = sizeof(struct sockaddr_in6);

    if(bind(element->fd_[1], (struct sockaddr *)source_end, socklen)==-1) { 
      log_printf(INFO, "Error on bind(): %s, not adding client %d", strerror(errno), element->fd_[0]);
      close(element->fd_[0]);
      close(element->fd_[1]);
      free(element);
      return -1;
    }
  }

  socklen_t socklen = sizeof(*remote_end);
  if(remote_end->ss_family == AF_INET)
    socklen = sizeof(struct sockaddr_in);
  else if (remote_end->ss_family == AF_INET6)
    socklen = sizeof(struct sockaddr_in6);
    
  if(connect(element->fd_[1], (struct sockaddr *)remote_end, socklen)==-1) { 
    log_printf(INFO, "Error on connect(): %s, not adding client %d", strerror(errno), element->fd_[0]);
    close(element->fd_[0]);
    close(element->fd_[1]);
    free(element);
    return -1;
  }

  if(slist_add(list, element) == NULL) {
    close(element->fd_[0]);
    close(element->fd_[1]);
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
      FD_SET(c->fd_[1], set);
      *max_fd = *max_fd > c->fd_[0] ? *max_fd : c->fd_[0];
      *max_fd = *max_fd > c->fd_[1] ? *max_fd : c->fd_[1];
    }
    tmp = tmp->next_;
  }
}

void clients_write_fds(clients_t* list, fd_set* set, int* max_fd)
{
  if(!list)
    return;

  slist_element_t* tmp = list->first_;
  while(tmp) {
    client_t* c = (client_t*)tmp->data_;
    if(c) {
      if(c->write_buf_len_[0]) {
        FD_SET(c->fd_[0], set);
        *max_fd = *max_fd > c->fd_[0] ? *max_fd : c->fd_[0];
      }
      if(c->write_buf_len_[1]) {
        FD_SET(c->fd_[1], set);
        *max_fd = *max_fd > c->fd_[1] ? *max_fd : c->fd_[1];
      }
    }
    tmp = tmp->next_;
  }
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

          // TODO: what when buffer is full? 
      int len = recv(c->fd_[in], &(c->write_buf_[out][c->write_buf_len_[out]]), BUFFER_LENGTH - c->write_buf_len_[out], 0);
      if(len < 0) {
        log_printf(INFO, "Error on recv(): %s, removing client %d", strerror(errno), c->fd_[0]);
        slist_remove(list, c);
      }
      else if(!len) {
        log_printf(INFO, "client %d closed connection, removing it", c->fd_[0]);
        slist_remove(list, c);
      }       
      else
        c->write_buf_len_[out] += len;
    }
  }
  
  return 0;
}

int clients_write(clients_t* list, fd_set* set)
{
  if(!list)
    return -1;
  
  slist_element_t* tmp = list->first_;
  while(tmp) {
    client_t* c = (client_t*)tmp->data_;
    tmp = tmp->next_;
    if(c) {
      int i;
      for(i=0; i<2; ++i) {
        if(FD_ISSET(c->fd_[i], set)) {
          int len = send(c->fd_[i], c->write_buf_[i], c->write_buf_len_[i], 0);
          if(len < 0) {
            log_printf(INFO, "Error on send(): %s, removing client %d", strerror(errno), c->fd_[0]);
            slist_remove(list, c);
          }
          else {
            if(c->write_buf_len_[i] > len) {
              memmove(c->write_buf_[i], &c->write_buf_[i][len], c->write_buf_len_[i] - len);
              c->write_buf_len_[i] -= len;
            }
            else
              c->write_buf_len_[i] = 0;
          }
        }
      }
    }
  }

  return 0;
}
