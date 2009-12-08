import os
import sys


# If the user types e.g. 'scons all'...
if 'all' in COMMAND_LINE_TARGETS:
  # remove 'all' from the targets, because there isn't
  # any such directory (SCons targets are files or
  # directories)...
  BUILD_TARGETS.remove('all')
  # Add '#' (this directory, szg, which is the default
  # target & gets built if you just type 'scons') and
  # doc/txt2tags to build targets.
  BUILD_TARGETS.extend(['#'])

if 'python' in COMMAND_LINE_TARGETS:
  BUILD_TARGETS.remove('python')
  BUILD_TARGETS.append('#')


# Do all the build environments and checks unless the user
# just wants to build the docs.
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

  # Loop through the specified subdirectories and build them.
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

# Don't build the docs by default.
if 'doc' in COMMAND_LINE_TARGETS or 'all' in COMMAND_LINE_TARGETS:
  buildEnv = Environment()
  SConscript( '#/docsrc/SConscript.doc', \
      src_dir='#/docsrc', \
      variant_dir='#/doc', \
      exports={'buildEnv':buildEnv}, \
      duplicate=0 )

if 'python_sip/src' in COMMAND_LINE_TARGETS or 'python' in COMMAND_LINE_TARGETS or 'all' in COMMAND_LINE_TARGETS:
  sourcePath = 'python_sip/src/'
  buildPath = 'python_sip/build/'+envDict['platform']
  buildEnv = envDict['demoEnv']
  exports = ['buildEnv','pathDict','priorLibs','externalFlags']
  SConscript( sourcePath+'SConscript.sip', \
      variant_dir=buildPath, \
      exports=exports,
      duplicate=0 )

