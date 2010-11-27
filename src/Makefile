##
##  tcpproxy
##
##  tcpproxy is a simple tcp connection proxy which combines the 
##  features of rinetd and 6tunnel. tcpproxy supports IPv4 and 
##  IPv6 and also supports connections from IPv6 to IPv4 
##  endpoints and vice versa.
##  
##
##  Copyright (C) 2010-2011 Christian Pointner <equinox@spreadspace.org>
##                         
##  This file is part of tcpproxy.
##
##  tcpproxy is free software: you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation, either version 3 of the License, or
##  any later version.
##
##  tcpproxy is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with tcpproxy. If not, see <http://www.gnu.org/licenses/>.
##

ifneq ($(MAKECMDGOALS),distclean)
include include.mk
endif

EXECUTABLE := tcpproxy

C_OBJS := log.o \
          options.o \
          slist.o \
          string_list.o \
          sig_handler.o \
          tcp.o \
          listener.o \
          clients.o \
          tcpproxy.o

C_SRCS := $(C_OBJS:%.o=%.c)

.PHONY: clean cleanall distclean manpage install install-bin install-etc install-man uninstall remove purge

all: $(EXECUTABLE)

%.d: %.c
	@set -e; rm -f $@; \
  $(CC) -MM $(CFLAGS) $< > $@.$$$$; \
  sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
  rm -f $@.$$$$; echo '(re)building $@'

ifneq ($(MAKECMDGOALS),distclean)
-include $(C_SRCS:%.c=%.d)
endif

$(EXECUTABLE): $(C_OBJS)
	$(CC) $(C_OBJS) -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

strip: $(EXECUTABLE)
	$(STRIP) -s $(EXECUTABLE)


distclean: cleanall
	find . -name *.o -exec rm -f {} \;
	find . -name "*.\~*" -exec rm -rf {} \;
	rm -f include.mk
	rm -f config.h

clean:
	rm -f *.o
	rm -f *.d
	rm -f *.d.*
	rm -f $(EXECUTABLE)
	rm -f $(EXECUTABLE).exe

cleanall: clean
	$(MAKE) --directory="../doc/" clean

manpage:
	$(MAKE) --directory="../doc/" manpage


INSTALL_TARGETS := install-bin install-etc
REMOVE_TARGETS := remove-bin remove-etc

ifdef MANDIR
INSTALL_TARGETS += install-man
REMOVE_TARGETS += remove-man
endif

ifdef EXAMPLESDIR
INSTALL_TARGETS += install-examples
REMOVE_TARGETS += remove-examples
endif

install: all $(INSTALL_TARGETS)

install-bin: $(EXECUTABLE)
	$(INSTALL) -d $(DESTDIR)$(SBINDIR)
	$(INSTALL) -m 755 $(EXECUTABLE) $(DESTDIR)$(SBINDIR)

install-etc:
	$(INSTALL) -d $(DESTDIR)$(ETCDIR)/$(EXECUTABLE)
	@ echo "example configurations can be found at $(EXAMPLESDIR)/$(EXECUTABLE)" > $(DESTDIR)$(ETCDIR)/$(EXECUTABLE)/README

install-examples:
	$(INSTALL) -d $(DESTDIR)$(EXAMPLESDIR)/$(EXECUTABLE)

install-man: manpage
	$(INSTALL) -d $(DESTDIR)$(MANDIR)/man8/
	$(INSTALL) -m 644 ../doc/tcpproxy.8 $(DESTDIR)$(MANDIR)/man8/$(EXECUTABLE).8

uninstall: remove

remove: $(REMOVE_TARGETS)

remove-bin:
	rm -f $(DESTDIR)$(SBINDIR)/$(EXECUTABLE)

remove-etc:
	rm -f $(DESTDIR)$(ETCDIR)/init.d/$(EXECUTABLE)

remove-examples:
	rm -rf $(DESTDIR)$(EXAMPLESDIR)/$(EXECUTABLE)/

remove-man:
	rm -f $(DESTDIR)$(MANDIR)/man8/$(EXECUTABLE).8

purge: remove
	rm -rf $(DESTDIR)$(ETCDIR)/$(EXECUTABLE)/
