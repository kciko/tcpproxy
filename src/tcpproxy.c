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

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "datatypes.h"
#include "options.h"
#include "string_list.h"
#include "log.h"
#include "daemon.h"

#include "listener.h"

int main_loop(options_t* opt)
{
  log_printf(INFO, "entering main loop");

  int return_value = 0;
  int sig_fd = signal_init();
  if(sig_fd < 0)
    return_value -1;

  fd_set readfds, readyfds;
  FD_ZERO(&readfds);
  FD_SET(sig_fd, &readfds);
  int nfds = (nfds < sig_fd) ? sig_fd : nfds;

  while(!return_value) {
    memcpy(&readyfds, &readfds, sizeof(readyfds));
    int ret = select(nfds + 1, &readyfds, NULL, NULL, NULL);
    if(ret == -1 && errno != EINTR) {
      log_printf(ERROR, "select returned with error: %s", strerror(errno));
      return_value = -1;
      break;
    }
    if(!ret || ret == -1)
      continue;

    if(FD_ISSET(sig_fd, &readyfds)) {
      if(signal_handle()) {
        return_value = 1;
        break;
      }
    }
  }

  signal_stop();
  return return_value;
}

int main(int argc, char* argv[])
{
  log_init();

  options_t opt;
  int ret = options_parse(&opt, argc, argv);
  if(ret) {
    if(ret > 0) {
      fprintf(stderr, "syntax error near: %s\n\n", argv[ret]);
    }
    if(ret == -2) {
      fprintf(stderr, "memory error on options_parse, exitting\n");
    }
    if(ret == -3) {
      options_print_version();
    }

    if(ret != -2 && ret != -3) 
      options_print_usage();

    if(ret == -1 || ret == -3)
      ret = 0;

    options_clear(&opt);
    log_close();
    exit(ret);
  }
  slist_element_t* tmp = opt.log_targets_.first_;
  while(tmp) {
    ret = log_add_target(tmp->data_);
    if(ret) {
      switch(ret) {
      case -2: fprintf(stderr, "memory error on log_add_target, exitting\n"); break;
      case -3: fprintf(stderr, "unknown log target: '%s', exitting\n", (char*)(tmp->data_)); break;
      case -4: fprintf(stderr, "this log target is only allowed once: '%s', exitting\n", (char*)(tmp->data_)); break;
      default: fprintf(stderr, "syntax error near: '%s', exitting\n", (char*)(tmp->data_)); break;
      }
      
      options_clear(&opt);
      log_close();
      exit(ret);
    }
    tmp = tmp->next_;
  }

  log_printf(NOTICE, "just started...");
  options_parse_post(&opt);

  priv_info_t priv;
  if(opt.username_)
    if(priv_init(&priv, opt.username_, opt.groupname_)) {
      options_clear(&opt);
      log_close();
      exit(-1);
    }

  FILE* pid_file = NULL;
  if(opt.pid_file_) {
    pid_file = fopen(opt.pid_file_, "w");
    if(!pid_file) {
      log_printf(WARNING, "unable to open pid file: %s", strerror(errno));
    }
  }

  if(opt.chroot_dir_)
    if(do_chroot(opt.chroot_dir_)) {
      options_clear(&opt);
      log_close();
      exit(-1);
    }
  if(opt.username_)
    if(priv_drop(&priv)) {
      options_clear(&opt);
      log_close();
      exit(-1);
    }  

  if(opt.daemonize_) {
    pid_t oldpid = getpid();
    daemonize();
    log_printf(INFO, "running in background now (old pid: %d)", oldpid);
  }

  if(pid_file) {
    pid_t pid = getpid();
    fprintf(pid_file, "%d", pid);
    fclose(pid_file);
  }

  ret = main_loop(&opt);

  options_clear(&opt);

  if(!ret)
    log_printf(NOTICE, "normal shutdown");
  else if(ret < 0)
    log_printf(NOTICE, "shutdown after error");
  else
    log_printf(NOTICE, "shutdown after signal");

  log_close();

	return ret;
}
