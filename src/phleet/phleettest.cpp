//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

const int numberGetAttr = 2500;
bool done1 = false;
bool done2 = false;

// WHY IS THERE A SEGFAULT IF THIS IS DECLARED LIKE SO?
arSZGClient* szgClient = NULL;

void getTask1(void*){
  for (int i=0; i<numberGetAttr; i++){
    const string result(szgClient->getAttribute("foo", "bar1"));
    if (result != "bar1"){
      cout << "Error in get 1 (" << result << ").\n";
    }
  }
  done1 = true;
}

void getTask2(void*){
  for (int i=0; i<numberGetAttr; i++){
    const string result(szgClient->getAttribute("foo", "bar2"));
    if (result != "bar2"){
      cout << "Error in get 2 (" << result << ").\n";
    }
  }
  done2 = true;
}

int main(int argc, char** argv){
  // DOES NOT SEEM LIKE WE CAN HAVE A SUCCESSION OF arSZGCients 
  // connecting from the same program.
  szgClient = new arSZGClient;
  const bool fInit = szgClient->init(argc, argv);
  if (!*szgClient)
    return szgClient->failStandalone(fInit);

  while (true) {
    cout << "First trying connection speed.\n";
    int i = 0;

#if 0
    ar_timeval contime = ar_time();
    const int numberCon = 30;
    for (i=0; i<numberCon; i++){
      szgClient = new arSZGClient;
      szgClient->init(argc, argv);
      if (!*szgClient)
	return szgClient->failStandalone(fInit);

      szgClient->closeConnection();
      delete szgClient;
    }
    const double totalCon = ar_difftime(ar_time(), contime);
    cout << "Time to get attribute is "
	 << totalCon/(1000.0*numberCon) << " ms.\n"
	 << "  Requests per second = " 
	 << (1000000.0*numberCon)/totalCon << "\n";
    szgClient = new arSZGClient;
    szgClient->init(argc, argv);
    if (!*szgClient)
      return szgClient->failStandalone(fInit);

    szgClient->closeConnection();
    delete szgClient;
  }

  double totalCon = ar_difftime(ar_time(), contime);
    cout << "Time to get attribute is "
         << totalCon/(1000.0*numberCon) << " ms.\n";
    cout << "  Requests per second = " 
         << (1000000.0*numberCon)/totalCon << "\n";
  szgClient = new arSZGClient;
  szgClient->init(argc, argv);
  if (!*szgClient){
    return szgClient->failStandalone(fInit);
    }
#endif

    szgClient->setAttribute("foo","bar1","bar1");
    szgClient->setAttribute("foo","bar2","bar2");
    ar_timeval time1 = ar_time();
    for (i=0; i<numberGetAttr; i++){
      (void)szgClient->getAttribute("foo","bar1"); 
    }
    const double totalTime = ar_difftime(ar_time(), time1);
    cout << "Time to get attribute is "
         << totalTime/(1000.0*numberGetAttr) << " ms.\n"
         << "  Requests per second = " 
         << (1000000.0*numberGetAttr)/totalTime << "\n";
    done1 = false;
    done2 = false;
    arThread dummy1(getTask1);
    arThread dummy2(getTask2);
    while (!done1 || !done2){
      ar_usleep(200000);
    }

    /*szgClient->closeConnection();
      delete szgClient;*/

  }
  return 0;
}
