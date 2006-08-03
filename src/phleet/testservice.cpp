//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"
#include "arDataServer.h"

arDataServer server(1000);

void connectionTask(void*){
  while (true){
    server.acceptConnection();
  }
}

int main(int argc, char** argv){
  arSZGClient client;
  client.init(argc, argv);
  if (!client){
    return 1;
  }

  if (argc < 2){
    cout << "usage: testservice serviceName\n";
    return 1;
  }

  // Create the language.
  arTemplateDictionary dictionary;
  arDataTemplate record("test");
  const int TAG = record.add("tag", AR_CHAR);
  record.add("data", AR_INT);
  dictionary.add(&record);
  int ports[10];
  if (!client.registerService(argv[1],"default",1,ports)){
    ar_log_remark() << "testservice failed to register service.  Will retry when current service expires.\n";
    const int match = client.requestServiceReleaseNotification(argv[1]);
    list<int> tags;
    tags.push_back(match);
    if (client.getServiceReleaseNotification(tags) < 0){
      ar_log_error() << "testservice failed to get matching tag.\n";
      return 1;
    }

    ar_log_remark() << "testservice: Old service expired. Re-registering.\n";
    if (!client.registerService(argv[1],"default",1,ports)){
      ar_log_error() << "testservice failed twice register service.\n";
      return 1;
    }
  }

  server.setPort(ports[0]);
  server.setInterface("INADDR_ANY");
  int tries = 0;
  while (!server.beginListening(&dictionary)){
    if (!client.requestNewPorts(argv[1],"default",1,ports)){
      ar_log_error() << "testservice failed to request ports.\n";
      return 1;
    }

    server.setPort(ports[0]);
    if (++tries == 10){
      ar_log_error() << "testservice requested ports too many times.\n";
      return 1;
    }
  }

  if (!client.confirmPorts(argv[1],"default",1,ports)){
    ar_log_error() << "testservice failed to confirm ports.\n";
    return 1;
  }

  ar_log_remark() << "testservice bound to brokered ports.\n";
  arThread dummy(connectionTask);
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
