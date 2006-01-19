#!/bin/sh
#parameters: version 
$SZG_ROOT/make-clean.sh
ssh-agent $SZG_ROOT/szg-release/make-release-update.sh
$SZG_ROOT/szg-release/make-release-build.sh
ssh-agent $SZG_ROOT/szg-release/make-release-source.sh $1
$SZG_ROOT/szg-release/make-release.sh linux $1
