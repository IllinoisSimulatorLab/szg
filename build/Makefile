include make/Makefile.os

ifeq "$z" "UNKNOWN_OS"
all:
	@echo "Makefile failed to guess OS type."
else

.PHONY: all clean demo democlean install

# Note that demo is still a target here... there are programs that live
#in the top of that directory and not in subdirs
SUBDIRS=language phleet barrier math drivers graphics sound obj interaction \
  framework utilities 

# If one just wants to make a particular demo, go to that directory. There is
# no need for a "make fast"

# isosurface has been temporarily removed from the DEMODIRS until the code
# is brought up to data
DEMODIRS=ribbons atlantis coaster hspace avn volume \
  cosmos cubes parade timetunnel schprel q33 

INSTALLDIR = $(SZGHOME)/szg-install

# Must be defined here (as well as in Makefile.defines) since we need it below:
ifeq ($z,win32)
  OBJ_SUFFIX = .obj
else
  OBJ_SUFFIX = .o
endif

all:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i -j $(JOBS) "SZGHOME=$(SZGHOME)" || exit 1 ; done

demo: all
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i -j $(JOBS) "SZGHOME=$(SZGHOME)" ; done
create-install: all
	rm -rf $(INSTALLDIR)
	mkdir $(INSTALLDIR)
	mkdir $(INSTALLDIR)/temp
	mkdir $(INSTALLDIR)/include
	mkdir $(INSTALLDIR)/bin
	mkdir $(INSTALLDIR)/lib
	mkdir $(INSTALLDIR)/build
	mkdir $(INSTALLDIR)/build/make
	cp $(SZGHOME)/build/make/Makefile.defines $(INSTALLDIR)/build/make
	cp $(SZGHOME)/build/make/Makefile.os $(INSTALLDIR)/build/make
	cp $(SZGHOME)/build/make/Makefile.libscan $(INSTALLDIR)/build/make
	cp $(SZGHOME)/build/make/Makefile.vars $(INSTALLDIR)/build/make
	@for i in include $(SUBDIRS) ; do cp -f $(SZGHOME)/src/$$i/*.h $(INSTALLDIR)/include ; done
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i create-install "SZGHOME=$(SZGHOME)" "INSTALLDIR=$(INSTALLDIR)" ; done
	@for i in $(DEMOUTIL) ; do $(MAKE) -C $z/demo/$$i create-install-exe "SZGHOME=$(SZGHOME)" "INSTALLDIR=$(INSTALLDIR)" ; done
	$(AR) r $(INSTALLDIR)/lib/libszg.a $(INSTALLDIR)/temp/*$(OBJ_SUFFIX)
	rm -rf $(INSTALLDIR)/temp

clean:
	@for i in $(SUBDIRS) ; do $(MAKE) -C $z/$$i clean "SZGHOME=$(SZGHOME)" ; done
democlean:
	@for i in $(DEMODIRS) ; do $(MAKE) -C $z/demo/$$i clean "SZGHOME=$(SZGHOME)" ; done
installclean:
	rm -rf $(INSTALLLIB) $(INSTALLDIR)

endif
