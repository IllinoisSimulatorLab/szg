import sys
import os
import subprocess
import traceback
from StringIO import StringIO

def getBzrVersion():
  try:
    import bzrlib.commands
    saveout = sys.stdout
    myOut = StringIO()    
    sys.stdout = myOut
    exit_val = bzrlib.commands.run_bzr(['version-info','--rio'])
    sys.stdout = saveout
    versionString = myOut.getvalue()
    myOut.close()
  except ImportError:
    pipe = subprocess.Popen( 'bzr version-info --rio', shell=True, \
                  stdout=subprocess.PIPE).stdout
    versionString = pipe.read()
    pipe.close()
  versionString = versionString.replace('\n',r'\n')
  return versionString

def makeFile( srcDir, versionString ):
    templatePath = os.path.join( srcDir, 'arVersionTemplate.cpp' )
    outfilePath = os.path.join( srcDir, 'arVersion.cpp' )
    tFile = file( templatePath, 'r' )
    outFile = file( outfilePath, 'w' )
    foundFirst = False
    for line in tFile:
      if not foundFirst and 'REPLACE_THIS' in line:
        line = line.replace( 'REPLACE_THIS', versionString )
        foundFirst = True
      outFile.write( line )
    outFile.close()
    tFile.close()

if __name__ == '__main__':
  try:
    if len(sys.argv) != 2:
      print 'Usage: make_arVersion <srcdir>.\n'
      sys.exit(1)
    srcDir = sys.argv[1]
    versionString = getBzrVersion()
    print 'version info:',versionString
    makeFile( srcDir, versionString )
    print 'make_arVersion.py succeeded'
  except:
    print 'WARNING: FAILED TO INSERT VERSION INFO'
    print 'Are Python <www.python.org> and bazaar <bazaar-vcs.org> installed?'
    print '(non-fatal error)'
    traceback.print_exc( file=sys.stdout )
    sys.exit(1)
