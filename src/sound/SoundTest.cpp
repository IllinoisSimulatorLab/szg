//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundServer.h"
#include "arSZGClient.h"
#include "arSoundAPI.h"

int fooID = -1;
int barID = -1;
int zipID = -1;

#ifdef AR_USE_WIN_32
inline float drand48(){
  return float(rand()) / float(RAND_MAX);
}
#endif

void loadParameters(arSZGClient&){
  // does nothing!
}

// The database is a tree with one layer beneath the root.

void initDatabase(){
  // CREATE AND INITIALIZE THE DATABASE,
  // for later modification in changeDatabase()

  const float xyz[3] = { 1, 2, 3};
  fooID = dsLoop("foo", "root", "parade.wav", 0, 0.0, xyz);
  barID = dsLoop("bar", "root", "q33move.mp3", 1, 0.3, xyz);
  zipID = dsLoop("zip", "root", "q33collision.wav", 0, 0.0, xyz);
  (void)dsLoop("unchanging", "root", "q33beep.wav", 1, 0.1, xyz);

  /* A picture of this database:
   *
   * "root" +--> "foo"
   *        |
   *        +--> "bar"
   *        |
   *        +--> "zip"
   *        |
   *        +--> "unchanging"
   *
   * The same picture, seen as IDs for things we want to change:
   *
   * ______ +--> fooID
   *        |
   *        +--> barID
   *        |
   *        +--> zipID
   *        |
   *        +--> ____________
   *
   */
}

void changeDatabase(){
  // MODIFY THE DATABASE
  float xyz[3] = { drand48()-.5, drand48()-.5, drand48()-.5 };
  static int trigger = 0;
  ++trigger;
  dsLoop(fooID, "parade.wav", (trigger%20==0)?-1:0, 0.8, xyz);
  dsLoop(barID, "q33move.mp3", 1, 0.2, xyz);
  dsLoop(zipID, "q33collision.wav", (trigger%6==0)?-1:0, .8, xyz);
  cout << "\t\tServer database changed.\n";
}

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;

#define NYI
#ifdef NYI
  cerr << "under reconstruction to use framework.  see cosmos/cosmos.cpp.\n";
  return 1;
#else
  loadParameters(szgClient);
  arSoundServer soundServer;
  dsSetSoundDatabase(&soundServer);

  arThread dummy(ar_messageTask, &szgClient);
  if (!soundServer.setInterface(serverIP) ||
      !soundServer.setPort(serverPort)) {
    cerr << argv[0] << " error: invalid IP:port "
	 << serverIP  << ":" << serverPort
	 << " for sound server.\n";
    return 1;
  }
  initDatabase();
  soundServer,init(szgClient);
  if (!soundServer.start())
    return 1;
  while (true){
    changeDatabase();
    ar_usleep(500000);
  }
  return 0;
#endif
}
