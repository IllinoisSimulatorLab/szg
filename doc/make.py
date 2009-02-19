import os
import sys
import subprocess

if __name__ == '__main__':
  docFiles = [f for f in os.listdir('txt2tags') if os.path.splitext(f)[1] == '.t2t']
  for f in docFiles:
    outFile = os.path.splitext(f)[0]+'.html'
    print 'Converting',outFile
    command = [sys.executable,'txt2tags.py','-t','html','-C','t2t.rc','-o',outFile,'txt2tags/'+f]
    subprocess.call( command )
