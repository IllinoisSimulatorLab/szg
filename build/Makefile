include make/Makefile.os

ifeq "$z" "UNKNOWN_OS"
all:
	@echo "Makefile failed to guess OS type."
else

.PHONY: all clean demo democlean install

# Note that demo is still a target here... there are programs that live
#in the top of that directory and not in subdirs
LIBDIRS=language phleet barrier math drivers graphics sound obj interaction \
  framework
SUBDIRS=$(LIBDIRS) utilities 

# If one just wants to make a particular demo, go to that directory. There is
# no need for a "make fast"

DEMODIRS=ribbons atlantis coaster hspace avn volume \
  cosmos cubes parade timetunnel schprel q33 

# This must be defined both here (for the create of the "install" directory
# structure) AND in the local makefiles (via Makefile.vars)
INSTALLDIR = $(SZGHOME)/szg-install

all:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i -j $(JOBS) || exit 1 ; done

clean:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i clean ; done

demo: all
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i -j $(JOBS) ; done

demo-clean:
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i clean ; done

create-install: all
	rm -rf $(INSTALLDIR)
	mkdir $(INSTALLDIR)
	mkdir $(INSTALLDIR)/include
	mkdir $(INSTALLDIR)/bin
	mkdir $(INSTALLDIR)/lib
	mkdir $(INSTALLDIR)/build
	mkdir $(INSTALLDIR)/build/make
	cp $(SZGHOME)/build/make/Makefile.os $(INSTALLDIR)/build/make
	cp $(SZGHOME)/build/make/Makefile.libscan $(INSTALLDIR)/build/make
	cp $(SZGHOME)/build/make/Makefile.vars $(INSTALLDIR)/build/make
	cp $(SZGHOME)/build/make/Makefile.rules $(INSTALLDIR)/build/make 
	@for i in include $(LIBDIRS) ; do cp $(SZGHOME)/src/$$i/*.h $(INSTALLDIR)/include ; done
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i create-install ; done

install-clean:
	rm -rf $(INSTALLDIR)

endif
