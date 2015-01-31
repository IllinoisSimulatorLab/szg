import sys
import os
import traceback
from StringIO import StringIO
import traceback

print 'make_arVersion:',__file__

GIT_COMMAND = 'git log -1 --pretty=format:"%H"'


def getGitVersion():
    try:
        import subprocess
        versionString = subprocess.check_output( GIT_COMMAND, shell=True )
        print versionString
        print '============================================='
    except ImportError:
        sys.stderr.write( 'No subprocess module, trying older os.popen().\n' )
        pipe = os.popen( GIT_COMMAND )
        versionString = pipe.read().split('\n')
        pipe.close()
        versionString = versionString[0]
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
        versionString = getGitVersion()
        print 'version info:',versionString
        print '==========================================='
        makeFile( srcDir, versionString )
        print 'make_arVersion.py succeeded'
    except:
        print 'WARNING: FAILED TO INSERT VERSION INFO'
        print 'Are Python <www.python.org> and git <www.git-scm.com> installed?'
        print '(non-fatal error)'
        traceback.print_exc( file=sys.stdout )
        sys.exit(1)
