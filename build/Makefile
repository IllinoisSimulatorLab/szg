# Lets us guess the operating system type.
include make/Makefile.os
# Want to include Makefile.libscan here, so we can install the shared
# libraries in the SZG_BINDIR (or in the INSTALLDIR for the SDK).
include make/Makefile.libscan

ifeq "$z" "UNKNOWN_OS"
all:
	@echo "Makefile failed to guess OS type."
else

DRVDIRS=language math phleet barrier drivers
LIBDIRS=$(DRVDIRS) graphics sound obj interaction framework
SUBDIRS=$(LIBDIRS) utilities 
DEMODIRS=ribbons atlantis coaster hspace avn volume \
  cosmos cubes parade timetunnel schprel q33 
DEMOFAST=cosmos atlantis

all:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i -j $(JOBS) || exit 1 ; done

drivers:
	@for i in $(DRVDIRS) ; do $(MAKE) -C $z/$$i -j $(JOBS) || exit 1 ; done

clean:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i clean ; done

demo: all
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i -j $(JOBS) ; done

# Build only the few demo(s) you're working on, not the whole collection.
fast: all
	@for i in $(DEMOFAST) ; do $(MAKE) -C $z/demo/$$i -j $(JOBS) ; done

# This target needs INSTALLDIR.
create-sdk: all
	rm -rf $(INSTALLDIR)
	rm -f $(SZGHOME)/doc/*~
	mkdir $(INSTALLDIR)
	cd $(INSTALLDIR); mkdir include bin lib build build/pch build/make
	cp $(SZGHOME)/src/language/arPrecompiled.h \
	   $(SZGHOME)/src/language/arPrecompiled.cpp $(INSTALLDIR)/build/pch
	cd $(SZGHOME)/build/make; cp blessing.txt Makefile.os \
           Makefile.libscan Makefile.vars Makefile.globals Makefile.toprule \
           Makefile.rules $(INSTALLDIR)/build/make 
	cp $(SZGHOME)/build/make/Makefile.easy $(INSTALLDIR)/build/Makefile
	cp -r $(SZGHOME)/doc/ $(INSTALLDIR)/doc
	find $(INSTALLDIR)/doc -name 'CVS' -print | xargs rm -rf
	cp -r $(SZGHOME)/rsc/ $(INSTALLDIR)/rsc
	find $(INSTALLDIR)/rsc -name 'CVS' -print | xargs rm -rf
	cp $(SZGHOME)/script/ezszg.sh $(INSTALLDIR)
	@for i in $(LIBDIRS) ; do cp $(SZGHOME)/src/$$i/*.h $(INSTALLDIR)/include ; done
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i create-sdk ; done
	cp $(SZG_SHARED_COPY) $(INSTALLDIR)/bin
	cd $(INSTALLDIR)/bin; $(SZG_SHARED_COPY_ACTION)

clean-sdk:
	rm -rf $(INSTALLDIR)

# This target needs SZG_BINDIR. It also copies the external shared libraries
# into the right spot.
bin-install: all
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i bin-install || exit 1 ; done
	cp $(SZG_SHARED_COPY) $(SZG_BINDIR)
	cd $(SZG_BINDIR); $(SZG_SHARED_COPY_ACTION)

# This target needs SZG_BINDIR. It only copies the external shared libraries
# into the right spot (sometimes this is all one wants).
install-shared:
	cp $(SZG_SHARED_COPY) $(SZG_BINDIR)
	cd $(SZG_BINDIR); $(SZG_SHARED_COPY_ACTION)

demo-clean:
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i clean ; done

# The "demo" rules only need to depend on "all" since the local Makefile
# will build the software, if necessary.
demo-create-sdk: all
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i create-sdk ; done

demo-bin-install: all
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i bin-install ; done

endif
