include make/Makefile.os
# Want to include Makefile.libscan here, so we can install-shared
include make/Makefile.libscan

# This must be defined both here (for the create of the "install" directory
# structure) AND in the local makefiles (via Makefile.vars)
INSTALLDIR = $(SZGHOME)/szg-install

ifeq "$z" "UNKNOWN_OS"
all:
	@echo "Makefile failed to guess OS type."
else

LIBDIRS=language math phleet barrier drivers graphics sound \
  obj interaction framework
SUBDIRS=$(LIBDIRS) utilities 
DEMODIRS=ribbons atlantis coaster hspace avn volume \
  cosmos cubes parade timetunnel schprel q33 
DEMOFAST=cosmos atlantis

all:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i -j $(JOBS) || exit 1 ; done

clean:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i clean ; done

demo: all
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i -j $(JOBS) ; done

# Build only the few demo(s) you're working on, not the whole collection.
fast: all
	@for i in $(DEMOFAST) ; do $(MAKE) -C $z/demo/$$i -j $(JOBS) ; done

demo-clean:
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i clean ; done

create-install: all
	rm -rf $(INSTALLDIR)
	mkdir $(INSTALLDIR)
	cd $(INSTALLDIR); mkdir include bin lib build build/pch build/make
	cp $(SZGHOME)/src/language/arPrecompiled.h \
	   $(SZGHOME)/src/language/arPrecompiled.cpp $(INSTALLDIR)/build/pch
	cd $(SZGHOME)/build/make; cp Makefile.os Makefile.libscan \
	   Makefile.vars Makefile.rules $(INSTALLDIR)/build/make 
	cp $(SZGHOME)/build/make/Makefile.easy $(INSTALLDIR)/build/Makefile
	@for i in $(LIBDIRS) ; do cp $(SZGHOME)/src/$$i/*.h $(INSTALLDIR)/include ; done
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i create-install ; done

install-clean:
	rm -rf $(INSTALLDIR)

# This target needs SZG_BINDIR.
install-shared:
	cp $(SZG_SHARED_COPY) $(SZG_BINDIR)
	$(SZG_SHARED_COPY_ACTION)

bin-install:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i bin-install || exit 1 ; done

endif
