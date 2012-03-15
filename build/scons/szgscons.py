import os

def makeLibName( libName ):
  libName = 'ar'+libName.capitalize()
  if os.environ.get( 'SZG_LINKING', 'STATIC' ) != 'DYNAMIC':
    libName += '_static'
  return libName
    

