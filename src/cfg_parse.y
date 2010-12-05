%{
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
#include <string.h>

#include "datatypes.h"
#include "log.h"
#include "options.h"
#include "tcp.h"
#include "listener.h"

void yyerror(const char *);
int yylex(void);

int line_cnt = 1;
const char* config_file_;
listeners_t* glisteners;

struct listener {
  char* la_;
  resolv_type_t lrt_;
  char* lp_;
  char* ra_;
  resolv_type_t rrt_;
  char* rp_;
  char* sa_;
};

static struct listener* new_listener_struct()
{
  struct listener* l = malloc(sizeof(struct listener));
  l->la_ = NULL;
  l->lrt_ = ANY;
  l->lp_ = NULL;
  l->ra_ = NULL;
  l->rrt_ = ANY;
  l->rp_ = NULL;
  l->sa_ = NULL;
}

static void clear_listener_struct(struct listener* l)
{
  if(l->la_)
    free(l->la_);
  if(l->lp_)
    free(l->lp_);
  if(l->ra_)
    free(l->ra_);
  if(l->rp_)
    free(l->rp_);
  if(l->sa_)
    free(l->sa_);

  free(l);
}

static void merge_listener_struct(struct listener* dest, struct listener* src)
{
  if(src->la_) {
    if(dest->la_)
      free(dest->la_);
    dest->la_ = src->la_;
  }
  
  if(src->lrt_ != ANY)
    dest->lrt_ = src->lrt_;

  if(src->lp_) {
    if(dest->lp_)
      free(dest->lp_);
    dest->lp_ = src->lp_;
  }

  if(src->ra_) {
    if(dest->ra_)
      free(dest->ra_);
    dest->ra_ = src->ra_;
  }

  if(src->rrt_ != ANY)
    dest->rrt_ = src->rrt_;

  if(src->rp_) {
    if(dest->rp_)
      free(dest->rp_);
    dest->rp_ = src->rp_;
  }

  if(src->sa_) {
    if(dest->sa_)
      free(dest->sa_);
    dest->sa_ = src->sa_;
  }

  free(src);
}


void yyinit(const char* config_file, listeners_t* listeners)
{
  config_file_ = config_file;
  glisteners = listeners;
}

%}

%union 
{
  char null;
  char *string;
  int rtype;
  struct listener* listener;
}

%token <null> TOK_OPEN;
%token <null> TOK_CLOSE;
%token <null> TOK_COLON;
%token <null> TOK_SEMICOLON;
%token <null> TOK_ASTERISK;
%token <null> TOK_LISTEN;
%token <null> TOK_RESOLV;
%token <null> TOK_REMOTE;
%token <null> TOK_REMOTE_RESOLV;
%token <null> TOK_SOURCE;
%token <rtype> TOK_IPV4;
%token <rtype> TOK_IPV6;
%token <string> TOK_NUMBER;
%token <string> TOK_IPV4_ADDR;
%token <string> TOK_IPV6_ADDR;
%token <string> TOK_NAME;
%token <string> TOK_HOSTNAME;

%type <null> listen
%type <listener> listen_head
%type <listener> listen_body
%type <listener> listen_body_element
%type <listener> remote
%type <listener> source
%type <listener> resolv
%type <listener> remote_resolv
%type <rtype> resolv_type
%type <string> service
%type <string> host_or_addr

%error-verbose

%%
cfg: 
               | cfg listen
               ;

listen:          listen_head TOK_OPEN listen_body TOK_CLOSE TOK_SEMICOLON
{
  merge_listener_struct($1, $3);

  int ret = listener_add(glisteners, $1->la_, $1->lrt_, $1->lp_, $1->ra_, $1->rrt_, $1->rp_, $1->sa_);
  clear_listener_struct($1);
  if(ret) {
    YYABORT;
  }
}
;

listen_head:     TOK_LISTEN host_or_addr service
{
  struct listener* l = new_listener_struct();
  l->la_ = $2;
  l->lp_ = $3;
  $$ = l;
}
               | TOK_LISTEN TOK_ASTERISK service
{
  struct listener* l = new_listener_struct();
  l->lp_ = $3;
  $$ = l;
}
;

listen_body:     listen_body_element
               | listen_body listen_body_element
{
  merge_listener_struct($1, $2);
  $$ = $1;
}
;

listen_body_element:    resolv TOK_SEMICOLON
                      | remote TOK_SEMICOLON
                      | remote_resolv TOK_SEMICOLON
                      | source TOK_SEMICOLON
                      ;

remote:          TOK_REMOTE TOK_COLON host_or_addr service
{
  struct listener* l = new_listener_struct();
  l->ra_ = $3;
  l->rp_ = $4;
  $$ = l;
}
;

source:          TOK_SOURCE TOK_COLON host_or_addr
{
  struct listener* l = new_listener_struct();
  l->sa_ = $3;
  $$ = l;
}
;

resolv:          TOK_RESOLV TOK_COLON resolv_type
{
  struct listener* l = new_listener_struct();
  l->lrt_ = $3;
  $$ = l;
}
;

remote_resolv:   TOK_REMOTE_RESOLV TOK_COLON resolv_type
{
  struct listener* l = new_listener_struct();
  l->rrt_ = $3;
  $$ = l;
}
;


resolv_type:     TOK_IPV4
               | TOK_IPV6
               ;

service:         TOK_NUMBER
               | TOK_NAME
               ;

host_or_addr:    TOK_NUMBER
               | TOK_NAME
               | TOK_HOSTNAME
               | TOK_IPV4_ADDR
               | TOK_IPV6_ADDR
               ;
%%

void yyerror (const char *string)
{
  log_printf(ERROR, "%s:%d %s\n", config_file_, line_cnt, string);
}

