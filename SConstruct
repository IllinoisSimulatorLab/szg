import os
import sys

if COMMAND_LINE_TARGETS != ['doc']:
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
  externalFlags = envDict['externalFlags']
  priorLibs = []

  for srcDirname in envDict['subdirs']:
    sourcePath = 'src/'+srcDirname+'/SConscript.'+srcDirname
    variantDir = pathDict['buildPath']+'/'+srcDirname
    buildEnv = envDict[srcDirname+'Env']
    # NOTE: priorLibs gets modified by each directories' SConscript
    exports = ['srcDirname','buildEnv','pathDict','priorLibs','externalFlags']
    print 'SConscript(',sourcePath,')'
    SConscript( sourcePath, \
        variant_dir=variantDir, \
        exports=exports, \
        duplicate=0 )

if 'doc' in COMMAND_LINE_TARGETS or 'all' in COMMAND_LINE_TARGETS:
  SConscript( '#/doc/txt2tags/SConscript.doc' )
