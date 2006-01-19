#!/bin/sh
#parameters: platform version
./make-release-min.sh $1 $2 local
cd $SZGHOME/build
make demo-create-sdk
cd $SZG_ROOT/vmat
make create-sdk
cd $SZG_ROOT/szgdemo
make create-sdk
cd $SZG_ROOT/reality-peer
make create-sdk
cd $SZG_ROOT/parametric
make create-sdk
cd $SZGHOME
mv szg-install szg-$2
echo "CREATING FULL SDK TARBALL"
tar -cf szg-$2-$1-all.tar ./szg-$2
gzip szg-$2-$1-all.tar
rm -rf szg-$2
mv szg-$2-$1-all.tar.gz $SZG_ROOT/szg-release
