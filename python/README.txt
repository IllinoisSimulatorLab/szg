
pyszg created by Peter Brinkman, copyright 2004


This file describes how to build the bindings on the various platforms
supported by syzygy.

0. To use these bindings, you will need Python 2.2 or greater as well as
   SWIG 1.3 or greater.
   a. For a recent linux system, these requirements should be no problem.
      Slightly older systems, like RedHat 9, can be upgraded, assuming
      they have python 2.2 or better. On these systems, upgrading swig
      involves downloading the source to a recent version, compiling,
      and installing.
   b. For a windows system, download and install python. Then download
      and install swig. NOTE: It seems that python (as available from 
      cygwin) is NOT able to open the DLL created by the following process.
      Consequently, the solution is:
      i. Uninstall cygwin's python (if you had it installed).
      ii. Uninstall cygwin's swig.
      iii. Install a reasonable version of python from python.org.
      iv. Make sure the top level of the python directory is in your path.
      iv. Download a windows version of SWIG from swig.org. Put the
          SWIG folder somewhere and make sure its top level is in your path.
          NOTE: it is not sufficient to copy the swig.exe somewhere! 
   c. Further notes for installing python to a network drive:
      i. Copy the SWIG folder to the drive. Put the folder in your path.
      ii. Copy the Python folder to the drive. Get the python dll
          (python22.dll for Python 2.2, for instance) and copy it into the
          top level of your python install. Make sure this folder is in your
          path.
      iii. That's it!

1. First, build syzygy from source. (the "easy" developer model is not yet
   supported)

2. Next , make sure the following environment variables are set

   SZG_PYINCLUDE      (the location of the python include files)
   SZG_PYLIB          (the location of the python library)
 
   a. On a linux system, with python 2.2,

      SZG_PYINCLUDE=/usr/include/python2.2
      SZG_PYLIB=

      The second variable can be left blank because, on linux, the
      build should automatically find the dll.
      
   b. On a windows system, with python2.2 installed in c:, these variables
      will look like:

      SZG_PYINCLUDE=c:/Python22/include
      SZG_PYLIB=c:/Python22/libs/python22.lib

      NOTE: we are building inside of cygwin or mingw. Consequently,
      forward slashes are required instead of back slashes.

3. To run the stuff, you have to make sure your PYTHONPATH environment
   variable includes $(SZGBIN), which is where your executables will be
   placed.

******************* THE ORIGINAL README FOLLOWS ***********************************

PySZG --- tentative Python bindings for Syzygy's Scene Graph API


Requirements:

    - Software
        - A recent installation of szg (from the CVS repository; the
          version posted on the ISL website doesn't cut it; if you want
          szg to work through firewalls, then you'll need my patched
          version that's installed in the grafixLab)
        - A recent version of Python (I've got Python 2.3 on my machine;
          Python 2.2 seems to be okay. I don't make any promises regarding
          anything that's older)
        - A recent version of SWIG (I'm using 1.3.21; I don't know which
          other versions might still work)
        - For the Master/Slave framework: PyOpenGL
    - Environment
        - SZGHOME must point to your szg directory
        - PYSZGLIB must point to some directory that you can write to,
          such as ~/lib
        - PYTHONPATH must include $PYSZGLIB; I accomplish this by saying
            export PYTHONPATH=$PYTHONPATH:$PYSZGLIB


Installation:

In the pyszg/bindings directory, say

    make
    make test
    make install

The second step, make test, is optional as well as sort of pointless
at the moment as the testing suite, test.py, is still _very_ incomplete.

That's it! If nothing went wrong, you can now try the demos in pyzsg/demos.


