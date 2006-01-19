#!/bin/sh
#parameters: version 
ssh-agent $SZG_ROOT/szg-release/make-release-update.sh
$SZG_ROOT/szg-release/make-release-build.sh
$SZG_ROOT/szg-release/make-release.sh darwin $1
