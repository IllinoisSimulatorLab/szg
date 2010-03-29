# Guess the OS.

# Try "uname".

PLATFORM=UNKNOWN_OS
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

# Fallback: OSTYPE environment variable

ifeq "$(OSTYPE)" "linux"
  PLATFORM=linux
endif
ifeq "$(OSTYPE)" "Linux"
  PLATFORM=linux
endif
ifeq "$(OSTYPE)" "linux-gnu"
  PLATFORM=linux
endif

# Fallback: OS environment variable

ifeq "$(OS)" "Windows_NT"
  PLATFORM=win32
endif

# $z is a deprecated name for $PLATFORM.
z:=$(PLATFORM)

# Guess how many CPU's.

ifeq "$(PLATFORM)" "win32"
	ifeq "$(SZG_COMPILER)" "MINGW"
		JOBS = 2
	else
		JOBS = 1
	endif
else
ifeq "$(PLATFORM)" "linux"
  JOBS = 2
  ifeq ($(shell uname -m), x86_64)
    JOBS = 8
  endif
else
ifeq "$(PLATFORM)" "mips3"
  JOBS = 4
else
ifeq "$(PLATFORM)" "mips4"
  JOBS = 16
else
ifeq "$(PLATFORM)" "darwin"
  JOBS = 2
else
  JOBS = 1
endif
endif
endif
endif
endif

# A stub Makefile sets $MACHINE
# and then includes the regular Makefile (for the particular 
# application). However, the install-shared target needs
# that set here. So... do the following. Yes, this is redundant.
# Beware when thinking about changing the Makefiles so that the 
# Makefile.machine becomes unecessary. Now, the file structure 
# (via the Makefile.machine files) encodes what kind of build occurs.
# A future permutation would have to pass the information down the recursive make.

ifeq "$(PLATFORM)" "win32"
  MACHINE=WIN32
endif
ifeq "$(PLATFORM)" "linux"
  MACHINE=LINUX
endif
ifeq "$(PLATFORM)" "darwin"
  MACHINE=DARWIN
endif
ifeq "$(PLATFORM)" "mips3"
  MACHINE=SGI
endif
ifeq "$(PLATFORM)" "mips4"
  MACHINE=SGI
endif

# Try to keep SZG_BINDIR and INSTALLDIR consistent (in Makefile.globals).
include $(SZGHOME)/build/make/Makefile.globals
