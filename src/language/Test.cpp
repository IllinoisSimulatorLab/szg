//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arDataTemplate.h"
#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arStructuredDataParser.h"

#include <math.h>
#include <sstream>
#include <iostream>

#ifndef AR_USE_WIN_32
  #include <unistd.h>
#else
  #define unlink(x) ((void)0) // lazy
#endif

bool keepStringTestRunning = true;

void hammerString1(void*){
  cout << "XXX.\n";
  while (keepStringTestRunning){
    string temp("yyy_test_name");
  }
}

void hammerString2(void*){
  cout << "YYY.\n";
  while (keepStringTestRunning){
    string temp("xxx_test_name");
  }
}

int main(){
  cout << "\n";
  cout << "PLEASE NOTE: $SZGHOME must be set to the top level of your szg\n"
       << "installation for these tests to succeed.\n\n";
  cout << "Beginning unit test 1: basic arStructuredData.\n";
  // Check that it is possible to delete a template with an attribute added.
  // (This was a source of a perplexing error once upon a time when moving
  // to dll's on Win32). NOTE: A little web searching indicates that this
  // is essentially a *thread-safety* issue. So better leave the test here.
  arDataTemplate* t3 = new arDataTemplate("foo");
  cout << "AARGH!\n";
  t3->add("bar", AR_INT);
  delete t3;

  int i = -1;
  arTemplateDictionary dict1, dict2;
  arDataTemplate t1("nice test");
  arDataTemplate t2("test 2");

  int INT_ID = t1.add("INT ID",AR_INT);
  int FLOAT_ID = t1.add("FLOAT ID",AR_FLOAT);
  int CHAR_ID = t2.add("CHAR ID",AR_CHAR);
  int DOUBLE_ID = t2.add("DOUBLE ID",AR_DOUBLE);

  dict1.add(&t1);
  dict1.add(&t2);

  arStructuredData data1(&t1);
  arStructuredData data2(&t2);
  
  // We test packing/unpacking of various types of fields.
  ARint junkIntData[5];
  ARint moreIntData[5];
  for (i=0; i<5; i++)
    junkIntData[i] = 5*i;
  (void)data1.dataIn("INT ID",(void*)junkIntData,AR_INT,5);
  data1.dataOut(INT_ID,(void*)moreIntData,AR_INT,5);

  ARfloat junkFloatData[5];
  ARfloat moreFloatData[5];
  for (i=0; i<5; i++)
    junkFloatData[i] = i/5.0;
  (void)data1.dataIn(FLOAT_ID,(void*)junkFloatData,AR_FLOAT,5);
  data1.dataOut("FLOAT ID",(void*)moreFloatData,AR_FLOAT,5);

  ARchar moreCharData[5];
  (void)data2.dataInString(CHAR_ID,"AB DE");
  data2.dataOut("CHAR ID",(void*)moreCharData,AR_CHAR,5);

  ARdouble junkDoubleData[5];
  ARdouble moreDoubleData[5];
  for (i=0; i<5; i++){
    junkDoubleData[i] = i/2.0;
  }
  (void)data2.dataIn("DOUBLE ID",(void*)junkDoubleData,AR_DOUBLE,5);
  data2.dataOut(DOUBLE_ID,(void*)moreDoubleData,AR_DOUBLE,5);

  // Does the data have the right size?
  if (data1.size() != 72){
    cout << "*** Test failed because of wrong data size.\n";
    exit(0);
  }
  // Have we received the right data upon unpacking?
  for (i=0; i<5; i++){
    if (moreIntData[i] != 5*i){
      cout << "*** Test failed because of wrong integer field unpacking.\n";
      exit(0);
    }
  }
  for (i=0; i<5; i++){
    // NOTE: floats will not be EXACTLY the same!
    if (fabs(moreFloatData[i]-i/5.0) > 0.0001){
      cout << "*** Test failed because of wrong float field unpacking.\n";
      exit(0);
    }
  }
  // Check the second piece of data.
  if (data2.size() != 80){
    cout << "*** Test failed because of wrong data size (2).\n";
    exit(0);
  }
  // have we received the right data upon unpacking?
  char* testCharPtr = (char*) data2.getDataPtr(CHAR_ID, AR_CHAR);
   if (testCharPtr[0] != 'A'){
     cout << "*** test failed (wrong char field unpacking, "
	  << "record 2).\n";
     exit(0);
   }
   if (testCharPtr[1] != 'B'){
     cout << "*** test failed (wrong char field unpacking, "
	  << "record 2).\n";
     exit(0);
   }
   if (testCharPtr[2] != ' '){
     cout << "*** test failed (wrong char field unpacking, "
	  << "record 2).\n";
     exit(0);
   }
   if (testCharPtr[3] != 'D'){
      cout << "*** test failed (wrong char field unpacking, "
	   << "record 2).\n";
      exit(0);
   }
   if (testCharPtr[4] != 'E'){
     cout << "*** test failed (wrong char field unpacking, "
	  << "record 2).\n";
     exit(0);
   }
   for (i=0; i<5; i++){
     // NOTE: doubles will not be the same!
     if (fabs(((double*)data2.getDataPtr(DOUBLE_ID, AR_DOUBLE))[i]
	      -i/2.0) > 0.0001){
       cout << "*** test failed (wrong double field unpacking, "
	    << "record 2).\n";
       exit(0);
     }
   }

  char buffer[1000];
  // Testing packing/unpacking into a buffer.
  data2.pack(buffer);
  arStructuredData data3(&t2);
  data3.unpack(buffer);
  // See if the data has the right size AND if it has the right double data.
  if (data3.size() != 80){
    cout << "*** Test failed because of wrong unpack data size.\n";
    exit(0);
  }
  for (i=0; i<5; i++){
     // NOTE: doubles will not be the same!
     if (fabs(((double*)data3.getDataPtr(DOUBLE_ID, AR_DOUBLE))[i]
	      -i/2.0) > 0.0001){
       cout << "*** test failed (wrong double field unpacking, "
	    << "record 2).\n";
       exit(0);
     }
   }
  
  // Check the size of the template dictionary.
  if (dict1.size() != 216){
    cout << "*** Test failed (template dictionary has wrong size).\n";
    exit(0);
  }
  dict1.pack(buffer);
  dict2.unpack(buffer);
  // See if the unpacked dictionary has the right size.
  if (dict2.size() != 216){
    cout << "*** Test failed (unpacked dictionary has the wrong size).\n";
    exit(0);
  }
  
  cout << "*** PASSED\n";

  cout << "Beginning unit test 2: Basic file I/O.\n";
  ar_timeval time1;
  arFileTextStream fileStream2;
  arStructuredDataParser theParser2(&dict1);
  const char* filename = "Junk.txt";
  (void)unlink(filename);
  FILE* testFile = fopen(filename,"w");
  if (!testFile){
    cout << "*** Test failed (could not open temp file for writing).\n";
    exit(0);
  }
  data1.print(testFile);
  data2.print(testFile);
  fclose(testFile);
  testFile = fopen(filename,"r");
  if (!testFile){
    cout << "*** Test failed (could not open temp file for reading).\n";
    exit(0);
  }
  else{
    arFileTextStream fileStream;
    fileStream.setSource(testFile);
    arStructuredDataParser theParser(&dict1);
    arStructuredData* fileData = theParser.parse(&fileStream);
    if (fileData){
      // check to see that the data is correct.
      if (fileData->getID() != t1.getID()){
	cout << "*** test failed (wrong ID on first record from file).\n";
	exit(0);
      }
      if (fileData->getDataDimension(INT_ID) != 5){
	cout << "*** test failed (wrong data dimension on first record "
	     << "(int field) from file).\n";
	exit(0);
      }
      for (i=0; i<5; i++){
        if (((int*)fileData->getDataPtr(INT_ID, AR_INT))[i] != 5*i){
	  cout << "*** test failed (wrong integer field unpacking, "
	       << "record 1).\n";
	  exit(0);
	}
      }
      if (fileData->getDataDimension(FLOAT_ID) != 5){
	cout << "*** test failed (wrong data dimension on first record "
	     << "(float field) from file).\n";
	exit(0);
      }
      for (i=0; i<5; i++){
	// NOTE: floats will not be the same!
        if (fabs(((float*)fileData->getDataPtr(FLOAT_ID, AR_FLOAT))[i]-i/5.0)
            > 0.0001){
	  cout << "*** test failed (wrong float field unpacking, "
	       << "record 1).\n";
	  exit(0);
	}
      }
      theParser.recycle(fileData);
    }
    else{
      cout << "*** Test failed (could not read first record from file).\n";
      exit(0);
    }
    
    fileData = theParser.parse(&fileStream);
    fclose(testFile);
    if (fileData){
      // check to see that the data is correct.
      if (fileData->getID() != t2.getID()){
	cout << "*** test failed (wrong ID on second record from file).\n";
	exit(0);
      }
      if (fileData->getDataDimension(CHAR_ID) != 5){
	cout << "*** test failed (wrong data dimension on second record "
	     << "(char field) from file).\n";
	exit(0);
      }
      testCharPtr = (char*)fileData->getDataPtr(CHAR_ID, AR_CHAR);
      if (testCharPtr[0] != 'A'){
	cout << "*** test failed (wrong char field unpacking, "
	     << "record 2).\n";
	exit(0);
      }
      if (testCharPtr[1] != 'B'){
	cout << "*** test failed (wrong char field unpacking, "
	     << "record 2).\n";
	exit(0);
      }
      if (testCharPtr[2] != ' '){
	cout << "*** test failed (wrong char field unpacking, "
	     << "record 2).\n";
	exit(0);
      }
      if (testCharPtr[3] != 'D'){
        cout << "*** test failed (wrong char field unpacking, "
	     << "record 2).\n";
	exit(0);
      }
      if (testCharPtr[4] != 'E'){
	cout << "*** test failed (wrong char field unpacking, "
	     << "record 2).\n";
        exit(0);
      }
      if (fileData->getDataDimension(DOUBLE_ID) != 5){
	cout << "*** test failed (wrong data dimension on second record "
	     << "(char field) from file).\n";
	exit(0);
      }
      for (i=0; i<5; i++){
	// NOTE: doubles will not be the same!
        if (fabs(((double*)fileData->getDataPtr(DOUBLE_ID, AR_DOUBLE))[i]
		 -i/2.0) > 0.0001){
	  cout << "*** test failed (wrong double field unpacking, "
	       << "record 2).\n";
	  exit(0);
	}
      }
      theParser.recycle(fileData);
    }
    else{
      cout << "*** Test failed (could not read second record from file).\n";
      exit(0);
    }
  }
  cout << "*** PASSED.\n";
  
  cout << "Beginning unit test 3: conversion functions.\n";

  // conversion tests
  long theLong = -1;
  int theInt = -1;
  if (!ar_stringToLongValid("55555",theLong)){
   cout << "String->long test 1 failed.\n";
   exit(0);
  }
  if (theLong != 55555){
   cout << "String->long test 1 failed.\n";
   exit(0);
  }
  if (!ar_longToIntValid(5000,theInt)){
   cout << "Long->int test 1 failed.\n";
   exit(0);
  }
  if (!ar_stringToLongValid("-2140000000",theLong)){
   cout << "String->long test 2 failed.\n";
   exit(0);
  }
  if (theLong != -2140000000){
   cout << "String->long test 2 failed.\n";
   exit(0);
  }
  if (ar_stringToLongValid("10000000000000000",theLong)){
   cout << "String->long test 3 failed.\n";
   exit(0);
  }
  double theDouble = 0.;
  float theFloat = 0.;
  if (ar_stringToDoubleValid("1.2e500",theDouble)){
     cout << "String->double test 1 failed.\n";
     exit(0);
  }
  if (!ar_stringToDoubleValid("1.42678e125",theDouble)){
     cout << "String->double test 2 failed.\n";
     exit(0);
  }
  if (theDouble != 1.42678e125){
     cout << "String->double test 2 failed.\n";
     exit(0);
  }
  if (ar_doubleToFloatValid(theDouble,theFloat)){
    cout << "Double->float test 1 failed.\n";
    exit(0);
  }
  if (!ar_doubleToFloatValid(1.237e-10,theFloat)){
     cout << "Double->float test 2 failed.\n";
     exit(0);
  }
  cout << "*** PASSED.\n";

  cout << "Beginning unit test 4: XML file I/O.\n";
  // now, we will test speed of reading and writing..
  testFile = fopen(filename,"w");
  if (!testFile){
    cout << "*** Test failed (could not open file).\n";
    exit(0);
  }

  // Stlport may hang when reading back
  // bogus float data, so initialize speedFloatBuffer.
  float speedFloatBuffer[16];
  for (i=0; i<16; i++)
    speedFloatBuffer[i] = i;

  data1.dataIn(FLOAT_ID, speedFloatBuffer, AR_FLOAT, 16);
  time1 = ar_time();
  int s = 0;
  for (s=0; s<10000; s++){
    data1.print(testFile);
  }
  cout << "^^^^^ It took " << ar_difftime(ar_time(), time1)/1000000.0 
       << " seconds to write 10000 records to a file.\n";
  fclose(testFile);
  testFile = fopen(filename,"r");
  if (!testFile){
    cout << "*** Test failed (could not open file for reading)..\n";
    exit(0);
  }
  fileStream2.setSource(testFile);
  time1 = ar_time();
  for (s=0; s<10000; s++){
    arStructuredData* result = theParser2.parse(&fileStream2);
    if (!result){
      cout << "*** Test failed (could not read all records, stopped "
	   << "at " << s << ").\n";
      exit(0);
    }
  }
  cout << "^^^^^ It took " << ar_difftime(ar_time(), time1)/1000000.0 
       << " seconds to read 10000 records from a file.\n";
  fclose(testFile);
  (void)unlink(filename);
  cout << "*** PASSED.\n";
  exit(0);
  
  cout << "Beginning unit test 4: Thread safety of platform's lib c++.\n";
  // THIS ISN'T NEARLY LONG ENOUGH!
  arThread thread1(hammerString1);
  arThread thread2(hammerString2);
  time1 = ar_time();
  while (ar_difftime(ar_time(), time1)/1000000.0 < 10){
    ar_usleep(10000);
  }
  keepStringTestRunning = false;
  cout << "*** PASSED.\n";
  
  // OK... SEVERAL MORE CONFIDENCE TESTS ARE REQUIRED.
  // 1. Convert the below into a real confidence test (use $SZGHOME to
  //    get known directories).
  // 2. Make s *binary* reading/writing file test. This will highlight
  //    the speed differences between XML and binary.
  // 3. Make a arStructuredDataParser threading stress/memory leak test that
  //    pounds the CPU for a few minutes.
  // 4. The lib c++ thread-safety test really needs to run for MUCH longer.
  
  cout << "\nTest of file/directory checks.\n\n";
  string upDir("..");
  ar_pathAddSlash(upDir);
  bool itExists = false, isDirectory = false;
  if (!ar_directoryExists( upDir + "language", itExists, isDirectory ))
    cout << "ar_directoryExists() test 1 failed.\n";
  else {
    if (!itExists)
      cout << "Item " << (upDir + "language") << " does not exist (bad).\n";
    else {
      cout << "Item " << (upDir + "language") << " exists (good)";
      if (isDirectory)
        cout << " and is a directory (good).\n";
      else
        cout << ", but is not a directory (bad).\n";
    }
  }
  if (!ar_directoryExists( upDir + "bloobl", itExists, isDirectory ))
    cout << "ar_directoryExists() test 2 failed.\n";
  else {
    if (!itExists)
      cout << "Item " << (upDir + "bloobl") << " does not exist (good).\n";
    else {
      cout << "Item " << (upDir + "bloobl") << " exists (bad)";
      if (isDirectory)
        cout << " and is a directory (bad).\n";
      else
        cout << ", but is not a directory (bad).\n";
    }
  }
  bool isFile = false;
  if (!ar_fileExists( "Makefile", itExists, isFile ))
    cout << "ar_fileExists() test 1 failed.\n";
  else {
    if (!itExists)
      cout << "Item Makefile does not exist (bad).\n";
    else {
      cout << "Item Makefile exists (good)";
      if (isFile)
        cout << " and is a file (good).\n";
      else
        cout << ", but is not a file (bad).\n";
    }
  }
  if (!ar_fileExists( "yubble.blt", itExists, isFile ))
    cout << "ar_fileExists() test 2 failed.\n";
  else {
    if (!itExists)
      cout << "Item yubble.blt does not exist (good).\n";
    else {
      cout << "Item yubble.blt exists (bad)";
      if (isFile)
        cout << " and is a file (bad).\n";
      else
        cout << ", but is not a file (bad).\n";
    }
  }
  string dirString;
  if (!ar_getWorkingDirectory(dirString))
    cout << "\nFailed to get working directory.n";
  else
    cout << "\nWorking directory is " << dirString << endl;
  if (!ar_setWorkingDirectory(".."))
    cout << "Failed to change working directory.\n";
  else
    cout << "Changed working directory.\n";
  if (!ar_getWorkingDirectory(dirString))
    cout << "Failed to get working directory.n";
  else
    cout << "Working directory is " << dirString << endl;
  cout << "--------------------------------\n";
  
  cout << "\nCurrent date and time are: " << ar_currentTimeString() << endl;
  cout << "--------------------------------\n";
}
