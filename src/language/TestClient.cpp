//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataClient.h"
#include "arStructuredData.h"
#include "arDataTemplate.h"
#include "arTemplateDictionary.h"
#include "arDataUtilities.h"
#include "arSocketTextStream.h"
#include "arStructuredDataParser.h"
#include "arXMLUtilities.h"
#include <math.h>
#include <iostream>
using namespace std;

int CHAR_DATA_ID;
int INT_DATA_ID;
int FLOAT_DATA_ID;
int DOUBLE_DATA_ID;

bool dataCheck(arStructuredData* data, string identifier){
  char* charPtr = (char*) data->getDataPtr(CHAR_DATA_ID, AR_CHAR);
  if (!charPtr){
    cout << "  Test failed (" << identifier << "). char data not found.\n";
    return false;
  }
  int i;
  for (i=0; i<10; i++){
    if (charPtr[i] != 'A' + i){
      cout << "  Test failed (" << identifier << "). Reading char " 
           << i << ".\n";
      return false;
    }
  }
  int* intPtr = (int*) data->getDataPtr(INT_DATA_ID, AR_INT);
  if (!intPtr){
    cout << "  Test failed (" << identifier << "). int data not found.\n";
    return false;
  }
  for (i=0; i<10; i++){
    if (intPtr[i] != i*2){
      cout << "  Test failed (" << identifier << "). Reading int " << i 
           << ".\n";
      return false;
    }
  }
  float* floatPtr = (float*) data->getDataPtr(FLOAT_DATA_ID, AR_FLOAT);
  if (!floatPtr){
    cout << "  Test failed (" << identifier << "). float data not found.\n";
    return false;
  }
  for (i=0; i<10; i++){
    if (fabs(floatPtr[i] - i/10.0) > 0.0001){
      cout << "  Test failed (" << identifier << "). Reading float " << i 
           << ".\n";
      return false;
    }
  }
  double* doublePtr = (double*) data->getDataPtr(DOUBLE_DATA_ID, 
                                                     AR_DOUBLE);
  if (!doublePtr){
    cout << "  Test failed (" << identifier << "). double data not found.\n";
    return false;
  }
  for (i=0; i<10; i++){
    if (fabs(doublePtr[i] - i/2.0) > 0.0001){
      cout << "  Test failed (" << identifier << "). Reading double " << i 
           << ".\n";
      return false;
    }
  }
  return true;
}

int main(int argc, char** argv){
  if (argc<2){
    cerr << "usage: " << argv[0] << " IP_address_of_data_server\n";
    return 1;
  }

  arDataClient client(argv[0]);
  // OK, I was making an attempt to deal with low throughput on WANs
  // via the socket options... changing the buffer size here definitely
  // makes a difference, though I never saw anything better than about
  // 12 Mbps between LA and UIUC
  //client.setBufferSize(200000);
  cout << "Test 1: Checking initial connection process.\n";
  if (!client.dialUp(argv[1],3000)){
    cout << "  Test failed. Could not connect. Is TestLanguageServer "
	 << "running?\n";
    return 1;
  }
  arTemplateDictionary* theDictionary = client.getDictionary();
  
  arDataTemplate* dataTemplate = theDictionary->find("Test_Data");
  if (!dataTemplate){
    cout << "  Test failed. Did not receive template.\n";
    return 1;
  }
  if (dataTemplate->getAttributeID("Int_Data") != 0){
    cout << "  Test failed. Wrong attribute ID.\n";
    return 1;
  }
  if (dataTemplate->getAttributeID("Char_Data") != 1){
    cout << "  Test failed. Wrong attribute ID.\n";
    return 1;
  }
  if (dataTemplate->getAttributeID("Float_Data") != 2){
    cout << "  Test failed. Wrong attribute ID.\n";
    return 1;
  }
  if (dataTemplate->getAttributeID("Double_Data") != 3){
    cout << "  Test failed. Wrong attribute ID.\n";
    return 1;
  }
  cout << "  PASSED.\n";

  arDataTemplate* testDataTemplate = theDictionary->find("Test_Data");
  CHAR_DATA_ID = testDataTemplate->getAttributeID("Char_Data");
  INT_DATA_ID = testDataTemplate->getAttributeID("Int_Data");
  FLOAT_DATA_ID = testDataTemplate->getAttributeID("Float_Data");
  DOUBLE_DATA_ID = testDataTemplate->getAttributeID("Double_Data");

  arStructuredData testData1(testDataTemplate);
  arStructuredData testData2(testDataTemplate);
  arStructuredData testData3(testDataTemplate);

  ar_timeval init_time, done_time;
  ARint charSpaceSize = 20000000;
  char* charSpace = new char[charSpaceSize];
  char* anotherBuffer = new char[2000000];
  int* intSpace = new int[10];
  float* floatSpace = new float[10];
  double* doubleSpace = new double[10];

  cout << "Test 2: Determine TCP link speed.";
  
  // first, we receive some data over the TCP socket... no formating...
  // to determine the limiting data rate. At this point we get a ballpark
  // of the connection speed, 10,100,1000 Mbps
  arSocket* comm = client.getSocket();
  init_time = ar_time();
  comm->ar_safeRead(charSpace,200000);
  done_time = ar_time();
  float guessMbps = 1600000.0/ar_difftime(done_time,init_time);
  int numberSentChars;
  if (guessMbps < 20){
    cout << " Guessing this is a 10 Mbps link\n";
    guessMbps = 10;
    charSpace[0] = 0;
    numberSentChars = 200000;
  }
  else if (guessMbps < 120){
    cout << " Guessing this is a 100 Mbps link\n";
    guessMbps = 100;
    charSpace[0] = 1;
    numberSentChars = 2000000;
  }
  else{
    cout << " Guessing this is a 1000 Mbps link\n";
    guessMbps = 1000;
    charSpace[0] = 2;
    numberSentChars = 20000000;
  }
  comm->ar_safeWrite(charSpace,1);
  init_time = ar_time();
  comm->ar_safeRead(charSpace,numberSentChars);
  done_time = ar_time();
  cout << "  PASSED. TCP Link speed = " 
       << (8.0*numberSentChars)/ar_difftime(done_time,init_time)
       << " Mbps\n";

  // next, we receive 10 structured data packets, unpack them into 
  // storage, and copy that storage into memory

  cout << "Test 2: Determine arStructuredData link speed (large records).\n";
  init_time = ar_time();
  int i;
  for (i=0; i<10; i++){
    if (!client.getData(charSpace,charSpaceSize)){
      cout << "Test failed. Did not receive data record " << i << ".\n";
      return 1;
    }
    if (!testData1.unpack(charSpace)){
      cout << "  Test failed. Did not get data record " << i << ".\n";
      return 1;
    }
    testData1.dataOut(CHAR_DATA_ID,anotherBuffer,AR_CHAR,numberSentChars/10);
    testData1.dataOut(INT_DATA_ID,intSpace,AR_INT,10);
    testData1.dataOut(FLOAT_DATA_ID,floatSpace,AR_FLOAT,10);
    testData1.dataOut(DOUBLE_DATA_ID,doubleSpace,AR_DOUBLE,10);
  } 
  done_time = ar_time();
  cout << "  PASSED: 10 records received, each of size "
       << testData1.size() << ", with speed " 
       << (80.0*testData1.size())/ar_difftime(done_time,init_time)
       << " Mbps\n";

  cout << "Test 3: Determine arStructuredData link speed (small records).\n";
  init_time = ar_time();
  for (i=0; i<10000; i++){
    if (!client.getData(charSpace,charSpaceSize)){
      cout << "Test failed. Did not receive data record " << i << ".\n";
      return 1;
    }
    if (!testData1.unpack(charSpace)){
      cout << "  Test failed. Did not get data record " << i << ".\n";
      return 1;
    }
    testData1.dataOut(CHAR_DATA_ID,anotherBuffer,AR_CHAR,
                      numberSentChars/10000);
    testData1.dataOut(INT_DATA_ID,intSpace,AR_INT,10);
    testData1.dataOut(FLOAT_DATA_ID,floatSpace,AR_FLOAT,10);
    testData1.dataOut(DOUBLE_DATA_ID,doubleSpace,AR_DOUBLE,10);
  } 
  done_time = ar_time();
  cout << "  PASSED: 10000 records received, each of size "
       << testData1.size() << ", with speed "
       << (80000.0*testData1.size())/ar_difftime(done_time,init_time)
       << " Mbps\n";
  
  // Next, we receive a single structured data packet. This tests
  // network translation capabilities between machines w/ different endianess.
  cout << "Test 4. Make sure that data is correctly translated and "
       << "unpacked.\n";
  client.getData(charSpace,charSpaceSize);
  // Copy data from buffer into local storage.
  testData2.unpack(charSpace);
  if (!dataCheck(&testData2, "1")){
    return 1;
  }
  // Set internal pointers towards storage in the buffer. Do not copy data
  // in.
  testData3.parse(charSpace); 
  if (!dataCheck(&testData3,"2")){
    return 1;
  }
  cout << "  PASSED.\n";

  // Finally, we go ahead and receive a text version of the data record.
  cout << "Test 5: Determine if XML records are received correctly.\n";
  arStructuredDataParser theParser(theDictionary);
  arSocketTextStream socketStream;
  socketStream.setSource(comm);
  arStructuredData* result = theParser.parse(&socketStream);
  if (!result){
    cout << "  Test failed. Could not parse first XML record.\n";
    return 1;
  }
  else{
    if (!dataCheck(result, "1")){
      return 1;
    }
  }
  // Do it again...
  result = theParser.parse(&socketStream);
  if (!result){
    cout << "  Test failed. Could not parse second XML record.\n";
    return 1;
  }
  else{
    if (!dataCheck(result, "2")){
      return 1;
    }
  }
  // Sometimes, we need to be able to put various tags into the
  // stream of text XML records. We test that functionality now.
  string nextTag = ar_getTagText(&socketStream);
  if (nextTag != "extra_tag"){
    cout << "  Test failed. Did not get out-of-band tag.\n";
    return 1;
  }
  // Read one more record...
  result = theParser.parse(&socketStream);
  if (!result){
    cout << "  Test Failed. Could not parse third XML record.\n";
    return 1;
  }
  else{
    if (!dataCheck(result, "3")){
      return 1;
    }
  }
  // Now, we'll read in the tag and then parse the rest of the record.
  nextTag = ar_getTagText(&socketStream);
  result = theParser.parse(&socketStream, nextTag);
  if (!result){
    cout << "  Test failed. Could not parse fourth XML record.\n";
    return 1;
  }
  else{
    if (!dataCheck(result, "4")){
      return 1;
    }
  }
  cout << "  PASSED.\n";
  
  
  cout << "Test 6: Determine speed of XML record sending.\n";
  ar_timeval time1 = ar_time();
  for (int j=0; j<1000; j++){
    result = theParser.parse(&socketStream);
    if (!result){
      cout << "  Test failed. Could not receive record " << i << ".\n";
      return 1;
    }
  }
  cout << "  PASSED. XML send rate = " 
       << 1000000000.0/ar_difftime(ar_time(), time1) << " per second.\n";
  client.closeConnection();
}
