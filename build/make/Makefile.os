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
