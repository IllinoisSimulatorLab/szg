// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU LGPL as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/lesser.html).


// This is the main file for the szg Python bindings. 
// These bindings are supposed to serve the following purposes:
//      - proof of concept: establish that Python bindings for szg work
//      - introduce and illustrate ideas and issues that come up when
//        creating Python bindings for C++ code
//      - provide a point of departure for illiMath04: It is my hope that
//        a prototype that actually compiles will avoid the initial drudgery
//        of figuring out compiler options and such and let us get into
//        conceptual work more quickly.

// NOTE: the module name is now passed in by the makefile. DO NOT SET IT HERE!
//%module PySZG

// Include (some) pertinent C++ headers. Others are included as needed in the
// individual .i files.
%{
#include <iostream>
#include <sstream>
#include <math.h>
#include <string>
#include "arThread.h"
#include "arMath.h"
#include "arNavigationUtilities.h"
#include "arDrawableNode.h"
#include "arGraphicsAPI.h"
#include "arGraphicsHeader.h"
#include "arMesh.h"
#include "arOBJ.h"
#include "ar3DS.h"
#include "arInputNode.h"
#include "arInterfaceObject.h"
#include "arDistSceneGraphFramework.h"
#include "arMasterSlaveFramework.h"
#include "arSoundAPI.h"
%}

// derive new exception for PySZG
%pythoncode %{
import cPickle
import pickle
import os
import sys
import struct

# I dont know why this was here, but it does not look like a good thing.
# Jim 2/15/07
#try:
#    os.chdir(sys.path[0])
#except:
#    pass

def launchConsole(loc=globals()):
    from threading import Thread
    from code import interact
    Thread(target=interact,args=(None,None,loc)).start()

class PySZGException(Exception):
    pass

def getSwigModuleDll():
  dllName = '_'+__name__
  l = [item for item in sys.modules.items() if dllName in item]
  if len(l) != 1:
    raise RuntimeError, str(len(l))+' modules named '+dllName
  return l[0][1] 
%}

%include exception.i    // SWIG module for handling exceptions

%include PyTypemaps.i   // Typemaps for conversions between Python types
                        // and C++ types.
%include PyArrays.i
%include PyMath.i        // Wrappers for math/arMath.h
%include PyDataUtilities.i
%include PyPhleet.i   // Wrappers for phleet objects
%include PyGraphics.i    // Wrappers for graphics/arGraphicsAPI.h and more
%include PyObj.i         // Wrappers for obj/arOBJ.h and more
%include PyInputEvents.i
%include PyInteraction.i
%include PyAppLauncher.i
%include PyEventFilter.i
%include PySZGApp.i      // Wrappers for framework/arSZGAppFramework.h.
%include PySceneGraph.i  // Wrappers for framework/arDistSceneGraphFramework.h
%include PyMasterSlave.i // Wrappers for framework/arMasterSlaveGraphFramework.h
%include PySoundAPI.i    // Wrappers for sound/arSoundAPI.h
%include PyPeer.i        // Support for the Myriad distributed scene graph.

#ifdef AR_SWIG_SZGEXPT
%include PyExperiment.i
#endif

