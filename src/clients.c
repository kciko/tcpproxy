/*
 *  tcpproxy
 *
 *  tcpproxy is a simple tcp connection proxy which combines the
 *  features of rinetd and 6tunnel. tcpproxy supports IPv4 and
 *  IPv6 and also supports connections from IPv6 to IPv4
 *  endpoints and vice versa.
 *
 *
 *  Copyright (C) 2010-2015 Christian Pointner <equinox@spreadspace.org>
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
#include <netinet/tcp.h>
#include <fcntl.h>

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
  if(element->write_buf_[0].buf_)
    free(element->write_buf_[0].buf_);
  if(element->write_buf_[1].buf_)
    free(element->write_buf_[1].buf_);

  free(e);
}

int clients_init(clients_t* list, int32_t buffer_size)
{
  list->buffer_size_ = buffer_size;
  return slist_init(&(list->list_), &clients_delete_element);
}

void clients_clear(clients_t* list)
{
  slist_clear(&(list->list_));
}

static int handle_connect(client_t* c, int32_t buffer_size_)
{
  if(!c || c->state_ != CONNECTING)
    return -1;

  int error = 0;
  socklen_t len = sizeof(error);
  if(getsockopt(c->fd_[1], SOL_SOCKET, SO_ERROR, &error, &len)==-1) {
    log_printf(ERROR, "Error on getsockopt(): %s", strerror(errno));
    return -1;
  }
  if(error) {
    log_printf(ERROR, "Error on connect(): %s, not adding client %d", strerror(error), c->fd_[0]);
    return -1;
  }

  int i;
  for(i = 0; i < 2; ++i) {
    c->write_buf_[i].buf_ = malloc(buffer_size_);
    if(!c->write_buf_[i].buf_) return -2;
    c->write_buf_[i].length_ = buffer_size_;
    c->write_buf_offset_[i] = 0;
    c->transferred_[i] = 0;
  }

  log_printf(INFO, "successfully added client %d", c->fd_[0]);
  c->state_ = CONNECTED;
  c->fd_state_[1] = ESTABLISHED;
  return 0;
}

int clients_add(clients_t* list, int fd, const tcp_endpoint_t remote_end, const tcp_endpoint_t source_end)
{
  if(!list)
    return -1;

  client_t* element = malloc(sizeof(client_t));
  if(!element) {
    close(fd);
    return -2;
  }

  int i;
  for(i = 0; i < 2; ++i) {
    element->write_buf_[i].buf_ = NULL;
    element->write_buf_[i].length_ = 0;
    element->write_buf_offset_[i] = 0;
  }
  element->state_ = CONNECTING;
  element->fd_[0] = fd;
  element->fd_state_[0] = ESTABLISHED;
  element->fd_[1] = socket(remote_end.addr_.ss_family, SOCK_STREAM, 0);
  if(element->fd_[1] < 0) {
    log_printf(INFO, "Error on socket(): %s, not adding client %d", strerror(errno), element->fd_[0]);
    close(element->fd_[0]);
    free(element);
    return -1;
  }
  element->fd_state_[1] = ESTABLISHING;

  int on = 1;
  if(setsockopt(element->fd_[0], IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) ||
     setsockopt(element->fd_[1], IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on))) {
    log_printf(ERROR, "Error on setsockopt(): %s", strerror(errno));
    close(element->fd_[0]);
    close(element->fd_[1]);
    free(element);
    return -1;
  }

  if(fcntl(element->fd_[0], F_SETFL, O_NONBLOCK) ||
     fcntl(element->fd_[1], F_SETFL, O_NONBLOCK)) {
    log_printf(ERROR, "Error on fcntl(): %s", strerror(errno));
    close(element->fd_[0]);
    close(element->fd_[1]);
    free(element);
    return -1;
  }

  if(source_end.addr_.ss_family != AF_UNSPEC) {
    if(bind(element->fd_[1], (struct sockaddr *)&(source_end.addr_), source_end.len_)==-1) {
      log_printf(INFO, "Error on bind(): %s, not adding client %d", strerror(errno), element->fd_[0]);
      close(element->fd_[0]);
      close(element->fd_[1]);
      free(element);
      return -1;
    }
  }

  if(slist_add(&(list->list_), element) == NULL) {
    close(element->fd_[0]);
    close(element->fd_[1]);
    free(element);
    return -2;
  }

  if(connect(element->fd_[1], (struct sockaddr *)&(remote_end.addr_), remote_end.len_)==-1) {
    if(errno == EINPROGRESS)
      return 0;

    log_printf(INFO, "Error on connect(): %s, not adding client %d", strerror(errno), element->fd_[0]);
    slist_remove(&(list->list_), element);
    return -1;
  }

  log_printf(DEBUG, "connect() for client %d returned immediatly", element->fd_[0]);

  int ret = handle_connect(element, list->buffer_size_);
  if(ret)
    slist_remove(&(list->list_), element);

  return ret;
}

void clients_remove(clients_t* list, int fd)
{
  slist_remove(&(list->list_), clients_find(list, fd));
}

client_t* clients_find(clients_t* list, int fd)
{
  if(!list)
    return NULL;

  slist_element_t* tmp = list->list_.first_;
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

  slist_element_t* tmp = list->list_.first_;
  while(tmp) {
    client_t* c = (client_t*)tmp->data_;
    if(c) {
      char state = '?';
      switch(c->state_) {
      case CONNECTING: state = '>'; break;
      case CONNECTED: state = 'c'; break;
      case CLOSING: state = '-'; break;
      }
      log_printf(NOTICE, "[%c] client #%d/%d: %lld bytes received, %lld bytes sent", state, c->fd_[0], c->fd_[1], c->transferred_[0], c->transferred_[1]);
    }
    tmp = tmp->next_;
  }
}

void clients_read_fds(clients_t* list, fd_set* set, int* max_fd)
{
  if(!list)
    return;

  slist_element_t* tmp = list->list_.first_;
  while(tmp) {
    client_t* c = (client_t*)tmp->data_;
    if(c && (c->state_ == CONNECTED || c->state_ == CLOSING)) {
      if(c->fd_state_[0] != CLOSE_PENDING && c->fd_state_[0] != CLOSE_LINGER
         && c->write_buf_offset_[1] < c->write_buf_[1].length_) {
        FD_SET(c->fd_[0], set);
        *max_fd = *max_fd > c->fd_[0] ? *max_fd : c->fd_[0];

        log_printf(DEBUG, "adding %d for READ", c->fd_[0]);
      }
      if(c->fd_state_[1] != CLOSE_PENDING && c->fd_state_[1] != CLOSE_LINGER
         && c->write_buf_offset_[0] < c->write_buf_[0].length_) {
        FD_SET(c->fd_[1], set);
        *max_fd = *max_fd > c->fd_[1] ? *max_fd : c->fd_[1];

        log_printf(DEBUG, "adding %d for READ", c->fd_[1]);
      }
    }
    tmp = tmp->next_;
  }
}

void clients_write_fds(clients_t* list, fd_set* set, int* max_fd)
{
  if(!list)
    return;

  slist_element_t* tmp = list->list_.first_;
  while(tmp) {
    client_t* c = (client_t*)tmp->data_;
    if(c && (c->state_ == CONNECTED || c->state_ == CLOSING)) {
      if((c->fd_state_[0] == ESTABLISHED || c->fd_state_[0] == CLOSE_PENDING || c->fd_state_[0] == FIN_PENDING)
         && c->write_buf_offset_[0]) {
        FD_SET(c->fd_[0], set);
        *max_fd = *max_fd > c->fd_[0] ? *max_fd : c->fd_[0];

        log_printf(DEBUG, "adding %d for WRITE", c->fd_[0]);
      }
      if((c->fd_state_[1] == ESTABLISHED || c->fd_state_[1] == CLOSE_PENDING || c->fd_state_[1] == FIN_PENDING)
         && c->write_buf_offset_[1]) {
        FD_SET(c->fd_[1], set);
        *max_fd = *max_fd > c->fd_[1] ? *max_fd : c->fd_[1];

        log_printf(DEBUG, "adding %d for WRITE", c->fd_[1]);
      }
    } else if(c && c->state_ == CONNECTING) {
      FD_SET(c->fd_[1], set);
      *max_fd = *max_fd > c->fd_[1] ? *max_fd : c->fd_[1];

      log_printf(DEBUG, "adding %d for WRITE (connecting)", c->fd_[1]);
    }
    tmp = tmp->next_;
  }
}

static char* client_fd_state_to_string(client_fd_state_t s)
{
  switch(s) {
  case ESTABLISHING: return "establishing";
  case ESTABLISHED: return "established";
  case FIN_PENDING: return "FIN pending";
  case FIN_LINGER: return "FIN linger";
  case CLOSE_PENDING: return "CLOSE pending";
  case CLOSE_LINGER: return "CLOSE linger";
  }
}

static int client_handle_close(client_t* c, int in, int out)
{
  log_printf(DEBUG, "client %d: recv(%d) returned 0, %d: bytes=%d state=%s, %d: bytes=%d state=%s", c->fd_[0], c->fd_[in],
             c->fd_[0], c->write_buf_offset_[0], client_fd_state_to_string(c->fd_state_[0]),
             c->fd_[1], c->write_buf_offset_[1], client_fd_state_to_string(c->fd_state_[1]));

  c->state_ = CLOSING;
  c->fd_state_[in] = (c->write_buf_offset_[in]) ? CLOSE_PENDING : CLOSE_LINGER;

  switch(c->fd_state_[out]) {
  case ESTABLISHING: return 1; // this will never be reached - remove client for now > assert() this in future??
  case ESTABLISHED: {
    if(c->write_buf_offset_[out]) {
      c->fd_state_[out] = FIN_PENDING;
    } else {
      c->fd_state_[out] = FIN_LINGER;
      shutdown(c->fd_[out], SHUT_WR);
    }
    break;
  }
  case FIN_PENDING: log_printf(ERROR, "client %d: socket %d is already in FIN_PENDING state???", c->fd_[0], c->fd_[out]); break;
  case FIN_LINGER: log_printf(ERROR, "client %d: socket %d is already in FIN_LINGER state???", c->fd_[0], c->fd_[out]); break;
  case CLOSE_PENDING: log_printf(DEBUG, "client %d: socket %d has data pending - removal postponed", c->fd_[0], c->fd_[out]); break;
  case CLOSE_LINGER: log_printf(INFO, "client %d: both connections are closed now - removing it", c->fd_[0]); return 1;
  }

  log_printf(DEBUG, "client %d is now               %d: bytes=%d state=%s, %d: bytes=%d state=%s", c->fd_[0],
             c->fd_[0], c->write_buf_offset_[0], client_fd_state_to_string(c->fd_state_[0]),
             c->fd_[1], c->write_buf_offset_[1], client_fd_state_to_string(c->fd_state_[1]));

  return 0;
}

int clients_read(clients_t* list, fd_set* set)
{
  if(!list)
    return -1;

  slist_element_t* tmp = list->list_.first_;
  while(tmp) {
    client_t* c = (client_t*)tmp->data_;
    tmp = tmp->next_;
    if(c && (c->state_ == CONNECTED || c->state_ == CLOSING)) {
      int i;
      for(i=0; i<2; ++i) {
        int in, out;
        if(FD_ISSET(c->fd_[i], set)) {
          in = i;
          out = i ^ 1;
        }
        else continue;

        log_printf(DEBUG, "calling recv(%d)", c->fd_[in]);
        int len = recv(c->fd_[in], &(c->write_buf_[out].buf_[c->write_buf_offset_[out]]),  c->write_buf_[out].length_ - c->write_buf_offset_[out], 0);
        if(len < 0) {
              // TODO: the other socket might still have data pending....
          log_printf(INFO, "Error on recv(): %s, removing client %d", strerror(errno), c->fd_[0]);
          slist_remove(&(list->list_), c);
          break;
        }
        else if(!len) {
          if(client_handle_close(c, in, out)) {
            slist_remove(&(list->list_), c);
            break;
          }
        }
        else
          c->write_buf_offset_[out] += len;
      }
    }
  }

  return 0;
}

static int client_handle_pending(client_t* c, int i)
{
  log_printf(DEBUG, "client %d: cleared write buffer[%d], %d: bytes=%d state=%s, %d: bytes=%d state=%s", c->fd_[0], c->fd_[i],
             c->fd_[0], c->write_buf_offset_[0], client_fd_state_to_string(c->fd_state_[0]),
             c->fd_[1], c->write_buf_offset_[1], client_fd_state_to_string(c->fd_state_[1]));

  if(c->fd_state_[i] == CLOSE_PENDING) {
    c->fd_state_[i] = CLOSE_LINGER;
  } else {
    c->fd_state_[i] = FIN_LINGER;
  }
  shutdown(c->fd_[i], SHUT_WR);

  log_printf(DEBUG, "client %d is now          %d: bytes=%d state=%s, %d: bytes=%d state=%s", c->fd_[0], c->fd_[i],
             c->fd_[0], c->write_buf_offset_[0], client_fd_state_to_string(c->fd_state_[0]),
             c->fd_[1], c->write_buf_offset_[1], client_fd_state_to_string(c->fd_state_[1]));

  return 0;
}

int clients_write(clients_t* list, fd_set* set)
{
  if(!list)
    return -1;

  slist_element_t* tmp = list->list_.first_;
  while(tmp) {
    client_t* c = (client_t*)tmp->data_;
    tmp = tmp->next_;
    if(c && (c->state_ == CONNECTED || c->state_ == CLOSING)) {
      int i;
      for(i=0; i<2; ++i) {
        if(FD_ISSET(c->fd_[i], set)) {
          log_printf(DEBUG, "calling send(%d)", c->fd_[i]);
          int len = send(c->fd_[i], c->write_buf_[i].buf_, c->write_buf_offset_[i], 0);
          if(len < 0) {
                // TODO: the other socket might still have data pending....
            log_printf(INFO, "Error on send(): %s, removing client %d", strerror(errno), c->fd_[0]);
            slist_remove(&(list->list_), c);
            break;
          }
          else {
            c->transferred_[i] += len;
            if(c->write_buf_offset_[i] > len) {
              memmove(c->write_buf_[i].buf_, &c->write_buf_[i].buf_[len], c->write_buf_offset_[i] - len);
              c->write_buf_offset_[i] -= len;
            }
            else {
              c->write_buf_offset_[i] = 0;
              if(c->fd_state_[i] == CLOSE_PENDING || c->fd_state_[i] == FIN_PENDING)
                if(client_handle_pending(c, i))
                  slist_remove(&(list->list_), c);
            }
          }
        }
      }
    } else if(c && c->state_ == CONNECTING && FD_ISSET(c->fd_[1], set)) {
      int ret = handle_connect(c, list->buffer_size_);
      if(ret)
        slist_remove(&(list->list_), c);
    }
  }

  return 0;
}
