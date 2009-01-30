# Copyright (c) 2002 Jorge Acereda Macia <jacereda@users.sourceforge.net>
#
# Maintained by Peter O'Gorman <ogorman@users.sourceforge.net>
#
# Bug Reports and other queries should go to <ogorman@users.sourceforge.net>
#
#
# 
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice shall be
# included in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

prefix=/usr/local
DEBUG=0

CC=cc
OPT=-O2
DEBUGOPT=-g -O0
CFLAGS=-Wall -Wstrict-prototypes -Wmissing-prototypes $(OPT)
AR=ar cru
RANLIB=ranlib
INSTALL=install -c

DEFINES=-DDEBUG=$(DEBUG)
DATENOW=`date -u +"%Y%m%d"`

OBJS = dlfcn.o

all: libdl.a libdl.dylib

debug:
	make OPT="$(DEBUGOPT)" DEBUG=2

archive: clean
	cd .. && tar cvfz dlcompat-$(DATENOW).tar.gz dlcompat

finkinfo:
	cat dlcompat.info | sed "s|@VERSION@|$(DATENOW)|g" > ../dlcompat-$(DATENOW)-1.info

install: all
	$(INSTALL) -d -m 755 $(prefix)/lib/
	$(INSTALL) -m 644 libdl.a $(prefix)/lib/
	$(INSTALL) -m 644 libdl.0.dylib $(prefix)/lib
	ln -fs libdl.0.dylib $(prefix)/lib/libdl.dylib
	$(INSTALL) -d -m 755 $(prefix)/include/
	$(INSTALL) -m 644 dlfcn.h $(prefix)/include/

test: all
	DYLD_LIBRARY_PATH=".:$(DYLD_LIBRARY_PATH)" ./dltest.fs

installtest: install test

.c.o:
	$(CC) $(CFLAGS) $(DEFINES) -o $@ -c $<

libdl.a: $(OBJS)
	$(AR) libdl.a $(OBJS)
	$(RANLIB) libdl.a

libdl.dylib: libdl.0.dylib
	ln -sf $+ $@

libdl.0.dylib: $(OBJS)
	$(CC) -dynamiclib -o $@ $(OBJS) -install_name $(prefix)/lib/$@

clean:
	rm -f $(OBJS) libdl.* *~ *.o
