#!/bin/env python

# $Id: blobby.py,v 1.2 2006/01/27 05:45:53 schaeffr Exp $
# (c) 2004, Peter Brinkmann (brinkman@math.uiuc.edu)
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

import sys
from PySZG import *

# create and initialize distributed scene graph framework
fw=arDistSceneGraphFramework()
if not fw.init(sys.argv):
    raise PySZGException,'Unable to initialize framework.'

# Note that the arInterfaceObject is deprecated.
ifo=arInterfaceObject()
ifo.setInputDevice(fw.getInputDevice())

# configure stereo view
fw.setEyeSpacing(6.0/(2.54*12.0))
fw.setClipPlanes(0.3,1000.0)
fw.setUnitConversion(1.0)

# create transformation matrices for Blobby's joints
T={}
T['hip']=arMatrix4()
T['shoulders']=ar_translationMatrix(0,2.2,0)
T['neck']=arMatrix4()
T['rhip']=ar_translationMatrix(-.5,0,0)
T['lhip']=ar_translationMatrix(.5,0,0)
T['rshoulder']=ar_translationMatrix(-.6,0,0)*ar_rotationMatrix('z',-3.14/10)
T['lshoulder']=ar_translationMatrix(.6,0,0)*ar_rotationMatrix('z',3.14/10)
T['rknee']=ar_translationMatrix(0,-2,0)
T['lknee']=ar_translationMatrix(0,-2,0)
T['rankle']=ar_translationMatrix(0,-1.8,0)
T['lankle']=ar_translationMatrix(0,-1.8,0)
T['relbow']=ar_translationMatrix(0,-1.2,0)*ar_rotationMatrix('z',3.14/10)
T['lelbow']=ar_translationMatrix(0,-1.2,0)*ar_rotationMatrix('z',-3.14/10)
T['rwrist']=ar_translationMatrix(0,-1.4,0)
T['lwrist']=ar_translationMatrix(0,-1.4,0)

# build tree of transformation nodes
TID={}
TID['hip']=dgTransform('hip',fw.getNavNodeName(),T['hip'])
TID['rhip']=dgTransform('rhip','hip',T['rhip'])
TID['lhip']=dgTransform('lhip','hip',T['lhip'])
TID['rknee']=dgTransform('rknee','rhip',T['rknee'])
TID['lknee']=dgTransform('lknee','lhip',T['lknee'])
TID['rankle']=dgTransform('rankle','rknee',T['rankle'])
TID['lankle']=dgTransform('lankle','lknee',T['lankle'])
TID['shoulders']=dgTransform('shoulders','hip',T['shoulders'])
TID['rshoulder']=dgTransform('rshoulder','shoulders',T['rshoulder'])
TID['lshoulder']=dgTransform('lshoulder','shoulders',T['lshoulder'])
TID['relbow']=dgTransform('relbow','rshoulder',T['relbow'])
TID['lelbow']=dgTransform('lelbow','lshoulder',T['lelbow'])
TID['rwrist']=dgTransform('rwrist','relbow',T['rwrist'])
TID['lwrist']=dgTransform('lwrist','lelbow',T['lwrist'])
TID['neck']=dgTransform('neck','shoulders',T['neck'])

# attach limbs to transformation nodes
arSphereMesh(ar_translationMatrix(0,1.1,0)*ar_scaleMatrix(.2,1,.1)
    ).attachMesh('torsom','hip')
arSphereMesh(ar_scaleMatrix(.5,.1,.05)).attachMesh('hipm','hip')
arSphereMesh(ar_scaleMatrix(.7,.1,.05)).attachMesh('shoulder','shoulders')
arSphereMesh(ar_translationMatrix(0,.8,0)*ar_scaleMatrix(.3,.6,.1)
    ).attachMesh('headm','neck')
arSphereMesh(ar_translationMatrix(0,.7,.15)*ar_scaleMatrix(.1)
    ).attachMesh('nosem','neck')
arSphereMesh(ar_translationMatrix(0,-1,0)*ar_scaleMatrix(.1,1,.1)
    ).attachMesh('rthighm','rhip')
arSphereMesh(ar_translationMatrix(0,-1,0)*ar_scaleMatrix(.1,1,.1)
    ).attachMesh('lthighm','lhip')
arSphereMesh(ar_translationMatrix(0,-.9,0)*ar_scaleMatrix(.1,.9,.1)
    ).attachMesh('rlowerlegm','rknee')
arSphereMesh(ar_translationMatrix(0,-.9,0)*ar_scaleMatrix(.1,.9,.1)
    ).attachMesh('llowerlegm','lknee')
arSphereMesh(ar_translationMatrix(0,-.1,0.2)*ar_scaleMatrix(.2,.1,.3)
    ).attachMesh('rfootm','rankle')
arSphereMesh(ar_translationMatrix(0,-.1,0.2)*ar_scaleMatrix(.2,.1,.3)
    ).attachMesh('lfootm','lankle')
arSphereMesh(ar_translationMatrix(0,-.6,0)*ar_scaleMatrix(.1,.6,.1)
    ).attachMesh('rupperarmm','rshoulder')
arSphereMesh(ar_translationMatrix(0,-.6,0)*ar_scaleMatrix(.1,.6,.1)
    ).attachMesh('lupperarmm','lshoulder')
arSphereMesh(ar_translationMatrix(0,-.7,0)*ar_scaleMatrix(.08,.7,.08)
    ).attachMesh('rforearmm','relbow')
arSphereMesh(ar_translationMatrix(0,-.7,0)*ar_scaleMatrix(.08,.7,.08)
    ).attachMesh('lforearmm','lelbow')
arSphereMesh(ar_translationMatrix(0,-.3,0)*ar_scaleMatrix(.15,.3,.05)
    ).attachMesh('rhandm','rwrist')
arSphereMesh(ar_translationMatrix(0,-.3,0)*ar_scaleMatrix(.15,.3,.05)
    ).attachMesh('lhandm','lwrist')

# some lights...
dgLight('light0',fw.getNavNodeName(),0,arVector4(0,0,1,0),arVector3(0,1,1))
dgLight('light1',fw.getNavNodeName(),1,arVector4(0,0,-1,0),arVector3(0,1,1))
dgLight('light2',fw.getNavNodeName(),2,arVector4(0,-1,0,0),arVector3(.5,0,.5))
dgLight('light3',fw.getNavNodeName(),3,arVector4(0,1,0,0),arVector3(.5,0,.5))

# now start the framework
if not fw.start():
    raise PySZGException,'Unable to start framework.'
if not ifo.start():
    raise PySZGException,'Unable to start interface.'
northwall=ar_translationMatrix(0,5,-5)
ifo.setNavMatrix(northwall)
ar_setNavMatrix(ifo.getNavMatrix()**-1)
fw.loadNavMatrix()

# main routine
if __name__=='__main__':
    import time
    dgTransform('sign','root',northwall*
      ar_translationMatrix(3,0,0)*
            ar_scaleMatrix(.3))
    signID=dgBillboard('signb','sign',1,'hello')
    while 1:
        for joint in T.keys():
            dgBillboard(signID,1,joint)
            while fw.getButton(1):
                pass
            ifo.setObjectMatrix(arMatrix4())
            while not fw.getButton(1):
                ar_setNavMatrix(ifo.getNavMatrix()**-1)
                fw.loadNavMatrix()
                dgTransform(TID[joint],T[joint]*ifo.getObjectMatrix()**-1)
                fw.setViewer()
                if fw.getButton(2):
                    ifo.setObjectMatrix(arMatrix4())
                    for j in T.keys():
                        dgTransform(TID[j],T[j])
                time.sleep(0.02)
