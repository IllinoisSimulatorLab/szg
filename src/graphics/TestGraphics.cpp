//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arGraphicsServer.h"

arGraphicsServer g;

void display(){
  glClearColor(0,0,0,0);
  glEnable(GL_DEPTH_TEST);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(-2,2, -2,2, 0.1, 100);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  gluLookAt(0,0,2, 0,0,0, 0,1,0);
  g.draw();
  glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y){
  switch(key){
  case 27:
    exit(0);
    break;
  }
}

int main(int argc, char** argv){
  ar_timeval time1 = ar_time();
  int i;
  int num = 100000;
  for (i=0; i<num; i++){
    arGraphicsContext* g = new arGraphicsContext();
    delete g;
  }
  double t = ar_difftime(ar_time(), time1);
  cout << "Average time to create/delete graphics context = "
       << t/num << " microseconds.\n";

  // For drawing from szgrender, there must be a viewer node.
  arViewerNode* viewer 
    = (arViewerNode*) g.newNode(g.getRoot(), "viewer", "szg_viewer");
  arHead head;
  head.setMatrix(ar_translationMatrix(0,5,0));
  viewer->setHead(head);

  // This database tests using different point sizes and line widths, along
  // with color variations.
  arTransformNode* globalTrans 
    = (arTransformNode*) g.newNode(g.getRoot(), "transform");
  globalTrans->setTransform(ar_translationMatrix(0,5,-5));
  arGraphicsStateNode* s 
    = (arGraphicsStateNode*) g.newNode(globalTrans, "graphics state");
  s->setGraphicsState("point_size", NULL, 20.0);
  float c[12] = {-1, -1, 0, 
                 1, -1, 0,
		 1, 1, 0,
		 -1, 1, 0};
  arMaterialNode* m = (arMaterialNode*) g.newNode(s, "material");
  arMaterial mat;
  mat.diffuse = arVector3(1,1,0);
  m->setMaterial(mat);
  arPointsNode* p = (arPointsNode*) g.newNode(m, "points");
  p->setPoints(4, c);
  arDrawableNode* d = (arDrawableNode*) g.newNode(p, "drawable");
  d->setDrawable(DG_POINTS, 4); 
  
  s = (arGraphicsStateNode*) g.newNode(p, "graphics state");
  s->setGraphicsState("line_width", NULL, 5.0);
  arIndexNode* in = (arIndexNode*) g.newNode(s, "index");
  int index[8] = {0,1, 1,2, 2,3, 3,0};
  in->setIndices(8, index);
  arColor4Node* color = (arColor4Node*) g.newNode(in, "color4");
  float lineColor[32] = {1,0,0,1, 1,0,0,1,
			 1,0,0,1, 1,0,0,1,
			 1,0,0,1, 1,0,0,1,
			 1,0,0,1, 1,0,0,1};
  color->setColor4(8, lineColor);
  d = (arDrawableNode*) g.newNode(color, "drawable");
  d->setDrawable(DG_LINES, 4);

  s = (arGraphicsStateNode*) g.newNode(p, "graphics state");
  s->setGraphicsState("line_width", NULL, 2.0);
  in = (arIndexNode*) g.newNode(s, "index");
  int index2[4] = {0,2, 1,3};
  in->setIndices(4, index2);
  color = (arColor4Node*) g.newNode(in, "color4");
  float lineColor2[16] = {0,1,0,1, 0,1,0,1,
			 0,1,0,1, 0,1,0,1};
  color->setColor4(4, lineColor2);
  d = (arDrawableNode*) g.newNode(color, "drawable");
  d->setDrawable(DG_LINES, 2);

  s = (arGraphicsStateNode*) g.newNode(globalTrans, "graphics state");
  s->setGraphicsState("point_size", NULL, 10.0);
  float c2[12] = {-0.7, -0.7, 0,
	          0.7, -0.7, 0,
                  0.7, 0.7, 0,
	          -0.7, 0.7, 0};
  p = (arPointsNode*) g.newNode(s, "points");
  p->setPoints(4, c2);
  color = (arColor4Node*) g.newNode(p, "color4");
  float clr[16] = {1,0,0,1,
		   0,1,0,1,
		   0,0,1,1,
		   0,1,1,1};
  color->setColor4(4, clr);
  d = (arDrawableNode*) g.newNode(color, "drawable");
  d->setDrawable(DG_POINTS, 4); 

  arSZGClient client;
  client.init(argc, argv);
  if (!client){
    // Failed to connect to szgserver. That's it since we are testing
    // the distributed graphics as well.
    cout << "TestGraphics error: szgserver must be running and user must "
	 << "be dlogin'ed.\n";
    return 1;
  }
  if (!g.init(client)){
    cout << "TestGraphics error: graphics server failed to init.\n";
    return 1;
  }
  if (!g.start()){
    cout << "TestGraphics error: graphics server failed to start.\n";
    return 1;
  }
  int count = 0;
  arGraphicsStateNode* insertedState;
  while (true){
    g.setVRCameraID(viewer->getID());
    if (count == 100){
      insertedState = (arGraphicsStateNode*) g.insertNode(color, d,
							  "graphics state");
      insertedState->setGraphicsState("point_size", NULL, 5);
      g.ps();
    }
    if (count == 200){
      g.cutNode(insertedState->getID());
      g.ps();
      count = 0;
    }
    count++;
    ar_usleep(10000);
  }

  /*glutInit(&argc,argv);
  glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB|GLUT_DEPTH);
  glutInitWindowSize(600,600);
  glutInitWindowPosition(0, 0);
  glutCreateWindow("szg graphics test");
  glutDisplayFunc(display);
  glutIdleFunc(display);
  glutKeyboardFunc(keyboard);

  glutMainLoop();*/
}
