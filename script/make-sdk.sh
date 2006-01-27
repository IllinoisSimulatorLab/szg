#!/bin/sh
TEMP=$(uname)
if [ "$TEMP" = "CYGWIN_NT-4.0" ]; then
  arch=win32
elif [ "$TEMP" = "CYGWIN_NT-5.0" ]; then
  arch=win32
elif [ "$TEMP" = "CYGWIN_NT-5.1" ]; then
  arch=win32
elif [ "$TEMP" = "MINGW32_NT-4.0" ]; then
  arch=win32
elif [ "$TEMP" = "MINGW32_NT-5.0" ]; then
  arch=win32
elif [ "$TEMP" = "MINGW32_NT-5.1" ]; then
  arch=win32
elif [ "$TEMP" = "Linux" ]; then
  arch=linux
elif [ "$TEMP" = "linux" ]; then
  arch=linux
elif [ "$TEMP" = "Darwin" ]; then
  arch=darwin
else
  echo ERROR: Your architecture is unsupported.
  exit 1
fi

if  [ $# -lt 1 ]
then
  echo ERROR: You need to provide an szg version number.
  exit 1
fi

if [ -f "$SZGHOME/build/make/Makefile.vars" ]; then
  echo Creating SDK...
  cd $SZGHOME
  # If there was already an sdk, go ahead and clean up.
  rm -rf szg-install
  cd $SZGHOME/build
  echo Adding base library...
  make -s create-sdk
  cd ../python
  echo Adding python bindings...
  make -s create-sdk
  cd $SZGHOME
  if [ "$arch" = "win32" -a -f "/usr/bin/find" ]; then
    echo Making sure the CVS directories have been stripped from the SDK...
    /usr/bin/find szg-install -name 'CVS' -print | xargs rm -rf
  fi
  echo Removing previous SDK constructions.
  mv szg-install szgTEMP
  rm -rf szg-*
  mv szgTEMP szg-$1
  echo Changing ezszg.sh prompt...
  cd szg-$1
  sed "s/SZG_PROMPT_TEMPLATE/SZG-$1/" ezszg.sh > temp.sh
  mv temp.sh ezszg.sh
  cd ..
  echo Adding external libraries...
  if [ -d "$SZGEXTERNAL/$arch" ]; then
    cd szg-$1
    mkdir external
    cp -r $SZGEXTERNAL/$arch external
	if [ "$arch" = "darwin" ]; then
	  for libdir in $(ls external/darwin)
	  do
	    for libfile in $(ls external/darwin/$libdir/lib | grep ".a")
		do
		  echo Running ranlib on external/darwin/$libdir/lib/$libfile
		  ranlib external/darwin/$libdir/lib/$libfile
		done
	  done
	fi
    if [ "$arch" = "win32" ]; then
      if [ -d external/win32/VisualStudio ]; then
        # The libraries for VisualStudio were included. They should go in bin.
        cp external/win32/VisualStudio/* bin
        rm -rf external/win32/VisualStudio
      else
        echo WARNING: VisualStudio libraries not installed in SZGEXTERNAL.
      fi
      # For some unknown reason, I seem to need a different jpeg library with vc6 and vc7.
      if [ -d "external/win32/jpeg" ]; then
        rm -rf external/win32/jpeg
        # If SZG_STLPORT is TRUE, then we must be using VC6, otherwise VC7.
        if [ "$SZG_STLPORT" = "TRUE" ]; then
          if [ -d "external/win32/jpeg-vc6" ]; then
            mv external/win32/jpeg-vc6 external/win32/jpeg
          fi
        else
          if [ -d "external/win32/jpeg-vc7" ]; then
            mv external/win32/jpeg-vc7 external/win32/jpeg
          fi
        fi
        rm -rf external/win32/jpeg-vc6
        rm -rf external/win32/jpeg-vc7
      fi
    fi
    cd ..
  else
    echo ERROR: You do not have SZGEXTERNAL defined or the libs present for your arch.
  fi
  echo Archiving SDK and executing clean-up.
  if [ "$arch" = "win32" ]; then
    if [ "$SZG_STLPORT" = "TRUE" ]; then
      MODIFIER=win32-vc6
    else
      MODIFIER=win32-vc7
    fi
  else
    MODIFIER=$arch
  fi
  tar -cf szg-$1-$MODIFIER.tar ./szg-$1
  gzip szg-$1-$MODIFIER.tar
  rm -rf szg-$1
else
  echo SZGHOME must be set and an szg installation must exist!
fi
