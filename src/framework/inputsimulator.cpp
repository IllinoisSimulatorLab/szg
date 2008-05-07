//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arGenericDriver.h"
#include "arGlut.h"
#include "arGraphicsHeader.h"
#include "arInputNode.h"
#include "arInputSimulator.h"
#include "arInputSimulatorFactory.h"
#include "arNetInputSink.h"
#include "arNetInputSource.h"
#include "arPForthFilter.h"

arSZGClient* pszgClient = NULL;
arInputSimulator defaultSimulator;
arInputSimulator* pSim = NULL;
arInputNode inputNode;
int xPos = 0;
int yPos = 0;

void loadParameters(arSZGClient& szgClient){
  int posBuffer[2];
  const string posString = szgClient.getAttribute("SZG_INPUTSIM", "position");
  if (posString != "NULL" && ar_parseIntString(posString,posBuffer,2)){
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
  pSim->draw();
  glutSwapBuffers();
}

void idle(){
  pSim->advance();
#if 0
  bool dump = false;
  for (unsigned i=0; i<inp->getNumberButtons(); ++i) {
    dump |= inp->getOnButton(i);
  if (dump) {
    cerr << *inp << endl;
  }
#endif
  display();
  ar_usleep(20000); // Mild CPU throttle
}

void keyboard(unsigned char key, int x, int y){
  pSim->keyboard(key,1,x,y);
  switch(key){
  case 27:
    pszgClient->messageTaskStop();
    exit(0);
  }
}

void mouseButton(int button, int state, int x, int y){
  // Translate GLUT numbers to Syzygy.
  const int whichButton = 
  //(button == GLUT_LEFT_BUTTON  ) ? 0:
    (button == GLUT_MIDDLE_BUTTON) ? 1 :
    (button == GLUT_RIGHT_BUTTON ) ? 2 : 0;
  const int whichState = (state == GLUT_DOWN) ? 1 : 0;
  pSim->mouseButton(whichButton, whichState, x, y);
}

void mousePosition(int x, int y){
  pSim->mousePosition(x,y);
}

void messageTask(void* pv){
  arSZGClient* pszgClient = (arSZGClient*)pv;
  string messageType, messageBody;
  while (pszgClient->running()) {
    if (!pszgClient->receiveMessage(&messageType,&messageBody)){
      ar_log_debug() << "inputsimulator shutdown.\n";
      goto LStop;
    }
    if (messageType=="quit"){
LStop:
      inputNode.stop();
      exit(0);
    }
  }
}

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  if (argc > 3) {
    ar_log_critical() << "usage: inputsimulator [slot [-netinput]]\n";
LAbort:
    (void)szgClient.sendInitResponse(false);
    return 1;
  }

  pszgClient = &szgClient;
  const unsigned slot = (argc > 1) ? atoi(argv[1]) : 0;
  ar_log_remark() << "using slot " << slot << ".\n";

  // "netinput" doesn't mean input.  It means "output on the next slot."  I think.
  const bool fNetInput = (argc > 2) && !strcmp(argv[2], "-netinput");

  arNetInputSink netInputSink;
  if (!netInputSink.setSlot(slot)) {
    ar_log_critical() << "failed to set slot " << slot << ".\n";
    goto LAbort;
  }

  // Distinguish different SZG_INPUTn services.
  netInputSink.setInfo("inputsimulator");

  const string pforthProgramName = szgClient.getAttribute("SZG_PFORTH", "program_names");
  if (pforthProgramName == "NULL"){
    ar_log_remark() << "no pforth program for standalone joystick.\n";
  }
  else{
    const string pforthProgram = szgClient.getGlobalAttribute(pforthProgramName);
    if (pforthProgram == "NULL"){
      ar_log_remark() << "no pforth program for '" << pforthProgramName << "'\n";
    }
    else{
      arPForthFilter* filter = new arPForthFilter();
      ar_PForthSetSZGClient( &szgClient );
      if (!filter->loadProgram( pforthProgram )){
        ar_log_critical() << "failed to configure pforth filter with program '" <<
	  pforthProgram << "'.\n";
	goto LAbort;
      }
      // The input node is not responsible for clean-up
      inputNode.addFilter(filter, false);
    }
  }
  
  {
    arInputSimulatorFactory simFactory;
    arInputSimulator* simTemp = simFactory.createSimulator( szgClient );
    pSim = simTemp ? simTemp : &defaultSimulator;
  }
  pSim->configure(szgClient);
  pSim->registerInputNode(&inputNode);
  if (fNetInput) {
    arNetInputSource* netSource = new arNetInputSource;
    if (!netSource->setSlot(slot+1)) {
      ar_log_critical() << "failed to set slot " << slot+1 << " for netinput.\n";
      goto LAbort;
    }

    inputNode.addInputSource(netSource, true);
    ar_log_remark() << "using net input, slot " << slot+1 << ".\n";
    // Memory leak.  inputNode won't free its input sources, I think.
  }
  inputNode.addInputSink(&netInputSink,false);

  if (!inputNode.init(szgClient)) {
    goto LAbort;
  }

  (void)szgClient.sendInitResponse(true);

  if (!inputNode.start()){
    (void)szgClient.sendStartResponse(false);
    return 1;
  }

  (void)szgClient.sendStartResponse(true);

  arThread dummy(messageTask, &szgClient);
  loadParameters(szgClient);

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
  glutInitWindowSize(250,250);
  glutInitWindowPosition(xPos, yPos);
  glutCreateWindow(szgClient.getUserName() == "gfrancis" ? "wandsimpercolator" : "inputsimulator");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboard);
  glutPassiveMotionFunc(mousePosition);
  glutMotionFunc(mousePosition);
  glutMouseFunc(mouseButton);
  glutMainLoop();
  return 0;
}
