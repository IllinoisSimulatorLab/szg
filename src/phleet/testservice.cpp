//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSZGClient.h"
#include "arDataServer.h"

arDataServer server(1000);

void connectionTask(void*){
  while (true){
    server.acceptConnection();
  }
}

int main(int argc, char** argv){
  if (argc < 2){
    cout << "usage: testservice <service name>\n";
    return 1;
  }
  arSZGClient client;
  client.init(argc, argv);
  if (!client){
    return 1;
  }
  // go ahead and create the language
  arTemplateDictionary dictionary;
  arDataTemplate record("test");
  int TAG = record.add("tag", AR_CHAR);
  record.add("data", AR_INT);
  dictionary.add(&record);
  int ports[10];
  if (!client.registerService(argv[1],"default",1,ports)){
    cout << "testservice remark: failed to register service.\n";
    cout << "  Will try one more time upon current service expiration.\n";
    int match = client.requestServiceReleaseNotification(argv[1]);
    list<int> tags;
    tags.push_back(match);
    if (client.getServiceReleaseNotification(tags) < 0){
      cout << "szgserver error: failed to get matching tag.\n";
      return 1;
    }
    cout << "Old service has expired. Attempting to register again.\n";
    if (!client.registerService(argv[1],"default",1,ports)){
      cout << "testservice error: failed again to register servie.\n";
      return 1;
    }
  }
  server.setPort(ports[0]);
  server.setInterface("INADDR_ANY");
  int tries = 0;
  while (!server.beginListening(&dictionary)){
    if (!client.requestNewPorts(argv[1],"default",1,ports)){
      cout << "testservice error: failed to request ports.\n";
      return 1;
    }
    server.setPort(ports[0]);
    tries++;
    if (tries == 10){
      cout << "testservice error: too many port request attempts.\n";
      return 1;
    }
  }
  if (!client.confirmPorts(argv[1],"default",1,ports)){
    cout << "testservice error: failed to confirm ports.\n";
    return 1;
  }
  cout << "testservice remark: successfully bound to brokered ports.\n";
  arThread connectionThread;
  connectionThread.beginThread(connectionTask, NULL);

  arStructuredData data(&record);
  int counter = 0;
  while (true){
    data.dataInString(TAG, argv[1]);
    data.dataIn("data", &counter, AR_INT, 1);
    server.sendData(&data);
    if (++counter > 100)
      counter = 0;
    ar_usleep(100000);
  }
  return 0;
}
