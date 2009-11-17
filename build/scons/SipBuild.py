# SCons script for building a python extension with SIP, meant to be
# execfile()d rather than SConscript()ed. It's messy because SCons
# doesn't support SIP yet.

import os
import sys
import shutil
import re


# Add compiler and linker flags...
sipFlags = {}
sipFlags['CPPPATH'] = [os.environ['SZG_PYINCLUDE']]
sipFlags['LIBS'] = [os.environ['SZG_PYLIB']]
if sys.platform == 'win32':
  sipFlags['CPPPATH'].append( os.environ['SIP_INCLUDE'] )
  sipFlags['LIBPATH'] = [os.path.dirname( os.environ['SZG_PYLIB_PATH'] )]

buildEnv.MergeFlags( sipFlags )

# Done adding flags. Now setup the SIP builder...

# We'll use a Python function to call SIP. There must be two files:
#   <targetName>.sip
#   <targetName>.py
# The .py file gets copied to the build directory and then Install()ed
# to binPath. Sip generates two intermediate files in the build
# directory, sipAPI_<targetName>.h and sip_<targetName>part0.cpp; the
# build function renames the latter to sip_<targetName>.cpp. These
# are compiled and linked into: (win32) _<targetName>.dll, which gets
# moved to _<targetName>.pyd; or (other) _<targetName>.so. This also
# gets installed to binPath.
def sipBuildFunc( target, source, env ):
  # SIP only accepts a single source file (I think); ignore any additional
  # sources.
  sourceFilePath = str(source[0])
  sourceDirectory, sourceFile = os.path.split( sourceFilePath )
  sourceFileBase, sourceFileExt = os.path.splitext( sourceFile )
  # The target filename is actually ignored, we just need to get the
  # directory path.
  targetFilePath = str(target[0])
  targetDirectory, targetFile = os.path.split( targetFilePath )
  # Compute the SIP output cpp file name. SIP also creates a header
  # file, but we're not doing anything to that in here.
  sipOutputFile = os.path.join( targetDirectory, 'sip_' + os.path.splitext( sourceFile )[0] + 'part0.cpp' )
  # Run the SIP command.
  sipCommand = 'sip -c '+targetDirectory+' -j 1 -I '+sourceDirectory+' '+sourceFilePath
  print 'SIP command:',sipCommand
  os.system( sipCommand )
  # Move the output cpp file to <target dir>/<source>_sip.cpp.
  shutil.move( sipOutputFile, targetFilePath )
  # Copy the .py file.
  pyName = sourceFileBase + '.py'
  pyFile = os.path.join( sourceDirectory, pyName )
  dest = os.path.join( targetDirectory, pyName )
  print 'Copy(',pyFile,',',dest,')'
  shutil.copyfile( pyFile, dest )
  # success.
  return None


# Create the builder. Pass the build function just defined. The build
# function moves the output file to <target dir>/<source>_sip.cpp. We
# can't set up a builder for the actual SIP output file because if we
# provide a prefix, a '.' gets prepended to the suffix. Pretty sure that's
# and SCons bug.
bld = Builder( action=sipBuildFunc,  suffix='_sip.cpp', src_suffix='.sip' )

# Install the builder.
buildEnv['BUILDERS']['SipPythonExtensionSrc'] = bld

# Define a 'scanner' to scan the source file for any %Include's...

# Define a regex to detect '%Include <file>.sip'
sipIncludeRe = re.compile(r'^%Include\s+(\S+\.sip)$', re.MULTILINE)
#sipIncludeRe = re.compile(r'%Include\s+(\S+\.sip)')

# Scan function
def sipFileScan( node, env, path ):
  contents = node.get_contents()
  # Python regex '^' and '$' don't work properly with win32 CR+LF endings.
  # So get rid of the CR.
  contents = contents.replace( '\r', '' )
  #print 'contents:'
  #for c in contents:
  #  print ord(c),c
  includes = sipIncludeRe.findall( contents )
  print 'SIP Includes:'
  for i in includes:
    print '  ',i
  return includes

# Instantiate scanner object...
sipScan = Scanner( function=sipFileScan, skeys = ['.sip'] )
# ...and install it in the build environment.
buildEnv.Append( SCANNERS=sipScan )

# Build the SIP bindings cpp and header file.
sipCpp = buildEnv.SipPythonExtensionSrc( os.path.join( buildPath, targetName ) )

# Tell SCons about the header file, so it knows to clean it up.
Clean( sipCpp, os.path.join( buildPath, 'sipAPI_'+targetName+'.h' ) )

# Build the Python extension as a shared library with a name
# beginning with '_'.
targetDll = buildEnv.SharedLibrary( target=os.path.join( buildPath, '_'+targetName ), source=[sipCpp] )
targetPyExt = targetDll

# On windows, rename the .dll -> .pyd
if sys.platform == 'win32':
  # This involved syntax creates a new node in the build tree and removes or
  # hides the source (.dll) one, so that 'scons -c' removes the .pyd file
  # and not the missing .dll.
  targetPyExt = Command( os.path.join( buildPath, '_'+targetName+'.pyd' ), \
      os.path.join( buildPath, '_'+targetName+'.dll' ), Move("$TARGET", "$SOURCE"))

buildEnv.Install( pathDict['binPath'], targetPyExt )
buildEnv.Install( pathDict['binPath'], targetName+'.py' )
