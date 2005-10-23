// $Id: PySZG.i,v 1.5 2005/10/23 00:21:37 schaeffr Exp $
// (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).


// This file is the main file of a set of tentative Python bindings for
// szg. These bindings are supposed to serve the following purposes:
//      - proof of concept: establish that Python bindings for szg work
//      - introduce and illustrate ideas and issues that come up when
//        creating Python bindings for C++ code
//      - provide a point of departure for illiMath04: It is my hope that
//        a prototype that actually compiles will avoid the initial drudgery
//        of figuring out compiler options and such and let us get into
//        conceptual work more quickly.
//
// These bindings are _not_ supposed to be complete, efficient, well-designed,
// or even correct. There probably are a few memory leaks and other problems.


%module PySZG

// Include pertinent C++ headers. (Is this the best place to put the headers?)
%{
#include <iostream>
#include <sstream>
#include <math.h>
#include <string>
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

try:
    os.chdir(sys.path[0])
except:
    pass

def launchConsole(loc=globals()):
    from threading import Thread
    from code import interact
    Thread(target=interact,args=(None,None,loc)).start()

class PySZGException(Exception):
    pass
%}

%include exception.i    // SWIG module for handling exceptions

%include PyTypemaps.i   // Typemaps for conversions between Python types
                        // and C++ types.
%include PyArrays.i
%include PyMath.i       // Wrappers for math/arMath.h
%include PyDataUtilities.i
%include PySZGClient.i  // Wrappers for phleet/arSZGClient.h
%include PyGraphics.i   // Wrappers for graphics/arGraphicsAPI.h and more
%include PyObj.i        // Wrappers for obj/arOBJ.h and more
%include PyInputEvents.i
%include PyInteraction.i
%include PyAppLauncher.i
%include PyEventFilter.i
%include PySZGApp.i     // Wrappers for framework/arSZGAppFramework.h
%include PySceneGraph.i // Wrappers for framework/arDistSceneGraphFramework.h
                        // and more
%include PyMasterSlave.i // Wrappers for framework/arMasterSlaveGraphFramework.h
%include PySoundAPI.i   // Wrappers for sound/arSoundAPI.h
%include PyPeer.i       // Support for the Myriad distributed scene graph.

#ifdef AR_SWIG_SZGEXPT
%include PyExperiment.i
#endif

