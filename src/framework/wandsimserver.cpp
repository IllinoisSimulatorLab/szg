//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsHeader.h"
#include "arHeadWandSimulator.h"
#include "arInputNode.h"
#include "arNetInputSource.h"
#include "arNetInputSink.h"
#include "arGenericDriver.h"
#include "arPForthFilter.h"

arInputState* inp;
arSZGClient* client;
arInputNode* inputNode;

arHeadWandSimulator simulator;
int xPos = 0, yPos = 0;

void loadParameters(arSZGClient& client){
  int posBuffer[2];
  string posString = client.getAttribute("SZG_WANDSIM", "position");
  if (posString != "NULL"
      && ar_parseIntString(posString,posBuffer,2)){
    xPos = posBuffer[0];
    yPos = posBuffer[1];
  }
  else{
    xPos = 0;
    yPos = 0;
  }
}

void display(){
  glClearColor(0,0,0,0);
  glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT);
  simulator.draw();
  glutSwapBuffers();
}

void idle(){
  simulator.advance();
//bool dump(false);
//for (unsigned int i=0; i<inp->getNumberButtons();++i) {
//  if (inp->getOnButton(i))
//    dump = true;
//}
//if (dump) {
//  cerr << *inp << endl;
//}
  display();
  // we don't want to spin a CPU with the simulator, so framerate should
  // be throttled... but don't be too agressive about throttling...
  // otherwise the controls won't be responsive
  ar_usleep(20000);
}

void keyboard(unsigned char key, int x, int y){
  simulator.keyboard(key,1,x,y);
  // there is also some local key processing to do...
  switch(key){
  case 27:
    exit(0);
    break;
  }
}

void mouseButton(int button, int state, int x, int y){
  // translate from GLUT's magic numbers to ours
  const int whichButton = 
    (button == GLUT_LEFT_BUTTON  ) ? 0:
    (button == GLUT_MIDDLE_BUTTON) ? 1 :
    (button == GLUT_RIGHT_BUTTON ) ? 2 : 0;
  const int whichState = (state == GLUT_DOWN) ? 1 : 0;
  simulator.mouseButton(whichButton, whichState, x, y);
}

void mousePosition(int x, int y){
  simulator.mousePosition(x,y);
}

void messageTask(void* pClient){
  string messageType, messageBody;
  while (true) {
    int sendID = client->receiveMessage(&messageType,&messageBody);
    if (!sendID){
      // sendID == 0 exactly when we are "forced" to shutdown.
      cout << "Wandsimserver is shutting down.\n";
      // Cut-and-pasted from below.
      inputNode->stop();
      exit(0);
    }
    if (messageType=="quit"){
      inputNode->stop();
      exit(0);
    }
  }
}

int main(int argc, char** argv){
  client = new arSZGClient();
  inputNode = new arInputNode();
  client->simpleHandshaking(false);
  client->init(argc, argv);  
  if (!(*client))
    return 1;

  int slotNumber(0);
  if (argc > 1) {
    slotNumber = atoi(argv[1]);
  }
  cout << "wandsimserver remark: using slot #" << slotNumber << endl;
  bool useNetInput(false);
  if (argc > 2) {
    if (std::string(argv[2]) == "-netinput") {
      useNetInput = true;
    }
  }
  arNetInputSink netInputSink;
  netInputSink.setSlot(slotNumber);
  // NOTE: we need to distinguish different kinds of SZG_INPUTn services...
  // and this is how we do it!
  netInputSink.setInfo("wandsimserver");
  arPForthFilter pforth;
  if (!pforth.configure(client)){
    client->sendInitResponse(false);
    return 1;
  }
  
  simulator.registerInputNode(inputNode);
  inp = &inputNode->_inputState;
  if (useNetInput) {
    arNetInputSource* netSource = new arNetInputSource;
    netSource->setSlot(slotNumber+1);
    inputNode->addInputSource(netSource,true);
    cerr << "wandsimserver remark: using net input, slot #" 
         << slotNumber+1 << ".\n";
    // Memory leak.  inputNode won't free its input sources, I think.
  }
  inputNode->addInputSink(&netInputSink,false);
  inputNode->addFilter(&pforth, true);
  if (!inputNode->init(*client)) {
    client->sendInitResponse(false);
    return 1;
  }
  // initialization succeeded
  client->sendInitResponse(true);

  if (!inputNode->start()){
    client->sendStartResponse(false);
    return 1;
  }
  
  arThread messageThread;
  messageThread.beginThread(messageTask);

  loadParameters(*client);

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
  glutInitWindowSize(250,250);
  glutInitWindowPosition(xPos, yPos);
  glutCreateWindow("wandsimserver");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboard);
  glutPassiveMotionFunc(mousePosition);
  glutMotionFunc(mousePosition);
  glutMouseFunc(mouseButton);

  // start has succeeded by this point
  client->sendStartResponse(true);
  glutMainLoop();
}
