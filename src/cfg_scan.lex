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

#include "cfg_parse.h"

#include "tcp.h"

extern int line_cnt;
%}

%option noyywrap

%%

\s*"#".*\n               line_cnt++;
\{                       return TOK_OPEN;
\}                       return TOK_CLOSE;
\:                       return TOK_COLON;
\;                       return TOK_SEMICOLON;
\*                       return TOK_ASTERISK;
"listen"                 return TOK_LISTEN;
"resolv"                 return TOK_RESOLV;
"remote"                 return TOK_REMOTE;
"remote-resolv"          return TOK_REMOTE_RESOLV;
"source"                 return TOK_SOURCE;
"ipv4"                   yylval.rtype = IPV4_ONLY; return TOK_IPV4;
"ipv6"                   yylval.rtype = IPV6_ONLY; return TOK_IPV6;
[0-9]+                   yylval.string = strdup(yytext); return TOK_NUMBER;
[0-9\.]+                 yylval.string = strdup(yytext); return TOK_IPV4_ADDR;
[0-9a-fA-F:]+            yylval.string = strdup(yytext); return TOK_IPV6_ADDR;
[a-zA-Z0-9\-]+           yylval.string = strdup(yytext); return TOK_NAME;
[a-zA-Z0-9\-\.]+         yylval.string = strdup(yytext); return TOK_HOSTNAME;
\n                       line_cnt++;
[ \t]+                   /* ignore whitespace */;

%%

