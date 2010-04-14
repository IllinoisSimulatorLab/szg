.PHONY: all drivers phleet clean docs demo clean-sdk bin-install install-shared demo-clean demo-create-sdk demo-bin-install

ifeq "$(SZGHOME)" ""
all:
	@cd ..; X=`pwd`; echo "Before making, set the environment variable SZGHOME to" $$X" ( export SZGHOME="$$X" )."
else

include make/Makefile.os
ifeq "$PLATFORM" "UNKNOWN_OS"
all:
	@echo "Makefile failed to guess operating system."
else

# Install shared libraries in SZG_BINDIR (or INSTALLDIR for the SDK).
include make/Makefile.libscan

PHLEETDIRS=language math phleet
DRVDIRS=language math phleet barrier drivers
LIBDIRS=$(DRVDIRS) graphics sound model interaction framework
SUBDIRS=$(LIBDIRS) utilities 
# DEMODIRS=ribbons atlantis coaster hspace cosmos cubes parade schprel
DEMODIRS=ribbons atlantis coaster hspace schprel

# For 8-cpu salami.isl.uiuc.edu and other multicore multi-cpu hyperthreaded.
ifeq "$(PLATFORM)" "win32"
ifneq "$(NUMBER_OF_PROCESSORS)" ""
ifneq ($(wildcard $(SZGHOME)/build/win32/language/arPrecompiled.pch),)
  JOBS=$(eval( $(NUMBER_OF_PROCESSORS) * 3))
endif
endif
endif

all:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $(PLATFORM)/$$i -j $(JOBS) || exit 1 ; done

drivers:
	@for i in $(DRVDIRS) ; do $(MAKE) -C $(PLATFORM)/$$i -j $(JOBS) || exit 1 ; done

phleet:
	@for i in $(PHLEETDIRS) ; do $(MAKE) -C $(PLATFORM)/$$i -j $(JOBS) || exit 1 ; done

clean:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $(PLATFORM)/$$i --no-print-directory clean ; done

docs:
	rm -rf $(SZGHOME)/doc/*.html
	$(MAKE) -C $(SZGHOME)/doc

demo: all
	@for i in $(DEMODIRS) ; do $(MAKE) -C $(PLATFORM)/demo/$$i --no-print-directory -j $(JOBS) ; done

# Copy stuff into INSTALLDIR.
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
	$(MAKE) docs
	cp -r $(SZGHOME)/doc/ $(INSTALLDIR)/doc
	rm -rf $(INSTALLDIR)/doc/txt2tags
	find $(INSTALLDIR)/doc -name 'CVS' -print | xargs rm -rf
	cp -r $(SZGHOME)/rsc/ $(INSTALLDIR)/rsc
	find $(INSTALLDIR)/rsc -name 'CVS' -print | xargs rm -rf
	cp $(SZGHOME)/SZG_CREDITS $(SZGHOME)/script/ezszg.sh $(INSTALLDIR)
	@for i in $(LIBDIRS) ; do cp $(SZGHOME)/src/$$i/*.h $(INSTALLDIR)/include ; done
	@for i in $(SUBDIRS) ; do $(MAKE) -C $(PLATFORM)/$$i create-sdk || exit 1 ; done
	cp $(SZG_SHARED_COPY) $(INSTALLDIR)/bin
	cd $(INSTALLDIR)/bin; $(SZG_SHARED_COPY_ACTION)

clean-sdk:
	rm -rf $(INSTALLDIR)

# Stuff SZG_BINDIR.
bin-install: all install-shared
	@for i in $(SUBDIRS) ; do $(MAKE) -C $(PLATFORM)/$$i bin-install || exit 1 ; done

# Copy external shared libraries into SZG_BINDIR.
install-shared:
	cp $(SZG_SHARED_COPY) $(SZG_BINDIR)
	cd $(SZG_BINDIR); $(SZG_SHARED_COPY_ACTION)

demo-clean:
	@for i in $(DEMODIRS) ; do $(MAKE) -C $(PLATFORM)/demo/$$i clean ; done

demo-create-sdk: all
	@for i in $(DEMODIRS) ; do $(MAKE) -C $(PLATFORM)/demo/$$i --no-print-directory create-sdk || exit 1 ; done

demo-bin-install: all
	@for i in $(DEMODIRS) ; do $(MAKE) -C $(PLATFORM)/demo/$$i --no-print-directory bin-install || exit 1 ; done

endif
endif
