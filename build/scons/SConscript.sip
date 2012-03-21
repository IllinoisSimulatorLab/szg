#
# Hackish SCons tool for creating Python bindings using sip.
#

# sipEnv is created (as a copy of the standard syzygy build
# environment) in the calling script and populated here.
Import('sipEnv')

import os
import sys
import shutil
import re


if sys.platform == 'win32':
  sipEnv['no_import_lib'] = True
  # We'd like to be able to suppress creation of .def files for 
  # driver plugins, but a bug in SCons causes the dll's to be
  # re-linked every time if we do this.
  #sipEnv['WINDOWS_INSERT_DEF'] = False
  #
  # For now, in order to get things to work properly, we have to tell
  # SCons to generate a .def file for the driver plugins, even though
  # we don't need it, because otherwise a bug in SCons causes the
  # plugins to be re-linked whether needed or not.
  sipEnv['WINDOWS_INSERT_DEF'] = True

  # Python extensions on win32 are shared libraries with .pyd suffix...
  sipEnv['SHLIBSUFFIX'] = '.pyd'

elif 'linux' in sys.platform:
  # By default on linux, scons (at least version 1.2) assumes
  # that you want to prepend 'lib' to all your shared library
  # names. We don't want that for the _szg python extension.
  sipEnv['SHLIBPREFIX'] = ''



# Add compiler and linker flags...
sipFlags = {}
sipFlags['CPPPATH'] = [os.environ['SZG_PYINCLUDE']]
sipFlags['LIBS'] = [os.environ['SZG_PYLIB']]
if sys.platform == 'win32':
  sipFlags['CPPPATH'].append( os.environ['SIP_INCLUDE'] )
  sipFlags['LIBPATH'] = [os.path.dirname( os.environ['SZG_PYLIB_PATH'] )]

sipEnv.MergeFlags( sipFlags )

# Done adding flags. Now setup the SIP builder...

# We'll use a Python function to call SIP. There must be one source file:
#   <targetName>.sip
# Sip generates two intermediate files in the build
# directory, sipAPI_<targetName>.h and sip_<targetName>part0.cpp; the
# build function renames the latter to sip_<targetName>.cpp. These
# are compiled and linked into: (win32) _<targetName>.pyd;
# or (other) _<targetName>.so. This gets installed to binPath.
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
  # success.
  return None


# Create the builder. Pass the build function just defined. The build
# function moves the output file to <target dir>/<source>_sip.cpp. We
# can't set up a builder for the actual SIP output file because if we
# provide a prefix, a '.' gets prepended to the suffix. Pretty sure that's
# and SCons bug.
bld = Builder( action=sipBuildFunc,  suffix='_sip.cpp', src_suffix='.sip' )

# Install the builder.
sipEnv['BUILDERS']['SipPythonExtensionSrc'] = bld

# Define a 'scanner' to scan the source file for any %Include's...
# (which should then be recognized by scons as dependencies...)

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
  print 'SIP Includes: %s' % str( includes )
  return includes

# Instantiate scanner object...
sipScan = Scanner( function=sipFileScan, skeys = ['.sip'] )
# ...and install it in the build environment.
sipEnv.Append( SCANNERS=sipScan )

