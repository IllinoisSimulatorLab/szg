# This code guesses the OS we're running in

z=UNKNOWN_OS
# use uname to guess the OS
ifeq ($(shell uname), Linux)
  z=linux
endif
ifeq ($(shell uname), linux)
  z=linux
endif
ifeq ($(shell uname), Darwin)
  z=darwin
endif
ifeq ($(shell uname), CYGWIN_NT-4.0)
  z=win32
endif
ifeq ($(shell uname), CYGWIN_NT-5.0)
  z=win32
endif
ifeq ($(shell uname), CYGWIN_NT-5.1)
  z=win32
endif
ifeq ($(shell uname), MINGW32_NT-4.0)
  z=win32
endif
ifeq ($(shell uname), MINGW32-5.0)
  z=win32
endif
ifeq ($(shell uname), MINGW32_NT-5.1)
  z=win32
endif
ifeq ($(shell uname -m), IP19)
  z=mips3
endif
ifeq ($(shell uname -m), IP22)
  z=mips3
endif
ifeq ($(shell uname -m), IP25)
  z=mips4
endif
ifeq ($(shell uname), IRIX64)
  z=mips4
endif

#as a backup, try the OSTYPE environment variable

ifeq "$(OSTYPE)" "linux"
  z=linux
endif
ifeq "$(OSTYPE)" "Linux"
  z=linux
endif
ifeq "$(OSTYPE)" "linux-gnu"
  z=linux
endif
ifeq "$(OSTYPE)" "cygwin32"
  z=win32
endif
ifeq "$(OSTYPE)" "cygwin"
  z=win32
endif

#and, as a final back-up, try the OS environment variable

ifeq "$(OS)" "Windows_NT"
  z=win32
endif

# OK, we guessed the OS.  Let's actually do something now.

# The PLATFORM variable is used some places
PLATFORM:=$z

ifeq "$z" "win32"
  JOBS = 1
else
ifeq "$z" "linux"
  JOBS = 2
else
ifeq "$z" "mips3"
  JOBS = 4
else
ifeq "$z" "mips4"
  JOBS = 8
else
ifeq "$z" "darwin"
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

ifeq "$z" "win32"
  MACHINE=WIN32
endif
ifeq "$z" "linux"
  MACHINE=LINUX
endif
ifeq "$z" "darwin"
  MACHINE=DARWIN
endif
ifeq "$z" "mips3"
  MACHINE=SGI
endif
ifeq "$z" "mips4"
  MACHINE=SGI
endif
