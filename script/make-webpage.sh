#!/bin/sh
if [ -d "$SZGHOME/webpage" ]; then
  rm -rf $SZGHOME/temp
  mkdir $SZGHOME/temp
  cp -r $SZGHOME/webpage $SZGHOME/temp
  cd $SZGHOME/build
  make docs
  cd $SZGHOME
  cp -r doc $SZGHOME/temp
  find $SZGHOME/temp -name 'CVS' -print | xargs rm -rf
  find $SZGHOME/temp -name '.cvsignore' -print | xargs rm -rf
  # A finder-related file on OS X.
  find $SZGHOME/temp -name '.DS_Store' -print | xargs rm -rf
  cd $SZGHOME/temp/doc
  rm Makefile
  rm -rf txt2tags*
  rm -rf notes
  cd ..
  mv doc webpage/szg
  tar -cf webpage.tar ./webpage
  mv webpage.tar ../
  cd ../
  # Clean-up
  rm -rf temp
  # Publish
  scp webpage.tar schaeffr@jack.isl.uiuc.edu:public_html
  ssh schaeffr@jack.isl.uiuc.edu chmod ga+r public_html/webpage.tar
  rm webpage.tar
else
  echo Error: SZGHOME is not defined. Cannot continue.
  exit 1
fi
