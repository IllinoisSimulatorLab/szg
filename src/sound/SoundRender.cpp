//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSoundDatabase.h"
#include "arSoundClient.h"
#include "arMath.h"
#include "arSoundFile.h"
#include "arThread.h"
#include "arSZGClient.h"

arSoundClient* soundClient;

// the parameter variables
char serverIP[1024] = {0}; /// \todo fixed size buffer
int serverPort = -1;
string textPath;
arSpeakerObject speakerObject;

bool loadParameters(arSZGClient& cli){
  stringstream& initResponse = cli.initResponse();
  // using the arSoundClient's configure(...) method now.
  //const string path = cli.getAttribute("SZG_SOUND", "path");
  //soundClient->setPath(path);
  soundClient->configure(&cli);
  if (soundClient->getPath() == "NULL"){
    initResponse << "SoundRender warning: SZG_SOUND/path "
                 << soundClient->getPath() << " invalid or undefined.\n";
  }

  return speakerObject.configure(&cli);
}

void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (true) {
    int sendID = cli->receiveMessage(&messageType,&messageBody);
    if (!sendID){
      // sendID == 0 exactly when we are "forced" to shutdown.
      cout << "SoundRender is shutting down.\n";
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
  if (!szgClient)
    return 1;

  // Only a single sound render should be running on a given computer
  // copy-pasted (more or less) from szgd.cpp
  int ownerID = -1;
  if (!szgClient.getLock(szgClient.getComputerName()+"/SoundRender", ownerID)){
    szgClient.initResponse() << argv[0] 
      << " error: another copy is already running (pid = " 
      << ownerID << ").\n";
    szgClient.sendInitResponse(false);
    return 1;
  }
  
  // get the initial parameters
  if (!loadParameters(szgClient)){
    szgClient.sendInitResponse(false);
    return 1;
  }
  // we've succeeded in initialization
  szgClient.sendInitResponse(true);

  soundClient->setSpeakerObject(&speakerObject);
  soundClient->setNetworks(szgClient.getNetworks("sound"));
  // We want to start the DSP callback, which can be used to relay the
  // playing waveform elsewhere.
  soundClient->startDSP();
  soundClient->start(szgClient);

  // we've started by this point
  szgClient.sendStartResponse(true);  

  arThread dummy(messageTask, &szgClient);
  while (true) {
    soundClient->_cliSync.consume();
    // NOTE, it is VERY IMPORTANT that consume() not be called too often.
    // Since there is no natural limit to how many times this can be called
    // (unlike graphics where we at least have the buffer swap), we have
    // to throttle it. This will CRASH on Win32 without this throttling.
    ar_usleep(1000000/50); // 50 FPS CPU-throttle
  }
  return 0;
}
