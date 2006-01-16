#!/bin/sh
if [ -f "$SZGHOME/build/make/Makefile.vars" ]; then
  echo Creating SDK...
  cd $SZGHOME
  # If there was already an sdk, go ahead and clean up.
  rm -rf szg-install
  cd $SZGHOME/build
  make create-sdk
  cd ../python
  make create-sdk
  # TODO: NEED AN INSTALL TARGET!!!!
  cd $SZGHOME
  mv szg-install szg-0.87
  tar -cf szg-0.87.tar ./szg-0.87
  gzip szg-0.87.tar
  rm -rf szg-0.87
else
  echo $SZGHOME must be set and an szg installation must exist!
fi