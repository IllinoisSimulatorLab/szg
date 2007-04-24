//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

// This program tests receiving data from 2 different servers simultaneously
// ( a good way to verify that the server discovery functions of the
// arSZGClient are threadsafe

arSZGClient szgClient;
arDataClient dataClient[2];
string serviceName[2];
arLock printLock;
arSlashString networks;

void readDataTask(void* num){
  int number = *((int*) num);
  int trial = 0;
  while(true){
    cout << "&&&&&& Component ID of service " << serviceName[number]
	 << " = " << szgClient.getServiceComponentID(serviceName[number]) << "\n";
    trial++;
    const int whichNetwork = rand()% networks.size();
    const string testNetwork = networks[whichNetwork];
    printLock.lock();
    cout << "***** Thread number = " << number << "\n"
         << "  trial = " << trial << "\n"
         << "  Discovering service on network " << testNetwork << "\n";
    printLock.unlock();
    // the call to discoverService uses some async mojo...
    // the upshot is that discoverService blocks until an appropriate service
    // is registered
    arPhleetAddress result 
      = szgClient.discoverService(serviceName[number], testNetwork, true);
    printLock.lock();
    cout << "***** Thread number = " << number << "\n"
         << "  trial = "<< trial << "\n"
         << "  Service = " << serviceName[number] << "\n"
         << "  Address = " << result.address << "\n"
         << "  Number ports = " << result.numberPorts << "\n"
         << "  Ports = ";
    for (int i=0; i<result.numberPorts; i++){
      cout << result.portIDs[i] << " ";
    }
    cout << "\n";
    printLock.unlock();
    
    if (!dataClient[number].dialUpFallThrough(result.address.c_str(), 
                                              result.portIDs[0])){
      printLock.lock();
      cout << "***** Thread number = " << number << "\n";
      cout << "  trial = " << trial << "\n";
      cout << "  error: failed to connect to supposedly registered server.\n";
      printLock.unlock();
      continue;
    }
    arTemplateDictionary* dictionary = dataClient[number].getDictionary();
    int bufferSize = 1000;
    ARchar* buffer = new ARchar[bufferSize];
    arStructuredData* data = new arStructuredData(dictionary->find("test"));
    for (int k=0; k<10; k++){
      if (!dataClient[number].getData(buffer, bufferSize)){
        printLock.lock();
        cout << "***** Thread number = " << number << "\n";
        cout << "  trial = " << trial << "\n";
        cout << "  error: szgClient failed to get data.\n";
        printLock.unlock();
	// try to discover a new service
        break;
      }
      data->unpack(buffer);
      printLock.lock();
      cout << "##### Thread number = " << number << "\n";
      data->print();
      printLock.unlock();
    }
    dataClient[number].closeConnection();
  }
}

int main(int argc, char** argv){
  if (argc < 3){
    cout << "usage: testserviceclient <service name 1> <service name 2>\n";
    return 1;
  }
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  arPhleetConfig config;
  if (!config.read())
    return 1;
  networks = config.getNetworks();
  serviceName[0] = string(argv[1]);
  serviceName[1] = string(argv[2]);
  
  // num1 and num2 need to be distinct, lest each thread attach to source 1.

  int num1 = 0;
  arThread thread1;
  thread1.beginThread(readDataTask, &num1);

  int num2 = 1;
  arThread thread2;
  thread2.beginThread(readDataTask, &num2);

  while (true){
    ar_usleep(100000);
  }
  return 0;
}
