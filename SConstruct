import os
import sys

# Set version compile flags
versionFlags = {'CCFLAGS':[ 
    '-D SZG_MAJOR_VERSION=1',
    '-D SZG_MINOR_VERSION=3',
    '-D SZG_PATCH_VERSION=1'
    ]}

envDict = {'versionFlags':versionFlags}

# Populate envDict with build environments and other configuration parameters.
SConscript('build/scons/SConscript.platform',exports='envDict')

# Get the dictionary of build, include, lib, install paths.
pathDict = envDict['paths']

subdirs = ('language','math','phleet','barrier')
for d in subdirs:
  sourcePath = 'src/'+d+'/SConscript.'+d
  variantDir = pathDict['buildPath']+'/'+d
  buildEnv = envDict[d+'Env']
  exports = ['buildEnv','pathDict']
  SConscript( sourcePath, \
      variant_dir=variantDir, \
      exports=exports, \
      duplicate=0 )

