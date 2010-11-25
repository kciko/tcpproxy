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
#include "version.h"

#include "options.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define PARSE_BOOL_PARAM(SHORT, LONG, VALUE)             \
    else if(!strcmp(str,SHORT) || !strcmp(str,LONG))     \
      VALUE = 1;

#define PARSE_INVERSE_BOOL_PARAM(SHORT, LONG, VALUE)     \
    else if(!strcmp(str,SHORT) || !strcmp(str,LONG))     \
      VALUE = 0;

#define PARSE_INT_PARAM(SHORT, LONG, VALUE)              \
    else if(!strcmp(str,SHORT) || !strcmp(str,LONG))     \
    {                                                    \
      if(argc < 1)                                       \
        return i;                                        \
      VALUE = atoi(argv[i+1]);                           \
      argc--;                                            \
      i++;                                               \
    }

#define PARSE_STRING_PARAM(SHORT, LONG, VALUE)           \
    else if(!strcmp(str,SHORT) || !strcmp(str,LONG))     \
    {                                                    \
      if(argc < 1 || argv[i+1][0] == '-')                \
        return i;                                        \
      if(VALUE) free(VALUE);                             \
      VALUE = strdup(argv[i+1]);                         \
      if(!VALUE)                                         \
        return -2;                                       \
      argc--;                                            \
      i++;                                               \
    }

#define PARSE_STRING_PARAM_SEC(SHORT, LONG, VALUE)       \
    else if(!strcmp(str,SHORT) || !strcmp(str,LONG))     \
    {                                                    \
      if(argc < 1 || argv[i+1][0] == '-')                \
        return i;                                        \
      if(VALUE) free(VALUE);                             \
      VALUE = strdup(argv[i+1]);                         \
      if(!VALUE)                                         \
        return -2;                                       \
      size_t j;                                          \
      for(j=0; j < strlen(argv[i+1]); ++j)               \
        argv[i+1][j] = '#';                              \
      argc--;                                            \
      i++;                                               \
    }

#define PARSE_HEXSTRING_PARAM_SEC(SHORT, LONG, VALUE)    \
    else if(!strcmp(str,SHORT) || !strcmp(str,LONG))     \
    {                                                    \
      if(argc < 1 || argv[i+1][0] == '-')                \
        return i;                                        \
      int ret;                                           \
      ret = options_parse_hex_string(argv[i+1], &VALUE); \
      if(ret > 0)                                        \
        return i+1;                                      \
      else if(ret < 0)                                   \
        return ret;                                      \
      size_t j;                                          \
      for(j=0; j < strlen(argv[i+1]); ++j)               \
        argv[i+1][j] = '#';                              \
      argc--;                                            \
      i++;                                               \
    }

#define PARSE_STRING_LIST(SHORT, LONG, LIST)             \
    else if(!strcmp(str,SHORT) || !strcmp(str,LONG))     \
    {                                                    \
      if(argc < 1 || argv[i+1][0] == '-')                \
        return i;                                        \
      int ret = string_list_add(&LIST, argv[i+1]);       \
      if(ret == -2)                                      \
        return ret;                                      \
      else if(ret)                                       \
        return i+1;                                      \
      argc--;                                            \
      i++;                                               \
    }

int options_parse_hex_string(const char* hex, buffer_t* buffer)
{
  if(!hex || !buffer)
    return -1;

  uint32_t hex_len = strlen(hex);
  if(hex_len%2)
    return 1;

  if(buffer->buf_) 
    free(buffer->buf_);
  
  buffer->length_ = hex_len/2;
  buffer->buf_ = malloc(buffer->length_);
  if(!buffer->buf_) {
    buffer->length_ = 0;
    return -2;
  }

  const char* ptr = hex;
  int i;
  for(i=0;i<buffer->length_;++i) {
    uint32_t tmp;
    sscanf(ptr, "%2X", &tmp);
    buffer->buf_[i] = (uint8_t)tmp;
    ptr += 2;
  }

  return 0;
}


int options_parse(options_t* opt, int argc, char* argv[])
{
  if(!opt)
    return -1;

  options_default(opt);

  if(opt->progname_)
    free(opt->progname_);
  opt->progname_ = strdup(argv[0]);
  if(!opt->progname_)
    return -2;

  argc--;

  int i;
  for(i=1; argc > 0; ++i)
  {
    char* str = argv[i];
    argc--;

    if(!strcmp(str,"-h") || !strcmp(str,"--help"))
      return -1;
    else if(!strcmp(str,"-v") || !strcmp(str,"--version"))
      return -3;
    PARSE_INVERSE_BOOL_PARAM("-D","--nodaemonize", opt->daemonize_)
    PARSE_STRING_PARAM("-u","--username", opt->username_)
    PARSE_STRING_PARAM("-g","--groupname", opt->groupname_)
    PARSE_STRING_PARAM("-C","--chroot", opt->chroot_dir_)
    PARSE_STRING_PARAM("-P","--write-pid", opt->pid_file_)
    PARSE_STRING_LIST("-L","--log", opt->log_targets_)
    PARSE_BOOL_PARAM("-U", "--debug", opt->debug_)
    PARSE_STRING_PARAM("-l","--local-addr", opt->local_addr_)
    PARSE_STRING_PARAM("-p","--local-port", opt->local_port_)
    PARSE_STRING_PARAM("-r","--remote-addr", opt->remote_addr_)
    PARSE_STRING_PARAM("-o","--remote-port", opt->remote_port_)
    PARSE_STRING_PARAM("-c","--config", opt->config_file_)
    else 
      return i;
  }

  if(opt->debug_) {
    string_list_add(&opt->log_targets_, "stdout:5");
    opt->daemonize_ = 0;
  }

  if(!opt->log_targets_.first_) {
    string_list_add(&opt->log_targets_, "syslog:3,tcpproxy,daemon");
  }

  return 0;
}

void options_parse_post(options_t* opt)
{
  if(!opt)
    return;

      // nothing yet
}

void options_default(options_t* opt)
{
  if(!opt)
    return;

  opt->progname_ = strdup("tcpproxy");
  opt->daemonize_ = 1;
  opt->username_ = NULL;
  opt->groupname_ = NULL;
  opt->chroot_dir_ = NULL;
  opt->pid_file_ = NULL;
  opt->local_addr_ = NULL;
  opt->local_port_ = NULL;
  opt->remote_addr_ = NULL;
  opt->remote_port_ = NULL;
      // TODO set system config dir
  opt->config_file_ = strdup("/etc/tcpproxy.conf");
  string_list_init(&opt->log_targets_);
  opt->debug_ = 0;
}

void options_clear(options_t* opt)
{
  if(!opt)
    return;

  if(opt->progname_)
    free(opt->progname_);
  if(opt->username_)
    free(opt->username_);
  if(opt->groupname_)
    free(opt->groupname_);
  if(opt->chroot_dir_)
    free(opt->chroot_dir_);
  if(opt->pid_file_)
    free(opt->pid_file_);
  string_list_clear(&opt->log_targets_);
  if(opt->local_addr_)
    free(opt->local_addr_);
  if(opt->local_port_)
    free(opt->local_port_);
  if(opt->remote_addr_)
    free(opt->remote_addr_);
  if(opt->remote_port_)
    free(opt->remote_port_);
  if(opt->config_file_)
    free(opt->config_file_);
}

void options_print_usage()
{
  printf("USAGE:\n");
  printf("tcpproxy [-h|--help]                         prints this...\n");
  printf("         [-v|--version]                      print version info and exit\n");
  printf("         [-D|--nodaemonize]                  don't run in background\n");
  printf("         [-u|--username] <username>          change to this user\n");
  printf("         [-g|--groupname] <groupname>        change to this group\n");
  printf("         [-C|--chroot] <path>                chroot to this directory\n");
  printf("         [-P|--write-pid] <path>             write pid to this file\n");
  printf("         [-L|--log] <target>:<level>[,<param1>[,<param2>..]]\n");
  printf("                                             add a log target, can be invoked several times\n");
  printf("         [-U|--debug]                        don't daemonize and log to stdout with maximum log level\n");
  printf("         [-l|--local-addr] <host>            local address to listen on\n");
  printf("         [-p|--local-port] <service>         local port to listen on\n");
  printf("         [-r|--remote-addr] <host>           remote address to connect to\n");
  printf("         [-o|--remote-port] <service>        remote port to connect to\n");
  printf("         [-c|--config] <file>                configuration file\n");
}

void options_print_version()
{
  printf("%s\n", VERSION_STRING_0);
  printf("%s\n", VERSION_STRING_1);
}

void options_print(options_t* opt)
{
  if(!opt)
    return;

  printf("progname: '%s'\n", opt->progname_);
  printf("daemonize: %d\n", opt->daemonize_);
  printf("username: '%s'\n", opt->username_);
  printf("groupname: '%s'\n", opt->groupname_);
  printf("chroot_dir: '%s'\n", opt->chroot_dir_);
  printf("pid_file: '%s'\n", opt->pid_file_);
  printf("log_targets: \n");
  string_list_print(&opt->log_targets_, "  '", "'\n");
  printf("local_addr: '%s'\n", opt->local_addr_);
  printf("local_port: '%s'\n", opt->local_port_);
  printf("remote_addr: '%s'\n", opt->remote_addr_);
  printf("remote_port: '%s'\n", opt->remote_port_);
  printf("config_file: '%s'\n", opt->config_file_);
  printf("debug: %s\n", !opt->debug_ ? "false" : "true");
}
