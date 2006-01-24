# This code guesses the OS we're running in

PLATFORM=UNKNOWN_OS
# use uname to guess the OS
ifeq ($(shell uname), Linux)
  PLATFORM=linux
endif
ifeq ($(shell uname), linux)
  PLATFORM=linux
endif
ifeq ($(shell uname), Darwin)
  PLATFORM=darwin
endif
ifeq ($(shell uname), CYGWIN_NT-4.0)
  PLATFORM=win32
endif
ifeq ($(shell uname), CYGWIN_NT-5.0)
  PLATFORM=win32
endif
ifeq ($(shell uname), CYGWIN_NT-5.1)
  PLATFORM=win32
endif
ifeq ($(shell uname), MINGW32_NT-4.0)
  PLATFORM=win32
endif
ifeq ($(shell uname), MINGW32-5.0)
  PLATFORM=win32
endif
ifeq ($(shell uname), MINGW32_NT-5.1)
  PLATFORM=win32
endif
ifeq ($(shell uname -m), IP19)
  PLATFORM=mips3
endif
ifeq ($(shell uname -m), IP22)
  PLATFORM=mips3
endif
ifeq ($(shell uname -m), IP25)
  PLATFORM=mips4
endif
ifeq ($(shell uname), IRIX64)
  PLATFORM=mips4
endif

#as a backup, try the OSTYPE environment variable

ifeq "$(OSTYPE)" "linux"
  PLATFORM=linux
endif
ifeq "$(OSTYPE)" "Linux"
  PLATFORM=linux
endif
ifeq "$(OSTYPE)" "linux-gnu"
  PLATFORM=linux
endif
ifeq "$(OSTYPE)" "cygwin32"
  PLATFORM=win32
endif
ifeq "$(OSTYPE)" "cygwin"
  PLATFORM=win32
endif

#and, as a final back-up, try the OS environment variable

ifeq "$(OS)" "Windows_NT"
  PLATFORM=win32
endif

# OK, we guessed the OS.  Let's actually do something now.

# The variable z used to hold the PLATFORM. For backwards compatibility in
# the makefiles, let's set it appropriately here. HOWEVER, its use is
# deprecated!
z:=$PLATFORM

ifeq "$PLATFORM" "win32"
  JOBS = 1
else
ifeq "$PLATFORM" "linux"
  JOBS = 2
else
ifeq "$PLATFORM" "mips3"
  JOBS = 4
else
ifeq "$PLATFORM" "mips4"
  JOBS = 8
else
ifeq "$PLATFORM" "darwin"
  JOBS = 2
else
  JOBS = 1
endif
endif
endif
endif
endif

# Finally, the way the stuff is set up now, a stub Makefile sets the MACHINE
# variable and then includes the regular Makefile (for the particular 
# application). However, to be able to do the install-shared target, we need
# that set here. So... do the following. Yes, this is redundant.
# Beware when thinking about changing the Makefiles so that the 
# Makefile.machine becomes unecessary. Now, the file structure 
# (via the Makefile.machine files) encodes the type of build that must occur.
# A future permutation would have to pass the information down the recursive
# make.

# A good idea to also set MACHINE_DIR, which is used for some top-level
# makefile operations depending on SZG_BINDIR.

ifeq "$PLATFORM" "win32"
  MACHINE=WIN32
  MACHINE_DIR=win32
endif
ifeq "$PLATFORM" "linux"
  MACHINE=LINUX
  MACHINE_DIR=linux
endif
ifeq "$PLATFORM" "darwin"
  MACHINE=DARWIN
  MACHINE_DIR=darwin
endif
ifeq "$PLATFORM" "mips3"
  MACHINE=SGI
  MACHINE_DIR=mips3
endif
ifeq "$PLATFORM" "mips4"
  MACHINE=SGI
  MACHINE_DIR=mips4
endif

# Want the definitions of SZG_BINDIR and INSTALLDIR to be consistent
# pretty much everywhere. These definitions are in the following fragment.
include $(SZGHOME)/build/make/Makefile.globals


