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

arInputNode* inputNode = NULL;

arInputSimulator defaultSimulator;
arInputSimulator* simPtr;
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
  simPtr->draw();
  glutSwapBuffers();
}

void idle(){
  simPtr->advance();
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
  simPtr->keyboard(key,1,x,y);
  switch(key){
  case 27:
    exit(0);
  }
}

void mouseButton(int button, int state, int x, int y){
  // translate GLUT's magic numbers to ours
  const int whichButton = 
    (button == GLUT_LEFT_BUTTON  ) ? 0:
    (button == GLUT_MIDDLE_BUTTON) ? 1 :
    (button == GLUT_RIGHT_BUTTON ) ? 2 : 0;
  const int whichState = (state == GLUT_DOWN) ? 1 : 0;
  simPtr->mouseButton(whichButton, whichState, x, y);
}

void mousePosition(int x, int y){
  simPtr->mousePosition(x,y);
}

void messageTask(void* pv){
  arSZGClient* pszgClient = (arSZGClient*)pv;
  string messageType, messageBody;
  while (true) {
    if (!pszgClient->receiveMessage(&messageType,&messageBody)){
      cout << "inputsimulator remark: shutdown.\n";
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
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  const unsigned slot = (argc > 1) ? atoi(argv[1]) : 0;
  ar_log_remark() << "inputsimulator using slot " << slot << ".\n";
  const bool useNetInput = (argc > 2) && !strcmp(argv[2], "-netinput");
  arNetInputSink netInputSink;
  if (!netInputSink.setSlot(slot)) {
    ar_log_warning() << "inputsimulator failed to set slot " << slot << ".\n";
    return 1;
  }

  // Distinguish different kinds of SZG_INPUTn services.
  netInputSink.setInfo("inputsimulator");

  inputNode = new arInputNode;
  const string pforthProgramName = szgClient.getAttribute("SZG_PFORTH", "program_names");
  if (pforthProgramName == "NULL"){
    ar_log_remark() << "inputsimulator: no pforth program for standalone joystick.\n";
  }
  else{
    const string pforthProgram = szgClient.getGlobalAttribute(pforthProgramName);
    if (pforthProgram == "NULL"){
      ar_log_remark() << "inputsimulator: no pforth program for '" <<
	   pforthProgramName << "'\n";
    }
    else{
      arPForthFilter* filter = new arPForthFilter();
      ar_PForthSetSZGClient( &szgClient );
      if (!filter->loadProgram( pforthProgram )){
        ar_log_warning() << "inputsimulator failed to configure pforth filter with program '"
	     << pforthProgram << "'.\n";
        if (!szgClient.sendInitResponse(false))
          cerr << "inputsimulator error: maybe szgserver died.\n";
        return 1;
      }
      // The input node is not responsible for clean-up
      inputNode->addFilter(filter, false);
    }
  }
  
  // simPtr defaults to &simulator, a vanilla arInputSimulator instance.
  simPtr = &defaultSimulator;
  arInputSimulatorFactory simFactory;
  arInputSimulator* simTemp = simFactory.createSimulator( szgClient );
  if (simTemp) {
    simPtr = simTemp;
  }
  simPtr->configure(szgClient);
  simPtr->registerInputNode(inputNode);
  if (useNetInput) {
    arNetInputSource* netSource = new arNetInputSource;
    if (!netSource->setSlot(slot+1)) {
      ar_log_error() << "inputsimulator failed to set slot " << slot+1 << ".\n";
      return 1;
    }
    inputNode->addInputSource(netSource,true);
    ar_log_remark() << "inputsimulator using net input, slot " << slot+1 << ".\n";
    // Memory leak.  inputNode won't free its input sources, I think.
  }
  inputNode->addInputSink(&netInputSink,false);
  
  if (!inputNode->init(szgClient)) {
    if (!szgClient.sendInitResponse(false))
      cerr << "inputsimulator error: maybe szgserver died.\n";
    return 1;
  }
  // init succeeded
  if (!szgClient.sendInitResponse(true))
    cerr << "inputsimulator error: maybe szgserver died.\n";

  if (!inputNode->start()){
    if (!szgClient.sendStartResponse(false))
      cerr << "inputsimulator error: maybe szgserver died.\n";
    return 1;
  }

  // start succeeded
  if (!szgClient.sendStartResponse(true))
    cerr << "inputsimulator error: maybe szgserver died.\n";
  
  arThread dummy(messageTask, &szgClient);
  loadParameters(szgClient);

  glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
  glutInitWindowSize(250,250);
  glutInitWindowPosition(xPos, yPos);
  glutCreateWindow("inputsimulator");
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  glutKeyboardFunc(keyboard);
  glutPassiveMotionFunc(mousePosition);
  glutMotionFunc(mousePosition);
  glutMouseFunc(mouseButton);

  glutMainLoop();
  return 0;
}
