//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
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
  // Does nothing.
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
}

int main(int argc, char** argv){
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  loadParameters(szgClient);
  arSoundServer soundServer;
  dsSetSoundDatabase(&soundServer);

  if (!soundServer.init(szgClient)){
    ar_log_error() << "SoundServer failed to init.\n";
    return 1;
  }
  if (!soundServer.start()){
    ar_log_error() << "SoundServer failed to start.\n";
    return 1;
  }

  arThread dummy(ar_messageTask, &szgClient);

  initDatabase();

  while (true){
    changeDatabase();
    ar_usleep(500000);
  }
  return 0;
}
