# This script gets execfile()'d from the individual source directories'
# SConscript scripts...

import os
import sys

# Directory-specific compile and link flags
flags = {'LIBPATH':[pathDict['buildPath']+'/'+dirName]}
if sys.platform == 'win32':
  flags['CCFLAGS'] = ['-D SZG_COMPILING_'+dirName.upper()]
buildEnv.MergeFlags( flags )

# Build the library
libName = 'ar'+dirName.capitalize()
libObjs = [buildEnv.Object(s) for s in libSrc]
if os.environ.get( 'SZG_LINKING', 'STATIC' ) == 'DYNAMIC':
  lib = buildEnv.SharedLibrary( target=libName, source=libObjs )
else:
  libName += '_static'
  lib = buildEnv.StaticLibrary( target=libName, source=libObjs )

# Install the library in szg/lib/<platform>
buildEnv.Install( pathDict['libPath'], lib )

# Build the programs.
progEnv = buildEnv.Clone()
# Our libraries _MUST_ be prepended to the system library list!!!!!
progEnv.Prepend( LIBS=[libName] )
progs = [progEnv.Program( source=[p+'.cpp'] ) for p in progNames]

#Install the headers in szg/include.
srcPath = os.path.join( os.environ['SZGHOME'], 'src', dirName )
headers = [f for f in os.listdir(srcPath) if os.path.splitext(f)[1]=='.h']
buildEnv.Install( pathDict['includePath'], headers )

# Install the programs in $SZGBIN or szg/bin/<platform>
#buildEnv.Install( pathDict['installPath'], results )


