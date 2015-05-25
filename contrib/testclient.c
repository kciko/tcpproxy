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

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
  if(argc < 3) {
    fprintf(stderr, "Usage: %s <addr> <port>\n", argv[0]);
    return -1;
  }

  int c = socket(AF_INET, SOCK_STREAM, 0);
  if(!c) {
    perror("socket()");
    return -1;
  }

  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(atoi(argv[2]));
  int ret = inet_pton(AF_INET, argv[1], &(addr.sin_addr));
  if (ret <= 0) {
    if (ret == 0)
      fprintf(stderr, "inet_pton(): Not in presentation format\n");
    else
      perror("inet_pton()");
    return -1;
  }

  ret = connect(c, (struct sockaddr*)(&addr), sizeof(struct sockaddr_in));
  if(ret) {
    perror("connect()");
    return -1;
  }

  char buf[1234567];
  unsigned int i;
  for(i = 0; i<sizeof(buf); ++i) {
    buf[i] = 'A' + i%62;
  }
  buf[sizeof(buf)-1] = '\n';

  int nbwritten = send(c, buf, sizeof(buf), 0);
  if(nbwritten <= 0) {
    if(nbwritten < 0)
      perror("send()");
    else
      fprintf(stderr, "nothing sent... aborting\n");
    return -1;
  }
  printf("%d bytes sent\n", nbwritten);
  if(nbwritten != sizeof(buf)) {
    fprintf(stderr, "to few bytes sent ... aborting!");
    return -1;
  }

  shutdown(c, SHUT_WR);

  int len = 0;
  for(;;) {
    int nbread = recv(c, &(buf[len]), sizeof(buf) - len, 0);
    if(nbread <= 0) {
      if(!nbread) {
        fprintf(stderr, "connection closed\n");
      } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
        perror("recv()");
      } else
        continue;

      return -1;
    }

    printf("%d bytes received", nbread);
    len += nbread;
    if(len == nbwritten) {
      printf(" .. finished\n");
      for(i = 0; i<nbwritten-1; ++i) {
        if(buf[i] != ('A' + i%62)) {
          fprintf(stderr, "error at index %d (expected 0x%02X, got 0x%02X)\n", i, 'A' + i%62, (unsigned int)(buf[i]));
          return -1;
        }
      }
      if(buf[nbwritten-1] != '\n') {
        fprintf(stderr, "string doesn't end with new-line\n");
        return -1;
      }
      break;
    }

    printf("\n");
  }

  while(recv(c, buf, 1, 0) > 0);

  return 0;
}
