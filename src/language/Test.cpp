//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataTemplate.h"
#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arStructuredDataParser.h"
#include <sstream>
#include <iostream>

int main(){
  int i;
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

  cout << "The first piece of data should have size 72 bytes.\n";
  cout << "It should also have ID = 0 and have two fields, \"INT ID\" "
       << "and \"FLOAT ID\".\n";
  cout << "The integers should count from 0 to 20, skipping by 5.\n";
  cout << "The floats should count from 0 to 0.8, skipping by 0.2.\n";
  cout << "The integer in array position 1 should be 5 and is "
       << moreIntData[1] << "\n";
  cout << "The float in array position 2 should be 0.4 and is "
       << moreFloatData[2] << "\n";
  data1.dump(true);
  cout << "We can also print the data in XML format.\n";
  data1.print();

  cout << "*****************************************************\n";
  cout << "We now test file I/O. If all goes well, you should "
       << "see XML dumps of the two records defined above.\n";
  ar_timeval time1;
  arFileTextStream fileStream2;
  arStructuredDataParser theParser2(&dict1);
  FILE* testFile = fopen("Junk.txt","w");
  if (!testFile){
    cout << "failed to open test file for writing.\n";
    goto LNext;
  }
  data1.print(testFile);
  data2.print(testFile);
  fclose(testFile);
  testFile = fopen("Junk.txt","r");
  if (!testFile){
    cout << "failed to open test file for reading.\n";
  }
  else{
    arFileTextStream fileStream;
    fileStream.setSource(testFile);
    arStructuredDataParser theParser(&dict1);
    arStructuredData* fileData = theParser.parse(&fileStream);
    if (fileData){
      fileData->print();
      theParser.recycle(fileData);
    }
    else{
      cout << "Parser failed to deal with input file.\n";
    }
    
    fileData = theParser.parse(&fileStream);
    fclose(testFile);
    if (fileData){
      fileData->print();
      theParser.recycle(fileData);
    }
    else{
      cout << "Parser failed to deal with input file.\n";
    }
  }
  // now, we will test speed of reading and writing..
  testFile = fopen("Junk.txt","w");
  if (!testFile){
    cout << "failed to open test file for speed writing.\n";
    goto LNext;
  }
  float speedFloatBuffer[16];
  data1.dataIn(FLOAT_ID, speedFloatBuffer, AR_FLOAT, 16);
  time1 = ar_time();
  int s;
  for (s=0; s<10000; s++){
    data1.print(testFile);
  }
  cout << "^^^^^ It took " << ar_difftime(ar_time(), time1) 
       << " microseconds to write 10000 records to a file.\n";
  fclose(testFile);
  testFile = fopen("Junk.txt","r");
  if (!testFile){
    cout << "failed to open test file for speed reading.\n";
    goto LNext;
  }
  fileStream2.setSource(testFile);
  time1 = ar_time();
  for (s=0; s<10000; s++){
    arStructuredData* result = theParser2.parse(&fileStream2);
    if (!result){
      cout << "Error in reading 10000 records.\n";
      break;
    }
  }
  cout << "^^^^^ It took " << ar_difftime(ar_time(), time1)/1000000.0 
       << " seconds to read 10000 records from a file.\n";
  fclose(testFile);
LNext:
    
  cout << "*****************************************************\n";

  cout << "The next piece of data should have size 80 bytes.\n";
  cout << "It should also have ID = 1 and have two fields, \"CHAR ID\" "
       << "and \"DOUBLE ID\".\n";
  cout << "The chars should go from A to E.\n";
  cout << "The doubles should count from 0 to 2, skipping by 0.5.\n";
  cout << "The char in array position 0 should be A and is "
       << moreCharData[0] << "\n";
  cout << "The float in array position 3 should be 1.5 and is "
       << moreDoubleData[3] << "\n";
  data2.dump(true);
  cout << "We can also print the data in XML format.\n";
  data2.print();
  cout << "*****************************************************\n";
  char buffer[1000];
  data2.pack(buffer);
  arStructuredData data3(&t2);
  data3.unpack(buffer);
  cout << "We now  pack this piece of data into a buffer and extract it into\n"
       << "a new arStructuredData record from that buffer. The dump should\n"
       << "be identical.\n";
  data3.dump(true);
  
  cout << "The template dictionary should have size 216\n";
  dict1.dump();
  dict1.pack(buffer);
  dict2.unpack(buffer);
  cout << "The template dictionary was packed into a buffer and then\n"
       << "unpacked into a new structure. This should be identical.\n";
  dict2.dump();
  
  
  // conversion tests
  cout << "\nTests of conversions with error-checking.\n";
  cout << "Note that error messages do not necessarily indicate failure;\n"
      << "some of the conversions are intended to fail.  Only messages\n"
      << "of the form 'XXXX test failed' indicate failure.\n\n";
  long theLong;
  int theInt;
  if (!ar_stringToLongValid("55555",theLong))
   cout << "String->long test 1 failed.\n";
  if (theLong != 55555)
   cout << "String->long test 1 failed.\n";
  if (!ar_longToIntValid(5000,theInt))
   cout << "Long->int test 1 failed.\n";
  if (!ar_stringToLongValid("-2140000000",theLong))
   cout << "String->long test 2 failed.\n";
  if (theLong != -2140000000)
   cout << "String->long test 2 failed.\n";
  if (ar_stringToLongValid("10000000000000000",theLong))
   cout << "String->long test 3 failed.\n";
  double theDouble;
  float theFloat;
  if (ar_stringToDoubleValid("1.2e500",theDouble))
     cout << "String->double test 1 failed.\n";
  if (!ar_stringToDoubleValid("1.42678e125",theDouble))
     cout << "String->double test 2 failed.\n";
  if (theDouble != 1.42678e125)
     cout << "String->double test 2 failed.\n";
  if (ar_doubleToFloatValid(theDouble,theFloat))
    cout << "Double->float test 1 failed.\n";
  if (!ar_doubleToFloatValid(1.237e-10,theFloat))
     cout << "Double->float test 2 failed.\n";
  cout << "Conversion tests completed.\n";
  cout << "---------------------------\n";
  
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
  bool isFile;
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
