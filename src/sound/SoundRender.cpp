//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arSoundDatabase.h"
#include "arSoundClient.h"
#include "arMath.h"
#include "arSoundFile.h"
#include "arThread.h"
#include "arSZGClient.h"
#include "arLogStream.h"

arSoundClient* soundClient = NULL;

// the parameter variables
char serverIP[1024] = {0}; /// \todo fixed size buffer
int serverPort = -1;
string textPath;
arSpeakerObject speakerObject;

bool loadParameters(arSZGClient& cli){
  // using the arSoundClient's configure(...) method now.
  //const string path = cli.getAttribute("SZG_SOUND", "path");
  //soundClient->setPath(path);
  soundClient->configure(&cli);
  if (soundClient->getPath() == "NULL"){
    ar_log_warning() << "SoundRender warning: SZG_SOUND/path "
                     << soundClient->getPath() << " invalid or undefined.\n";
  }

  // Must remember to set up the data bundle info.
  string dataPath = cli.getAttribute("SZG_DATA", "path");
  if (dataPath != "NULL"){
    soundClient->addDataBundlePathMap("SZG_DATA", dataPath);
  }
  // Must also do this for the other bundle path possibility.
  string pythonPath = cli.getAttribute("SZG_PYTHON", "path");
  if (pythonPath != "NULL"){
    soundClient->addDataBundlePathMap("SZG_PYTHON", pythonPath);
  }

  return speakerObject.configure(&cli);
}

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (true) {
    const int sendID = cli->receiveMessage(&messageType,&messageBody);
    if (!sendID){
      cout << "SoundRender remark: shutdown.\n";
      // Cut-and-pasted from below.
      soundClient->_cliSync.skipConsumption(); // fold into terminateSound()?
      soundClient->terminateSound();
      exit(0);
    }
    if (messageType=="quit"){
      soundClient->_cliSync.skipConsumption(); // fold into terminateSound()?
      soundClient->terminateSound();
      exit(0);
    }
    if (messageType=="reload"){
      if (!loadParameters(*cli))
        exit(0);
    }
    if (messageType=="szg_sound_stream_info"){
      string response = soundClient->processMessage(messageType,messageBody);
      cli->messageResponse(sendID, response);
    }
  }
}

int main(int argc, char** argv){
  soundClient = new arSoundClient;
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  // note how we force the name of the component. This is because it is
  // impossible to get the name automatically on Win98 and we want to run
  // SoundRender on Win98
  szgClient.init(argc, argv, "SoundRender");
  if (!szgClient){
    return 1;
  }
  
  ar_log().setStream(szgClient.initResponse());
  
  // Only a single sound render should be running on a given computer
  // copy-pasted (more or less) from szgd.cpp
  int ownerID = -1;
  if (!szgClient.getLock(szgClient.getComputerName()+"/SoundRender", ownerID)){
    ar_log_error()
      << "SoundRender error: another copy is already running (pid = " 
      << ownerID << ").\n";
    if (!szgClient.sendInitResponse(false)){
      ar_log_error() << "SoundRender error: maybe szgserver died.\n";
    }
    return 1;
  }
  
  // get the initial parameters
  if (!loadParameters(szgClient)){
    if (!szgClient.sendInitResponse(false)){
      ar_log_error() << "SoundRender error: maybe szgserver died.\n";
    }
    return 1;
  }
  // we've succeeded in initialization
  if (!szgClient.sendInitResponse(true)){
    ar_log_error() << "SoundRender error: maybe szgserver died.\n";
  }
  
  ar_log().setStream(szgClient.startResponse());

  soundClient->setSpeakerObject(&speakerObject);
  soundClient->setNetworks(szgClient.getNetworks("sound"));
  (void)soundClient->init();
  // We want to start the DSP callback, which can be used to relay the
  // playing waveform elsewhere. This must occur AFTER init (since
  // that is where the fmod library gets started!)
  soundClient->startDSP();
  (void)soundClient->start(szgClient);

  if (!szgClient.sendStartResponse(true)){
    ar_log_error() << "SoundRender error: maybe szgserver died.\n";
  }
  
  ar_log().setStream(cout);

  arThread dummy(messageTask, &szgClient);
  while (true) {
    soundClient->_cliSync.consume();
    // NOTE, it is VERY IMPORTANT that consume() not be called too often.
    // Since there is no natural limit to how many times this can be called
    // (unlike graphics where we at least have the buffer swap), we have
    // to throttle it. This will CRASH on Win32 without this throttling.
    ar_usleep(1000000/50); // 50 FPS
  }
  return 0;
}
