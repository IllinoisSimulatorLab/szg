import os
import sys

def getVersionFlags():
  """Return compiler flags specifying current Syzygy version."""
  return {'CCFLAGS':[ 
      '-D SZG_MAJOR_VERSION=1',
      '-D SZG_MINOR_VERSION=4',
      '-D SZG_PATCH_VERSION=0'
      ]}


def makeLibName( dirName ):
  """Convert a directory name into a Syzygy library name."""  
  libName = 'ar'+dirName.capitalize()
  if os.environ.get( 'SZG_LINKING', 'STATIC' ) != 'DYNAMIC':
    libName += '_static'
  return libName
    

def updateSzgParams( buildEnv, dependencies, externalFlags ):
  """Fill in some build environment parameters related to the
  internal & external dependencies."""

  # Add headers & libs from already-built subdirectories to the
  # current build environment.
  for d in dependencies['internal']:
    buildEnv.Prepend( LIBS=makeLibName(d) )
    # NOTE: the paths in CPPPATH are evaluated at compile time.
    # Without the '#' (meaning 'the directory containing SConstruct',
    # i.e. szg) they would be interpreted as relative to the current
    # build directory, e.g. szg/build/win32/language.
    buildEnv.Prepend( CPPPATH=['#/src/'+d] )

  # Add compiler & linker args for external dependencies.
  for flagGroup in dependencies['external']:
    buildEnv.MergeFlags( externalFlags[flagGroup] )



# Method called to build each of the src subdirectories.
def buildSzgSubdirectory( srcDirname, buildEnv, libSrc, progNames, priorLibs, pathDict ):
  # Directory-specific compile and link flags
  flags = {'LIBPATH':[pathDict['buildPath']+'/'+srcDirname]}
  if sys.platform == 'win32':
    flags['CCFLAGS'] = ['-D SZG_COMPILING_'+srcDirname.upper()]

  buildEnv.MergeFlags( flags )

  # libSrc gets passed from the calling script & is a list of
  # source files to build the current directory's library.
  if len( libSrc ) > 0:
    # Build the library
    libName = 'ar'+srcDirname.capitalize()
    libObjs = [buildEnv.Object(s) for s in libSrc]

    # Determine whether to build static or dynamic library,
    # default to static.
    if os.environ.get( 'SZG_LINKING', 'STATIC' ) == 'DYNAMIC':
      # BUG: platform-dependent? Doesn't matter yet as we don't
      # support dynamic linking with MinGW
      sharedLib, linkLib, defFile = buildEnv.SharedLibrary( target=libName, source=libObjs )
    else:
      libName += '_static'
      lib = buildEnv.StaticLibrary( target=libName, source=libObjs )
      linkLib = lib
      sharedLib = []

    # Install the library in (i.e. copy to) szg/lib/<platform>
    globalLinkLib = buildEnv.Install( pathDict['libPath'], linkLib )
    buildEnv.Install( pathDict['binPath'], linkLib )

  progEnv = buildEnv.Clone()

  # Our libraries _MUST_ be prepended to the system library list!!!!!
  if len( libSrc ) > 0:
    progEnv.Prepend( LIBS=[libName] )

  # Build the programs.
  progs = [progEnv.Program( source=[p+'.cpp'] ) for p in progNames]

  # specify that each depends on all of the Syzygy libraries built up til now.
  # (actually, it should use the subdirs 'external' list to determine this,
  # doing it this was is overkill but simpler). Note priorLibs also gets
  # bassed from the calling script.
  for p in progs:
    progEnv.Depends( p, priorLibs )

  # Add the current library to priorLibs for the next source directory.
  if len( libSrc ) > 0:
    priorLibs.append( linkLib )
    progs.append( sharedLib )

  #Install the headers in szg/include/<platform>.
  headers = buildEnv.Glob( '#/src/%s/*.h' % srcDirname )
  buildEnv.Install( pathDict['includePath'], headers )

  # Install the programs in $SZGBIN or szg/bin/<platform>
  #print 'installing in',pathDict['binPath']
  #for p in progs:
  #  print str(p)
  buildEnv.Install( pathDict['binPath'], progs )

  return progEnv
