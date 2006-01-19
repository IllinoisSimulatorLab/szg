#!/bin/sh
#parameters: version 
$SZG_ROOT/szg-release/make-release-build.sh
$SZG_ROOT/szg-release/make-release.sh win32-vc7 $1
