#!/bin/sh
cd ~
mv linux.tar.gz bin
cd bin
echo Removing Files...
rm -rf linux
echo Unpacking Files...
tar -xzf linux.tar.gz
rm linux.tar.gz
echo Done.

