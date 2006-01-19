#!/bin/sh
ssh-add $SZG_ROOT/szg-release/szg.private.key
cd $SZG_ROOT/szg-release
cvs checkout szg
cd szg
cvs update -Pd
find . -name 'CVS' -print | xargs rm -rf
cd ../
mv szg szg-$1
tar -cf szg-$1.tar ./szg-$1
gzip szg-$1.tar
rm -rf szg-$1

cvs checkout vmat
cd  vmat
cvs update -Pd
find . -name 'CVS' -print | xargs rm -rf
cd ../
mv vmat vmat-$1
tar -cf vmat-$1.tar ./vmat-$1
gzip vmat-$1.tar
rm -rf vmat-$1

cvs checkout szgdemo
cd szgdemo
cvs update -Pd
find . -name 'CVS' -print | xargs rm -rf
cd VRMLView
rm -rf data
cd ../bigpicture
rm -rf data
cd ../cubecake
rm -rf data
cd ../danceparty
rm -rf data
cd ../info-forest
rm -rf data
cd ../internet2-02
rm -rf data
cd ../landspeeder
rm -rf landspeederData
cd ../salamiman
rm -rf data
cd ../skyfly
rm -rf images
cd ../vtkfile
rm -rf data
cd ../warpeace
rm -rf data
cd ../../
mv szgdemo szgdemo-$1
tar -cf szgdemo-$1.tar ./szgdemo-$1
gzip szgdemo-$1.tar
rm -rf szgdemo-$1

cvs checkout parametric
cd parametric
cvs update -Pd
find . -name 'CVS' -print | xargs rm -rf
cd ../
mv parametric parametric-$1
tar -cf parametric-$1.tar ./parametric-$1
gzip parametric-$1.tar
rm -rf parametric-$1

cvs checkout reality-peer
cd reality-peer
cvs update -Pd
find . -name 'CVS' -print | xargs rm -rf
cd ../
mv reality-peer reality-peer-$1
tar -cf reality-peer-$1.tar ./reality-peer-$1
gzip reality-peer-$1.tar
rm -rf reality-peer-$1

cvs checkout bkitchen
cd bkitchen
cvs update -Pd
find . -name 'CVS' -print | xargs rm -rf
cd ../
mv bkitchen bkitchen-$1
tar -cf bkitchen-$1.tar ./bkitchen-$1
gzip bkitchen-$1.tar
rm -rf bkitchen-$1

cvs checkout szgexpt
cd szgexpt
cvs update -Pd
find . -name 'CVS' -print | xargs rm -rf
cd ../
mv szgexpt szgexpt-$1
tar -cf szgexpt-$1.tar ./szgexpt-$1
gzip szgexpt-$1.tar
rm -rf szgexpt-$1

cvs checkout szg_app_template
cd szg_app_template
cvs update -Pd
find . -name 'CVS' -print | xargs rm -rf
cd ../
mv szg_app_template szg_app_template-$1
tar -cf szg_app_template-$1.tar ./szg_app_template-$1
gzip szg_app_template-$1.tar
rm -rf szg_app_template-$1

