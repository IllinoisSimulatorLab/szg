//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDataServer.h"
#include "arStructuredData.h"
#include "arDataTemplate.h"
#include "arTemplateDictionary.h"
#include "arThread.h"
#include <sstream>
#include <iostream>
using namespace std;

arDataServer* dataServer = NULL;

void acceptConnections(void*){
  while (dataServer->acceptConnection() != NULL)
    cerr << "Got a connection.\n";
}

int main(int argc, char** argv){
  arTemplateDictionary dictionary;
  arDataTemplate testTemplate("Test Data");
  const int INT_DATA_ID = testTemplate.add("Int Data", AR_INT);
  const int CHAR_DATA_ID = testTemplate.add("Char Data", AR_CHAR);
  const int FLOAT_DATA_ID = testTemplate.add("Float Data", AR_FLOAT);
  const int DOUBLE_DATA_ID = testTemplate.add("Double Data",AR_DOUBLE);
  dictionary.add(&testTemplate);
  dictionary.dump();
  dataServer = new arDataServer(1000);
  // OK, I was making an attempt to deal with low throughput on WANs
  // via the socket options... changing the buffer size here definitely
  // makes a difference, though I never saw anything better than about
  // 12 Mbps between LA and UIUC
  //dataServer->setBufferSize(200000);
  if (!dataServer->setPort(3000))
    return -1;

  if (argc>1){
    cout << argv[0] << "remark: setting interface to " << argv[1] << ".\n";
    if (!dataServer->setInterface(argv[1]))
      return -1;
  }
  arStructuredData testData(&testTemplate);
  char* charData = new char[20000000];
  int* intData = new int[10];
  float* floatData = new float[10];
  double* doubleData = new double[10];
  cout << argv[0] << "remark: accepting connections.\n";
  if (!dataServer->beginListening(&dictionary))
    return -1;

  while (true) {
    arSocket* comm = dataServer->acceptConnection();
    cout << "Got a connection\n";
    // send some data to preliminarily test the link
    comm->ar_safeWrite(charData,200000);
    // receive the guess from the other side as to the link rate
    comm->ar_safeRead(charData,1);
    int numberSentChars;
    if (charData[0] == 0){
      cout << "Guessing link is 10 Mbps\n";
      numberSentChars = 200000;
    }
    else if (charData[0] == 1){
      cout << "Guessing link is 100 Mbps\n";
      numberSentChars = 2000000;
    }
    else{
      cout << "Guessing link is 1000 Mbps\n";
      numberSentChars = 20000000;
    }
    // now, let's make a more accurate test
    comm->ar_safeWrite(charData,numberSentChars);

    // Test the arStructuredData data rate!
    int i;
    for (i=0; i<10; i++){
      if (!testData.dataIn(CHAR_DATA_ID,charData,AR_CHAR,numberSentChars/10) ||
          !testData.dataIn(INT_DATA_ID,intData,AR_INT,10) ||
          !testData.dataIn(FLOAT_DATA_ID,floatData,AR_FLOAT,10) ||
          !testData.dataIn(DOUBLE_DATA_ID,doubleData,AR_DOUBLE,10) ||
          !dataServer->sendData(&testData,comm)) {
        cerr << argv[0] << " warning: dataIn or sendData failed.\n";
      }
    }

    // time to see if the data gets across correctly through the
    // arStructuredData mediator
    for (i=0; i<10; i++){
      charData[i] = 'A' + i;
      intData[i] = i*2;
      floatData[i] = i/10.0;
      doubleData[i] = i/2.0;
    }
    if (!testData.dataIn(CHAR_DATA_ID,charData,AR_CHAR,10) ||
        !testData.dataIn(INT_DATA_ID,intData,AR_INT,10) ||
        !testData.dataIn(FLOAT_DATA_ID,floatData,AR_FLOAT,10) ||
        !testData.dataIn(DOUBLE_DATA_ID,doubleData,AR_DOUBLE,10) ||
        !dataServer->sendData(&testData,comm)) {
      cerr << argv[0] << " warning: dataIn or sendData failed.\n";
    }
    // see if the data gets through the text-thing correctly...
    stringstream s;
    testData.print(s);
    string socketOutput = s.str();
    if (!comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length())){
      cerr << argv[0] << " warning: sending text version of test data "
	   << "failed (1).\n";
    }
    if (!comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length())){
      cerr << argv[0] << " warning: sending text version of test data "
	   << "failed (2).\n";
    }
    string extraTag("    <extra_tag>      ");
    if (!comm->ar_safeWrite(extraTag.c_str(), extraTag.length())){
      cerr << argv[0] << " warning: sending extra tag failed.\n";
    }
    if (!comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length())){
      cerr << argv[0] << " warning: sending text version of test data "
	   << "failed (3).\n";
    }
    if (!comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length())){
      cerr << argv[0] << " warning: sending text version of test data "
	   << "failed (3).\n";
    }
    // Finally, test the speed of XML record sending...
    for (i=0; i<1000; i++){
      stringstream s2;
      testData.print(s2);
      socketOutput = s2.str();
      comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length());
    }
  }
}
