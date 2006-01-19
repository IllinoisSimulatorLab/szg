#!/bin/sh
cd $SZG_ROOT/szg/build
make clean; make demo-clean; make demo
make install-shared
cd $SZG_ROOT/szg/python
make clean; make
cd $SZG_ROOT/vmat
make clean; make
cd $SZG_ROOT/szgdemo
make clean; make
make install-shared
cd $SZG_ROOT/parametric
make clean; make
cd $SZG_ROOT/reality-peer
make clean; make
cd $SZG_ROOT/bkitchen
make clean; make
cd $SZG_ROOT/szgexpt
make clean; make

