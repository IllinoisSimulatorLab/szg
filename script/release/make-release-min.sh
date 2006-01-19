#!/bin/sh
#Parameters: platform version local/nonlocal
cd $SZGHOME/build
make create-sdk
cd ../python
make create-sdk
cd $SZGHOME
mv szg-install szg-$2
if [ "$1" = "win32-vc6" ]; then
  echo "COPYING DLLS"
  cp $SZG_ROOT/szg-release/VisualStudio/* szg-$2/bin
fi
if [ "$1" = "win32-vc7" ]; then
  echo "COPYING DLLS"
  cp $SZG_ROOT/szg-release/VisualStudio/* szg-$2/bin
fi
echo "CREATING MINIMAL SDK TARBALL"
tar -cf szg-$2-$1.tar ./szg-$2
gzip szg-$2-$1.tar
mv szg-$2-$1.tar.gz $SZG_ROOT/szg-release
# Must move this back. Otherwise, the additional copying in will not work.
# (as occurs in make-release.sh)
mv szg-$2 szg-install
if [ "$3" != "local" ]; then
  rm -rf szg-install
fi

