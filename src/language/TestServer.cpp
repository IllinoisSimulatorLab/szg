//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arDataServer.h"
#include "arStructuredData.h"
#include "arDataTemplate.h"
#include "arTemplateDictionary.h"
#include "arThread.h"
#include <sstream>

arDataServer* dataServer = NULL;

void acceptConnections(void*){
  while (dataServer->acceptConnection() != NULL)
    cerr << "Got a connection.\n";
}

int main(int argc, char** argv){
  arTemplateDictionary dictionary;
  arDataTemplate testTemplate("Test_Data");
  const int INT_DATA_ID = testTemplate.add("Int_Data", AR_INT);
  const int CHAR_DATA_ID = testTemplate.add("Char_Data", AR_CHAR);
  const int FLOAT_DATA_ID = testTemplate.add("Float_Data", AR_FLOAT);
  const int DOUBLE_DATA_ID = testTemplate.add("Double_Data",AR_DOUBLE);
  dictionary.add(&testTemplate);
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
  if (!dataServer->beginListening(&dictionary))
    return -1;

  while (true) {
    arSocket* comm = dataServer->acceptConnection();
    if (!comm){
      continue;
    }
    // send some data to preliminarily test the link
    comm->ar_safeWrite(charData,200000);
    // receive the guess from the other side as to the link rate
    comm->ar_safeRead(charData,1);
    int numberSentChars = -1;
    if (charData[0] == 0){
      numberSentChars = 200000;
    }
    else if (charData[0] == 1){
      numberSentChars = 2000000;
    }
    else{
      numberSentChars = 20000000;
    }
    // now, let's make a more accurate test
    comm->ar_safeWrite(charData,numberSentChars);

    // Test the arStructuredData data rate!
    int i = 0;
    // Very large records (here the memory copying will dominate the 
    // decoding).
    bool errorFlag = false;
    for (i=0; i<10; i++){
      if (!testData.dataIn(CHAR_DATA_ID,charData,AR_CHAR,numberSentChars/10) ||
          !testData.dataIn(INT_DATA_ID,intData,AR_INT,10) ||
          !testData.dataIn(FLOAT_DATA_ID,floatData,AR_FLOAT,10) ||
          !testData.dataIn(DOUBLE_DATA_ID,doubleData,AR_DOUBLE,10) ||
          !dataServer->sendData(&testData,comm)) {
        cerr << argv[0] << " warning: dataIn or sendData failed.\n";
	errorFlag = true;
	break;
      }
    }
    if (errorFlag){
      continue;
    }
    // Small records (here the field manipulation should dominate the 
    // decoding).
    errorFlag = false;
    for (i=0; i<10000; i++){
      if (!testData.dataIn(CHAR_DATA_ID,charData,AR_CHAR,
                           numberSentChars/10000) ||
          !testData.dataIn(INT_DATA_ID,intData,AR_INT,10) ||
          !testData.dataIn(FLOAT_DATA_ID,floatData,AR_FLOAT,10) ||
          !testData.dataIn(DOUBLE_DATA_ID,doubleData,AR_DOUBLE,10) ||
          !dataServer->sendData(&testData,comm)) {
        cerr << argv[0] << " warning: dataIn or sendData failed.\n";
	errorFlag = true;
	break;
      }
    }
    if (errorFlag){
      continue;
    }

    // Time to see if the data gets across correctly through the
    // arStructuredData mediator.
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
      continue;
    }

    // See if the data gets through the text-thing correctly...
    stringstream s;
    testData.print(s);
    string socketOutput = s.str();
    if (!comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length())){
      cerr << argv[0] << " warning: sending text version of test data "
	   << "failed (1).\n";
      continue;
    }
    if (!comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length())){
      cerr << argv[0] << " warning: sending text version of test data "
	   << "failed (2).\n";
      continue;
    }
    string extraTag("    <extra_tag>      ");
    if (!comm->ar_safeWrite(extraTag.c_str(), extraTag.length())){
      cerr << argv[0] << " warning: sending extra tag failed.\n";
      continue;
    }
    if (!comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length())){
      cerr << argv[0] << " warning: sending text version of test data "
	   << "failed (3).\n";
      continue;
    }
    if (!comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length())){
      cerr << argv[0] << " warning: sending text version of test data "
	   << "failed (3).\n";
      continue;
    }
    // Finally, test the speed of XML record sending...
    for (i=0; i<1000; i++){
      stringstream s2;
      testData.print(s2);
      socketOutput = s2.str();
      if (!comm->ar_safeWrite(socketOutput.c_str(), socketOutput.length())){
	break;
      }
    }
  }
}
