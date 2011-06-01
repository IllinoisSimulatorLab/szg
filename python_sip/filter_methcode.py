# 
# Filter program for use with doxygen. Removes everything in a .sip
# file between lines starting with '%MethodCode' and '%End', because
# doxygen interprets those as symbols.
#
# In the Doxyfile, set INPUT_FILTER = "python filter_methcode.py"
# (with quotes).
#
import sys

def main():
  inMeth = False
  f = open( sys.argv[1], 'r' )
  for line in f:
    if not inMeth:
      if line.find('%MethodCode') == 0:
        inMeth = True
    if not inMeth:
      sys.stdout.write( line )
    if inMeth:
      if line.find('%End') == 0:
        inMeth = False
  f.close()

if __name__ == '__main__':
  if len( sys.argv ) != 2:
    print 'Usage: python filter_methcode.py <filename>'
  sys.stderr.write( sys.argv[1]+'\n' )
  main()
