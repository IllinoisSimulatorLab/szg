#!/bin/sh
ssh-add $SZG_ROOT/szg-release/szg.private.key
cd $SZG_ROOT/szg
cvs update -Pd
cd $SZG_ROOT/vmat
cvs update -Pd
cd $SZG_ROOT/szgdemo
cvs update -Pd
cd $SZG_ROOT/parametric
cvs update -Pd
cd $SZG_ROOT/reality-peer
cvs update -Pd
cd $SZG_ROOT/bkitchen
cvs update -Pd
cd $SZG_ROOT/szgexpt
cvs update -Pd
cd $SZG_ROOT/szg_app_template
cvs update -Pd

