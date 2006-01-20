#!/bin/sh

# Figure out which of the supported architectures we are.
# TODO: MINGW is not really supported yet.
if [ $(uname) == "CYGWIN_NT-4.0" ]; then
  arch=win32
elif [ $(uname) == "CYGWIN_NT-5.0" ]; then
  arch=win32
elif [ $(uname) == "CYGWIN_NT-5.1" ]; then
  arch=win32
elif [ $(uname) == "MINGW32_NT-4.0" ]; then
  arch=win32
elif [ $(uname) == "MINGW32_NT-5.0" ]; then
  arch=win32
elif [ $(uname) == "MINGW32_NT-5.1" ]; then
  arch=win32
elif [ $(uname) == "Linux" ]; then
  arch=linux
elif [ $(uname) == "linux" ]; then
  arch=linux
elif [ $(uname) == "Darwin" ]; then
  arch=darwin
else
  echo "ERROR: Your architecture is unsupported."
fi

# Check to see if we are in the right spot... Necessary since all variables
# are relative to the current directory.
TEMP=$PWD/doc/images/NormalAl.jpg
if [ -f $TEMP ]; then
  echo "Setting up your szg SDK..."
  # TODO: How to exit without exiting the shell???????
  if [ "$(echo $PS1 | sed '/^SZG/d')" == "" ]; then
    echo ERROR: Tried to call script a second time!
  fi
  # Set the shell prompt so we know which version of szg this is.
  export PS1=SZG_PROMPT_TEMPLATE:\\u@\\h:\\w\\r\\n::
  # This is an SDK. Thus, we want to set the SZG developer variables like so:
  # NOTE: On the windows side, we actually need to mangle the path names...
  #  (because of the way mingw and cygwin make Unix work with the windows file system)
  if [ $arch == "win32" ]; then
    echo Checking for cygwin root...
    if [ -d "c:/cygwin" ]; then
      HOMEDRIVE=c
      CYGSUBST="$HOMEDRIVE:\/cygwin"
    elif [ -d "d:/cygwin" ]; then
      HOMEDRIVE=d
      CYGSUBST="$HOMEDRIVE:\/cygwin"
    else
      echo ERROR: could not find cygwin in one of the expected places.
    fi
    if [ "$(echo $PWD | sed '/^\/home\//d')" != "" ]; then
      if [ "$(echo $PWD | sed '/^\/cygdrive/d')" == "" ]; then
        LOCALDIR=$(echo $PWD | sed "s/^\/cygdrive\/./$HOMEDRIVE:/")
      fi
    else
      LOCALDIR=$(echo $PWD | sed "s/^\/home/$CYGSUBST\/home/")
    fi
    export SZGHOME=$LOCALDIR
    export SZGBIN=$LOCALDIR/bin
    export SZGEXTERNAL=$LOCALDIR/external
    # File location vs. path location... darn that Cygwin name mangling!
    export PATH=$PWD/bin:$PATH
    # Must make sure to use windows native slashes in the PYTHONPATH
    export PYTHONPATH=$(echo "$LOCALDIR/bin;$PYTHONPATH" | sed 's/\//\\/g')
  else 
    export SZGHOME=$PWD
    export SZGBIN=$PWD/bin
    export SZGEXTERNAL=$PWD/external
    # Must be able to find the executables and Python modules.
    export PATH=$PWD/bin:$PATH
    export PYTHONPATH=$PWD/bin:$PYTHONPATH
  fi
  export SZG_DEVELOPER_STYLE=EASY
  # These variables tell Syzygy apps where to look for data (in the default
  # install of the sdk).
  export SZG_RENDER_texture_path=$PWD/rsc
  export SZG_RENDER_text_path=$PWD/rsc
  export SZG_PEER_path=$PWD/rsc
  export SZG_SOUND_path=$PWD/rsc
  export SZG_PARAM=$PWD/doc/window_configs.txt
  export SZG_DISPLAY0_name=single_window
  export SZG_DISPLAY1_name=virtual_cube_window
  export SZG_DISPLAY2_name=quad_window
  export SZG_DISPLAY3_name=single_window_fullscreen
  # The OS-dependent environment variables.
  if [ $arch == "linux" ]; then
    export LD_LIBRARY_PATH=$PWD/bin:$LD_LIBRARY_PATH
  elif [ $arch == "darwin" ]; then
    export DYLD_LIBRARY_PATH=$PWD/bin:$DYLD_LIBRARY_PATH
  fi
  # Visual Studio needs quite a few environment variables set to be used
  # from the command line. 
  if [ $arch == "win32" ]; then
    # If Visual Studio .NET is installed, use that.
    if [ -d "c:/Program Files/Microsoft Visual Studio .NET 2003" ]; then
      # We do NOT use STLport here.
      export SZG_STLPORT=FALSE
      # The following are good for a standard install of Visual Studio .NET 2003.
      export INCLUDE="C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Vc7\\ATLMFC\\INCLUDE;C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Vc7\\INCLUDE;C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Vc7\\PlatformSDK\include"
      export LIB="C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Vc7\\ATLMFC\\Lib;C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Vc7\\PlatformSDK\\Lib;C:\\Program Files\\Microsoft Visual Studio .NET 2003\\Vc7\\Lib"
      export PATH="/cygdrive/c/Program Files/Microsoft Visual Studio .NET 2003/Common7/IDE:$PATH"
      export PATH="/cygdrive/c/Program Files/Microsoft Visual Studio .NET 2003/Vc7/BIN:$PATH"
      export PATH="/cygdrive/c/Program Files/Microsoft Visual Studio .NET 2003/Common7/Tools:$PATH"
      export PATH="/cygdrive/c/Program Files/Microsoft Visual Studio .NET 2003/Common7/Tools/bin:$PATH"
    elif [ -d "c:/Program Files/Microsoft Visual Studio" ]; then
      # We MUST use STLport
      export SZG_STLPORT=TRUE
      # The following are good for Visual Studio 6, a standard install.
      # Note how we need to mangle the PATH for the cygwin environment.
      export PATH="/cygdrive/c/Program Files/Microsoft Visual Studio/Common/Tools/WinNT:$PATH"
      export PATH="/cygdrive/c/Program Files/Microsoft Visual Studio/Common/MSDev98/Bin:$PATH"
      export PATH="/cygdrive/c/Program Files/Microsoft Visual Studio/Common/Tools:$PATH"
      export PATH="/cygdrive/c/Program Files/Microsoft Visual Studio/VC98/bin:$PATH"
      export INCLUDE="C:\\Program Files\\Microsoft Visual Studio\\VC98\\atl\\include;C:\\Program Files\\Microsoft Visual Studio\\VC98\\mfc\\include;C:\\Program Files\\Microsoft Visual Studio\\VC98\\include"
      export LIB="C:\\Program Files\\Microsoft Visual Studio\\VC98\\mfc\\lib;C:\\Program Files\\Microsoft Visual Studio\\VC98\\lib"
      export MSDEVDIR="C:\\Program Files\\Microsoft Visual Studio\\Common\\MSDev98"
    else
      echo ERROR: Could not find the Visual Studio installation.
    fi
  fi
else
  echo "This script must be run from the top-level of your szg SDK directory."
fi




