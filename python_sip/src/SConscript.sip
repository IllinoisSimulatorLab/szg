import os

Import('buildEnv','pathDict','priorLibs','externalFlags')

buildPath = os.getcwd()
targetName = 'szg'

# Call the generic sip-builder script
scriptPath = os.path.join( os.environ['SZGHOME'], 'build', 'scons', 'SipBuild.py' )
execfile( scriptPath )


