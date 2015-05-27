//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSoundServer.h"
#include "arSZGClient.h"
#include "arSoundAPI.h"

#ifdef AR_USE_WIN_32
inline float drand48() {
  return float(rand()) / float(RAND_MAX);
}
#endif

int fooID = -1;
int barID = -1;
int zipID = -1;
int speechID = -1;

/* The database is a tree, one layer deep:
 *
 * "root" +--> "foo"
 *        |
 *        +--> "bar"
 *        |
 *        +--> "zip"
 *        |
 *        +--> "speech"
 *        |
 *        +--> "unchanging"
 *
 * Seen as IDs for changeable things,
 * this is merely a set not a tree:
 *
 * { fooID, barID, zipID, speechID }
 *
 */

void initDatabase() {
  const float xyz[3] = { 1, 2, 3 };
  fooID = dsLoop("foo", "root", "parade.wav", 0, 0.0, xyz);
  barID = dsLoop("bar", "root", "q33move.mp3", 1, 0.3, xyz);
  zipID = dsLoop("zip", "root", "q33collision.wav", 0, 0.0, xyz);
  speechID = dsSpeak("speech", "root", "Starting up" );
  (void)dsLoop("unchanging", "root", "q33beep.wav", 1, 0.1, xyz);
}

void changeDatabase() {
  const float xyz[3] = { float(drand48()-.5), float(drand48()-.5), float(drand48()-.5) };
  static int trigger = 0;
  ++trigger;
  ar_log_debug() << "SoundTest changing sounds.\n";
  (void)dsLoop(fooID, "parade.wav", (trigger%20)?0:-1, .8, xyz);
  (void)dsLoop(barID, "q33move.mp3", 1, 0.2, xyz);
  (void)dsLoop(zipID, "q33collision.wav", (trigger%6)?0:-1, .8, xyz);
  dsSpeak(speechID, "I can talk." );
}

int main(int argc, char** argv) {
  arSZGClient szgClient;
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  arSoundServer soundServer;
  dsSetSoundDatabase(&soundServer);

  if (!soundServer.init(szgClient)) {
    ar_log_critical() << "SoundTest server failed to init.\n";
    return 1;
  }
  if (!soundServer.start()) {
    ar_log_critical() << "SoundTest server failed to start.\n";
    return 1;
  }

  arThread dummy(ar_messageTask, &szgClient);
  ar_log_remark() << "SoundTest playing sounds.\n";
  initDatabase();
  while (szgClient.running()) {
    ar_usleep(5000000);
    changeDatabase();
  }
  ar_log_remark() << "SoundTest exiting.\n";
  szgClient.messageTaskStop();
  return 0;
}
