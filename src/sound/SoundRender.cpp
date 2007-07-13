//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSoundDatabase.h"
#include "arSoundClient.h"
#include "arMath.h"
#include "arSoundFile.h"
#include "arSZGClient.h"
#include "arLogStream.h"

arSoundClient* soundClient = NULL;

// Parameter variables.
char serverIP[1024] = {0}; // todo: fixed size buffer
int serverPort = -1;
string textPath;
arSpeakerObject speakerObject;

bool loadParameters(arSZGClient& cli){
  soundClient->configure(&cli);
  if (soundClient->getPath() == "NULL"){
    ar_log_warning() << "SoundRender: undefined or invalid SZG_SOUND/path '"
                     << soundClient->getPath() << "'.\n";
  }

  soundClient->addDataBundlePathMap("SZG_DATA", cli.getDataPath());
  soundClient->addDataBundlePathMap("SZG_PYTHON", cli.getDataPathPython());
  return speakerObject.configure(cli);
}

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (cli->running()) {
    const int sendID = cli->receiveMessage(&messageType,&messageBody);
    if (!sendID){
      ar_log_debug() << "SoundRender shutdown.\n";
      goto LQuit;
    }
    if (messageType=="quit"){
LQuit:
      soundClient->_cliSync.skipConsumption(); // fold into terminateSound()?
      soundClient->terminateSound();
      exit(0);
    }
    if (messageType=="reload"){
      if (!loadParameters(*cli))
        exit(0);
    }

    else if (messageType=="log") {
      if (ar_setLogLevel( messageBody )) {
        ar_log_remark() << "SoundRender set log level to " << messageBody << ar_endl;
      } else {
        ar_log_error() << "SoundRender ignoring unrecognized loglevel '"
                         << messageBody << "'.\n";
      }
    }

    else if (messageType=="szg_sound_stream_info"){
      const string response(soundClient->processMessage(messageType,messageBody));
      cli->messageResponse(sendID, response);
    }
  }
}

int main(int argc, char** argv){
  soundClient = new arSoundClient;
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  // Force the component's name, because win98 can't provide it.
  const bool fInit = szgClient.init(argc, argv, "SoundRender");
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  // Only one SoundRender per host.
  // copy-pasted (more or less) from szgd.cpp
  int ownerID = -1;
  if (!szgClient.getLock(szgClient.getComputerName()+"/SoundRender", ownerID)){
    ar_log_error()
      << "SoundRender error: another copy is already running (pid = " 
      << ownerID << ").\n";
    if (!szgClient.sendInitResponse(false)){
      cerr << "SoundRender error: maybe szgserver died.\n";
    }
    return 1;
  }

  if (!loadParameters(szgClient)){
    if (!szgClient.sendInitResponse(false)){
      cerr << "SoundRender error: maybe szgserver died.\n";
    }
    return 1;
  }

  // init succeeded
  if (!szgClient.sendInitResponse(true)){
    cerr << "SoundRender error: maybe szgserver died.\n";
  }

  soundClient->setSpeakerObject(&speakerObject);
  soundClient->setNetworks(szgClient.getNetworks("sound"));
  if (!soundClient->init()) {
    ar_log_warning() << "SoundRender silent.\n";
    return 1;
  }

  // Now that init() has started FMOD, start the FMOD DSP callback
  // which can forward playing sounds.
  if (!soundClient->startDSP() ||
      !soundClient->start(szgClient)) {
    if (!szgClient.sendStartResponse(false))
      cerr << "SoundRender error: maybe szgserver died.\n";
    return 1;
  }

  if (!szgClient.sendStartResponse(true)){
    cerr << "SoundRender error: maybe szgserver died.\n";
  }

  arThread dummy(messageTask, &szgClient);
  while (szgClient.running()) {
    soundClient->_cliSync.consume();
    // Only the LAN throttles arSoundClient/ arSoundServer communication,
    // allowing thousands of updates per second.  Throttle down to 50 FPS.
    ar_usleep(1000000/50);
  }
  szgClient.messageTaskStop();
  return 0;
}
