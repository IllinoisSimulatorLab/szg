import sys
import os
import traceback
from StringIO import StringIO
import traceback

print 'make_arVersion:',__file__

TEMPLATE = '"revno          : {revno}\\ndate           : {date}\\nrevision id    : {revision_id}\\nbranch nickname: {branch_nick}"'

def getBzrVersion():
  try:
    import bzrlib.commands
    from bzrlib.errors import BzrCommandError
    saveout = sys.stdout
    myOut = StringIO()    
    sys.stdout = myOut
    sys.stdout = saveout
    import subprocess
    pipe = subprocess.Popen( 'bzr version-info --custom --template=%s' % TEMPLATE, shell=True, \
                stdout=subprocess.PIPE).stdout
    versionString = pipe.read().strip().split('\n')
    print versionString
    print '============================================='
    pipe.close()
    versionString = '\n'.join( versionString[-4:] )
  except ImportError:
    try:
      import subprocess
      sys.stderr.write( 'No bzrlib module, trying subprocess module.\n' )
      pipe = subprocess.Popen( 'bzr version-info --custom --template=%s' % TEMPLATE, shell=True, \
                  stdout=subprocess.PIPE).stdout
      versionString = pipe.read().split('\n')
      print versionString
      print '============================================='
      pipe.close()
      versionString = '\n'.join( versionString[-4:] )
    except ImportError:
      sys.stderr.write( 'No subprocess module, trying older os.popen().\n' )
      pipe = os.popen( 'bzr version-info  --custom --template=%s' % TEMPLATE )
      versionString = pipe.read().split('\n')
      pipe.close()
      versionString = '\n'.join( versionString[-4:] )
  versionString = versionString.replace('\n',r'\n')
  return versionString

START_MARKER = 'static string __bzr_revinfo("'
END_MARKER = '");'

def makeFile( srcDir, versionString ):
    templatePath = os.path.join( srcDir, 'arVersionTemplate.cpp' )
    outfilePath = os.path.join( srcDir, 'arVersion.cpp' )
    if os.path.exists( outfilePath ):
      outFile = file( outfilePath, 'r' )
      s = outFile.read()
      outFile.close()
      i1 = s.find( START_MARKER )
      if i1 > 0:
        i1 += len(START_MARKER)
        i2 = s.find( END_MARKER, i1 )
        stringThere = s[i1:i2]
        if stringThere == versionString:
          print 'Current bzr version info already present, leaving arVersion.cpp unchanged'
          return
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
    print '==========================================='
    makeFile( srcDir, versionString )
    print 'make_arVersion.py succeeded'
  except:
    print 'WARNING: FAILED TO INSERT VERSION INFO'
    print 'Are Python <www.python.org> and bazaar <bazaar-vcs.org> installed?'
    print '(non-fatal error)'
    traceback.print_exc( file=sys.stdout )
    sys.exit(1)
