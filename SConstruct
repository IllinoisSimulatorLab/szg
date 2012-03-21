# This is the top-level SCons build script.

# It runs sub-scripts (generally named 'SConscript' or 'SConscript.<something>')
# in a couple of different places.

# First, it uses SConscripts in build/scons and platform-specific subdirectories
# thereof to determine platform-specific parameters and check for 3rd-party
# libraries (which still currently have to be in $SZGEXTERNAL, but once
# we make the switch from make to SCons we might be able to be more
# flexible about that. NOTE: any external apps built using syzygy will
# probably need to run $SZGHOME/build/scons/SConscript.platform.

# Second, there is an SConscript.<dirname> file in each src directory
# that specifies what gets built from that directory.

# Libraries and executables get built in szg/build/<platform>/srcdir/.
# Executables are then copied to $SZGBIN, libraries are copied to
# szg/lib/, and header files are copied to szg/include.

# Usage: in the szg directory, typing 'scons' will build the syzygy
# libraries and demos.
# Typing 'scons -c' will delete them.
# Typing 'scons python' will build the python bindings, building
#    the main libraries and demos if needed.
# Typing 'scons doc' will build the html documentation.
# Typing 'scons all' will do all of the above.
# 'scons -c all' will delete everything built.
# In a source directory (e.g. src/math), typing 'scons -u .'
#    will build the current directory and any it depends on.
#    Typing 'scons -u -c .' will delete all built targets
#    in the current directory and any that it depends on.

# NOTE: if you need to read the SCons output, I suggest 'scons -j 1'
# (i.e. tell it to use only one concurrent job).

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

# Ditto for 'scons python' (we'll handle it later)
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

  import sys
  sys.path.append( os.path.join( os.environ['SZGHOME'], 'build', 'scons' ) )
  import odict

  # Set up dictionary of sub-directories, specifying which other
  # directories each depends on (these are the 'internal'
  # dependencies, 'external' ones are things like fmod and
  # so on and are platform-specific, so will be added in
  # Sconscript.platform).
  # NOTE: using and OrderedDict() here instead of an ordinary dict
  # to try & get scons to build the subdirectories more or less
  # in order.
  subdirs = odict.OrderedDict((
    ('language',{
      'internal':[],
      }),
    ('math'    ,{
      'internal':['language'],
      }),
    ('phleet'  ,{
      'internal':['language','math'],
      }),
    ('barrier'  ,{
      'internal':['language','math','phleet'],
      }),
    ('drivers'  ,{
      'internal':['language','math','phleet','barrier'],
      }),
    ('graphics'  ,{
      'internal':['language','math','phleet','barrier'],
      }),
    ('model'  ,{
      'internal':['language','math','phleet','barrier','graphics'],
      }),
    ('sound'  ,{
      'internal':['language','math','phleet','barrier'],
      }),
    ('interaction'  ,{
      'internal':['language','math','phleet','drivers'],
      }),
    ('framework'  ,{
      'internal':['language','math','phleet','barrier','drivers','graphics','model','sound','interaction'],
      }),
    ('utilities'  ,{
      'internal':['language','math','phleet','barrier','drivers','graphics','model','sound','interaction','framework'],
      }),
    ('demo'  ,{
      'internal':['language','math','phleet','barrier','drivers','graphics','model','sound','interaction','framework'],
      }),
    ))
  envDict['subdirs'] = subdirs

  # Populate envDict with build environments and other configuration parameters.
  # (this script calls platform-specific sub-scripts to do stuff like
  # library detection)
  SConscript('build/scons/SConscript.platform',exports='envDict')

  # Get the dictionary of build, include, lib, install paths, created
  # in Sconscript.platform.
  pathDict = envDict['paths']
  externalFlags = envDict['externalFlags']

  # Loop through the specified subdirectories and build them.
  priorLibs = []
  for srcDirname in envDict['subdirs']:
    # Use VariantDir() function instead?
    # Path to SConscript file in source subdirectory.
    sourceScriptPath = 'src/'+srcDirname+'/SConscript.'+srcDirname
    # 'variantDir' is the platform-specific build directory, e.g. build/win32.
    variantDir = pathDict['buildPath']+'/'+srcDirname
    buildEnv = envDict[srcDirname+'Env']
    # NOTE: priorLibs gets modified by each source directory's SConscript
    variables = ['srcDirname','buildEnv','pathDict','priorLibs','externalFlags']
    print 'SConscript(',sourceScriptPath,')'
    # Execute a directory-specific scons build script (contained in file sourceScriptPath)
    SConscript( sourceScriptPath, \
        variant_dir=variantDir, \
        exports=variables, \
        duplicate=0 )

# Don't build the docs by default, only if 'doc' or 'all' is
# passed to the 'scons' command on the command line.
# NOTE: '#' always refers to the directory containing the
# SConstruct script, i.e. the root szg directory.
if 'doc' in COMMAND_LINE_TARGETS or 'all' in COMMAND_LINE_TARGETS:
  buildEnv = Environment()
  SConscript( '#/docsrc/SConscript.doc', \
      src_dir='#/docsrc', \
      variant_dir='#/doc', \
      exports={'buildEnv':buildEnv}, \
      duplicate=0 )

# Build python bindings if 'python' or 'all' is passed on
# command line.
if 'python_sip/src' in COMMAND_LINE_TARGETS or 'python' in COMMAND_LINE_TARGETS or 'all' in COMMAND_LINE_TARGETS:
  sipEnv = envDict['demoEnv'].Clone()
  # Populate the sip-builder environment
  SConscript( 'build/scons/SConscript.sip', exports=['sipEnv'] )
  if sipEnv is None:
    print 'SKIPPING PYTHON BINDINGS BUILD!'
  else:
    sourcePath = 'python_sip/src/'
    buildPath = 'python_sip/build/'+envDict['platform']
    exports = ['sipEnv','pathDict','priorLibs','externalFlags']
    SConscript( sourcePath+'SConscript.python', \
        variant_dir=buildPath, \
        exports=exports,
        duplicate=0 )

BUILD_TARGETS.append('install')
