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
#include <iostream>
using namespace std;

int main(int argc, char** argv){
  if (argc<2){
    cerr << "usage: " << argv[0] << " IP_address_of_data_server\n";
    return 1;
  }

  arDataClient szgClient(argv[0]);
  // OK, I was making an attempt to deal with low throughput on WANs
  // via the socket options... changing the buffer size here definitely
  // makes a difference, though I never saw anything better than about
  // 12 Mbps between LA and UIUC
  //szgClient.setBufferSize(200000);
  szgClient.dialUp(argv[1],3000);
  arTemplateDictionary* theDictionary = szgClient.getDictionary();
  cout << "****** This is the dictionary decoded from the wire.\n";
  theDictionary->dump();

  arDataTemplate* testDataTemplate = theDictionary->find("Test Data");
  int CHAR_DATA_ID = testDataTemplate->getAttributeID("Char Data");
  int INT_DATA_ID = testDataTemplate->getAttributeID("Int Data");
  int FLOAT_DATA_ID = testDataTemplate->getAttributeID("Float Data");
  int DOUBLE_DATA_ID = testDataTemplate->getAttributeID("Double Data");

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

  cout << "****** Waiting for test stream of data\n";
  
  // first, we receive some data over the TCP socket... no formating...
  // to determine the limiting data rate. At this point we get a ballpark
  // of the connection speed, 10,100,1000 Mbps
  arSocket* comm = szgClient.getSocket();
  init_time = ar_time();
  comm->ar_safeRead(charSpace,200000);
  done_time = ar_time();
  float guessMbps = 1600000.0/ar_difftime(done_time,init_time);
  int numberSentChars;
  if (guessMbps < 20){
    cout << "Guessing this is a 10 Mbps link\n";
    guessMbps = 10;
    charSpace[0] = 0;
    numberSentChars = 200000;
  }
  else if (guessMbps < 120){
    cout << "Guessing this is a 100 Mbps link\n";
    guessMbps = 100;
    charSpace[0] = 1;
    numberSentChars = 2000000;
  }
  else{
    cout << "Guessing this is a 1000 Mbps link\n";
    guessMbps = 1000;
    charSpace[0] = 2;
    numberSentChars = 20000000;
  }
  comm->ar_safeWrite(charSpace,1);
  init_time = ar_time();
  comm->ar_safeRead(charSpace,numberSentChars);
  done_time = ar_time();
  cout << "TCP Link speed = " 
       << (8.0*numberSentChars)/ar_difftime(done_time,init_time)
       << " Mbps\n";

  // next, we receive 10 structured data packets, unpack them into 
  // storage, and copy that storage into memory

  cout << "Testing the Structured Data link speed\n";
  init_time = ar_time();
  for (int i=0; i<10; i++){
    szgClient.getData(charSpace,charSpaceSize);
    testData1.unpack(charSpace);
    testData1.dataOut(CHAR_DATA_ID,anotherBuffer,AR_CHAR,numberSentChars/10);
    testData1.dataOut(INT_DATA_ID,intSpace,AR_INT,10);
    testData1.dataOut(FLOAT_DATA_ID,floatSpace,AR_FLOAT,10);
    testData1.dataOut(DOUBLE_DATA_ID,doubleSpace,AR_DOUBLE,10);
  } 
  done_time = ar_time();
  cout << "Structured Data Link speed = " 
       << (8.0*numberSentChars)/ar_difftime(done_time,init_time)
       << " Mbps\n";
  
  // Next, we receive a single structured data packet. This tests
  // network translation capabilities between machines w/ differnet endianess
  szgClient.getData(charSpace,charSpaceSize);
  testData2.unpack(charSpace);
  cout << "****** Dumping the arStructuredData unpacked from the wire.\n";
  testData2.dump(true);
  testData3.parse(charSpace); 
  cout << "****** Dumping the arStructuredData parsed from the wire.\n";
  testData3.dump(true);

  // Finally, we go ahead and receive a text version of the data record.
  arStructuredDataParser theParser(theDictionary);
  arSocketTextStream socketStream;
  socketStream.setSource(comm);
  arStructuredData* result = theParser.parse(&socketStream);
  if (!result){
    cerr << "****** Failed to read text from the wire(1).\n";
  }
  else{
    cout << "****** Succeeded in reading text from the wire(1).\n";
    result->print();
  }
  // do it again...
  result = theParser.parse(&socketStream);
  if (!result){
    cerr << "****** Failed to read text from the wire(2).\n";
  }
  else{
    cout << "****** Succeeded in reading text from the wire(2).\n";
    result->print();
  }
  // Sometimes, we need to be able to put various tags into the
  // stream of text XML records. We test that functionality now.
  string nextTag = ar_getTagText(&socketStream);
  cout << "****** The out-of-band tag = " << nextTag << "\n";
  // Read one more record...
  result = theParser.parse(&socketStream);
  if (!result){
    cerr << "****** Failed to read text from the wire(3).\n";
  }
  else{
    cout << "****** Succeeded in reading text from the wire(3).\n";
    result->print();
  }
  // Now, we'll read in the tag and then parse the rest of the record.
  nextTag = ar_getTagText(&socketStream);
  if (!result){
    cerr << "****** Failed to read text from the wire(4).\n";
  }
  else{
    cout << "****** Succeeded in reading text from the wire(4).\n";
    result->print();
  }result = theParser.parse(&socketStream, nextTag);
  
  cout << "****** Now, we test the speed of text record sending.\n";
  ar_timeval time1 = ar_time();
  for (int j=0; j<1000; j++){
    result = theParser.parse(&socketStream);
  }
  cout << "****** XML record send rate per second = " 
       << 1000000000.0/ar_difftime(ar_time(), time1) << "\n";
  szgClient.closeConnection();
}
