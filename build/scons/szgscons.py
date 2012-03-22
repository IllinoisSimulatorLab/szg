import os

def getVersionFlags():
  """Return compiler flags specifying current Syzygy version."""
  return {'CCFLAGS':[ 
      '-D SZG_MAJOR_VERSION=1',
      '-D SZG_MINOR_VERSION=3',
      '-D SZG_PATCH_VERSION=1'
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


