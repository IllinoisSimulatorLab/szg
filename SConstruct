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
# libraries and demos (equivalent to typing 'scons #').
# Typing 'scons -c' will delete them.
# Typing 'scons python' will build the python bindings, building
#    the main libraries and demos if needed (equivalent to
#    'scons python_sip/src').
# Typing 'scons doc' will build the html documentation.
# Typing 'scons all' will do all of the above.
# 'scons -c all' will delete everything built.
# Typing 'scons phleet' will build the subdirectories thru phleet.
#    (equivalent to 'scons src/phleet').
# Typing 'scons drivers' will build thru drivers.
#    (equivalent to 'scons src/drivers').
# In a source directory (e.g. src/math), typing 'scons -u .'
#    will build the current directory and any it depends on.
#    Typing 'scons -u -c .' will delete all built targets
#    in the current directory and any that it depends on.
#
# NOTE: passing multiple targets doesn't work, it seems
# only the first one gets built, e.g. 'scons phleet drivers'
# only builds phleet.
#
# NOTE: as indicated above, the 'clean' option ('scons -c <target>')
# works rather differently from make's. E.g., 'scons -c phleet' will
# delete everything built in language, math, and phleet (the target
# and all internal dependencies).
# On the upside, scons seems to be pretty good about scanning for
# header file dependencies (i.e. if you modify a header file it
# should recompile files that include it), so hopefully cleaning
# won't be necessary as often.
#
# NOTE: if you need to read the SCons output, I suggest 'scons -j 1'
# (i.e. tell it to use only one job, it defaults to at least 2 depending
# on the platform. On windows it uses the NUMBER_OF_PROCESSORS env var).


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


# Build only thru phleet
if 'phleet' in COMMAND_LINE_TARGETS:
  BUILD_TARGETS.remove('phleet')
  BUILD_TARGETS.append('#/src/phleet')

# Build only thru drivers
if 'drivers' in COMMAND_LINE_TARGETS:
  BUILD_TARGETS.remove('drivers')
  BUILD_TARGETS.append('#/src/drivers')

# Do all the build environments and checks unless the user
# just wants to build the docs.
if COMMAND_LINE_TARGETS != ['doc']:
  import sys
  sys.path.append( os.path.join( os.getcwd(), 'build', 'scons' ) )
  import odict
  import szgscons

  # get version compile flags
  envDict = {'versionFlags':szgscons.getVersionFlags()}

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

  if 'phleet' in COMMAND_LINE_TARGETS:
    subdirs = subdirs[:(subdirs.index('phleet')+1)]
  elif 'drivers' in COMMAND_LINE_TARGETS:
    subdirs = subdirs[:(subdirs.index('drivers')+1)]

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
  useGL = False
  for srcDirname,dependencies in envDict['subdirs'].iteritems():

    # Path to SConscript file in source subdirectory.
    sourceScriptPath = 'src/'+srcDirname+'/SConscript.'+srcDirname

    # 'variantDir' is the platform-specific build directory, e.g. build/win32.
    variantDir = pathDict['buildPath']+'/'+srcDirname

  # Link against graphics libraries for graphics and later directories.
    if srcDirname == 'graphics':
      useGL = True
    if useGL:
      buildEnv = envDict['szgEnv'].Clone()
    else:
      buildEnv = envDict['szgNoGLEnv'].Clone()
    
    # Call a utility function to fill in some build parameters
    szgscons.updateSzgParams( buildEnv, dependencies, externalFlags )

    # NOTE: priorLibs gets modified by each source directory's SConscript
    # It's a list of the already-built subdirectories.
    variables = ['srcDirname','buildEnv','pathDict','priorLibs','externalFlags']

    print 'SConscript( %s )' % sourceScriptPath
    # Execute a directory-specific scons build script (contained in file sourceScriptPath)
    SConscript( sourceScriptPath, \
        variant_dir=variantDir, \
        exports=variables, \
        duplicate=0 )


# Build python bindings if 'python' or 'all' is passed on
# command line.
if 'python_sip/src' in COMMAND_LINE_TARGETS or 'python' in COMMAND_LINE_TARGETS or 'all' in COMMAND_LINE_TARGETS:
  sipEnv = buildEnv.Clone()
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

    
# This rigamarole (along with creating and alias named 'install'
# for $SZGBIN in build/scons/SConscript.platform) is necessary
# if we want libraries and executables to be automatically copied
# to an SZGBIN directory that is _outside_ of szg.
# Alternatively, we could remove this line and manually issue
# the command 'scons install' to do the copy (building the
# stuff to be copied if necessary).
BUILD_TARGETS.append('install')
BUILD_TARGETS.append('headers')
