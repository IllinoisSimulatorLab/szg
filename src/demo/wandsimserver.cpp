//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsHeader.h"
#include "arHeadWandSimulator.h"
#include "arInputNode.h"
#include "arNetInputSink.h"
#include "arGenericDriver.h"
#include "arPForthFilter.h"


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

int main(int argc, char** argv){
  arSZGClient szgClient;
  szgClient.simpleHandshaking(false);
  szgClient.init(argc, argv);  
  if (!szgClient)
    return 1;

  arNetInputSink netInputSink;
  netInputSink.setSlot(0);
  arPForthFilter testFilter;
  if (!testFilter.configure(&szgClient)){
    szgClient.sendInitResponse(false);
    return 1;
  }
  arInputNode inputNode;
  simulator.registerInputNode(&inputNode);
  inputNode.addInputSink(&netInputSink,false);
  inputNode.addFilter(&testFilter, true);
  if (!inputNode.init(szgClient)) {
    szgClient.sendInitResponse(false);
    return 1;
  }
  // initialization succeeded
  szgClient.sendInitResponse(true);

  if (!inputNode.start()){
    szgClient.sendStartResponse(false);
    return 1;
  }
  
  arThread dummy(ar_messageTask, &szgClient);

  loadParameters(szgClient);

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
  szgClient.sendStartResponse(true);
  glutMainLoop();
}
