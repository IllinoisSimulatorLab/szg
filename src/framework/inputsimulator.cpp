//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arGraphicsHeader.h"
#include "arInputSimulator.h"
#include "arInputNode.h"
#include "arNetInputSource.h"
#include "arNetInputSink.h"
#include "arGenericDriver.h"
#include "arPForthFilter.h"

arInputNode* inputNode = NULL;

arInputSimulator simulator;
int xPos = 0, yPos = 0;

void loadParameters(arSZGClient& szgClient){
  int posBuffer[2];
  const string posString = szgClient.getAttribute("SZG_INPUTSIM", "position");
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
  ar_usleep(20000); // Mild CPU throttle
}

void keyboard(unsigned char key, int x, int y){
  simulator.keyboard(key,1,x,y);
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
  simulator.mouseButton(whichButton, whichState, x, y);
}

void mousePosition(int x, int y){
  simulator.mousePosition(x,y);
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
  inputNode = new arInputNode;
  szgClient.simpleHandshaking(false);
  szgClient.init(argc, argv);  
  if (!szgClient)
    return 1;

  ar_log().setStream(szgClient.initResponse());
  const int slotNumber = (argc > 1) ? atoi(argv[1]) : 0;
  ar_log_remark() << "inputsimulator using slot " << slotNumber << ".\n";
  const bool useNetInput = (argc > 2) && !strcmp(argv[2], "-netinput");
  arNetInputSink netInputSink;
  if (!netInputSink.setSlot(slotNumber)) {
    ar_log_warning() << "inputsimulator failed to set slot " << slotNumber << ".\n";
    return 1;
  }

  // Distinguish different kinds of SZG_INPUTn services.
  netInputSink.setInfo("inputsimulator");

  const string pforthProgramName = szgClient.getAttribute("SZG_PFORTH", "program_names");
  if (pforthProgramName == "NULL"){
    ar_log_remark() << "inputsimulator: no pforth program for standalone joystick.\n";
  }
  else{
    string pforthProgram = szgClient.getGlobalAttribute(pforthProgramName);
    if (pforthProgram == "NULL"){
      ar_log_remark() << "inputsimulator: no pforth program exists for name '" <<
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
  
  simulator.configure(szgClient);
  simulator.registerInputNode(inputNode);
  if (useNetInput) {
    arNetInputSource* netSource = new arNetInputSource;
    if (!netSource->setSlot(slotNumber+1)) {
      ar_log_error() << "inputsimulator failed to set slot " << slotNumber+1 << ".\n";
      return 1;
    }
    inputNode->addInputSource(netSource,true);
    ar_log_remark() << "inputsimulator using net input, slot " 
         << slotNumber+1 << ".\n";
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

  ar_log().setStream(szgClient.startResponse());
  if (!inputNode->start()){
    if (!szgClient.sendStartResponse(false))
      cerr << "inputsimulator error: maybe szgserver died.\n";
    return 1;
  }

  // start succeeded
  if (!szgClient.sendStartResponse(true))
    cerr << "inputsimulator error: maybe szgserver died.\n";
  ar_log().setStream(cout);
  
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
