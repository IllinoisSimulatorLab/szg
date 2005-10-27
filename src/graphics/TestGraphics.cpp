//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arGraphicsServer.h"
#include "arGraphicsPeer.h"
#include "arMesh.h"

arGraphicsServer* g;
arGraphicsPeer peer;

// Makes a simple test texture.
char* makePixels(){
  int width = 256;
  int height = 256;
  char* pixels = new char[width*height*4];
  for (int i = 0; i < height; i++){
    for (int j = 0; j < width; j++){
      pixels[4*(i*width + j)] = 128 & (j << 2) ? 255 : 0;
      pixels[4*(i*width + j)+1] = 0;
      pixels[4*(i*width + j)+2] = 0;
      pixels[4*(i*width + j)+3] = 128 & (j << 2) ? 255 : 0;
    }
  }
  return pixels;
}

void testDatabase(arGraphicsDatabase& g){
  // For drawing from szgrender, there must be a viewer node.
  arViewerNode* viewer 
    = (arViewerNode*) g.newNode(g.getRoot(), "viewer", "szg_viewer");
  arHead head;
  head.setMatrix(ar_translationMatrix(0,5,0));
  viewer->setHead(head);

  // This database tests using different point sizes and line widths, along
  // with color variations.
  arTransformNode* globalTrans 
    = (arTransformNode*) g.newNode(g.getRoot(), "transform", "global_trans");
  globalTrans->setTransform(ar_translationMatrix(0,5,-5));

  arGraphicsStateNode* s 
    = (arGraphicsStateNode*) g.newNode(globalTrans, "graphics state");
  s->setGraphicsStateFloat("point_size", 20.0);
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
  
  arGraphicsStateNode* s2 
    = (arGraphicsStateNode*) g.newNode(p, "graphics state");
  s2->setGraphicsStateFloat("line_width", 5.0);
  arIndexNode* in = (arIndexNode*) g.newNode(s2, "index");
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

  arGraphicsStateNode* s3 
    = (arGraphicsStateNode*) g.newNode(p, "graphics state");
  s3->setGraphicsStateFloat("line_width", 2.0);
  in = (arIndexNode*) g.newNode(s3, "index");
  int index2[4] = {0,2, 1,3};
  in->setIndices(4, index2);
  color = (arColor4Node*) g.newNode(in, "color4");
  float lineColor2[16] = {0,1,0,1, 0,1,0,1,
			 0,1,0,1, 0,1,0,1};
  color->setColor4(4, lineColor2);
  d = (arDrawableNode*) g.newNode(color, "drawable");
  d->setDrawable(DG_LINES, 2);

  arGraphicsStateNode* s4 
    = (arGraphicsStateNode*) g.newNode(globalTrans, "graphics state");
  s4->setGraphicsStateFloat("point_size", 10.0);
  float c2[12] = {-0.7, -0.7, 0,
	          0.7, -0.7, 0,
                  0.7, 0.7, 0,
	          -0.7, 0.7, 0};
  p = (arPointsNode*) g.newNode(s4, "points");
  p->setPoints(4, c2);
  color = (arColor4Node*) g.newNode(p, "color4");
  float clr[16] = {1,0,0,1,
		   0,1,0,1,
		   0,0,1,1,
		   0,1,1,1};
  color->setColor4(4, clr);
  d = (arDrawableNode*) g.newNode(color, "drawable");
  d->setDrawable(DG_POINTS, 4); 

  // Want to go ahead and try out the other mode of node insertion, whereby
  // the node in question makes its parent's children its own. Here we
  // are trying out the code that lets an arDatabaseNode be a node factory
  // (via it's owning database). Two additional points are attached.
  arNameNode* n = (arNameNode*) globalTrans->newNode("name");
  n->setName("foo");
  arPointsNode* additionalPoints = (arPointsNode*) n->newNode("points");
  float c3[3] = { -0.5, 0, 0 };
  additionalPoints->setPoints(1, c3);
  arDrawableNode* additionalDraw 
    = (arDrawableNode*) additionalPoints->newNode("drawable");
  additionalDraw->setDrawable(DG_POINTS, 1);
  additionalPoints = (arPointsNode*) n->newNode("points");
  float c4[3] = { 0.5, 0, 0 };
  additionalPoints->setPoints(1, c4);
  additionalDraw = (arDrawableNode*) additionalPoints->newNode("drawable");
  additionalDraw->setDrawable(DG_POINTS, 1);

  // Go ahead and attach a rectangle. We'll use this to test the texture
  // and transparency functionality. 
  arTransformNode* rt 
    = (arTransformNode*) globalTrans->newNode("transform", "rect_rot");
  rt->setTransform(ar_translationMatrix(0,0,0.1)
                   *ar_rotationMatrix('x', ar_convertToRad(-90)));
  // Turn off lighting for our rectangle since there are no lights in the
  // scene.
  arGraphicsStateNode* ls 
    = (arGraphicsStateNode*) rt->newNode("graphics state", "lighting_off");
  ls->setGraphicsStateInt("lighting", AR_G_FALSE );
  arTextureNode* tn = (arTextureNode*) ls->newNode("texture", "rect_texture");
  tn->setPixels(256, 256, makePixels(), true);
  // BUG BUG BUG BUG BUG BUG BUG BUG: Still need to call dgSetGraphicsDatabase
  dgSetGraphicsDatabase(&g);
  arRectangleMesh rect;
  rect.attachMesh("the_rect", "rect_texture");

  int count = 0;
  arGraphicsStateNode* insertedState = NULL;
  arGraphicsStateNode* insertedState2 = NULL;
  while (count <= 600){
    g.setVRCameraID(viewer->getID());
    if (count == 100){
      cout << "About to insert node.\n";
      insertedState = (arGraphicsStateNode*) g.insertNode(color, d,
							  "graphics state");
      if (!insertedState){
        cout << "Insert node failed.\n";
      }
      insertedState->setGraphicsStateFloat("point_size", 5);
      
      insertedState2 = (arGraphicsStateNode*) g.insertNode(n, NULL,
							   "graphics state");
      insertedState2->setGraphicsStateFloat("point_size", 10);
    }
    if (count == 200){
      cout << "About to cut node.\n";
      g.cutNode(insertedState->getID());
      g.cutNode(insertedState2->getID());
    }
    list<arDatabaseNode*> IDs;
    if (count == 300){
      cout << "testgraphics remark: about to permute nodes.\n";
      // s, s4, n, rt are the initial children for the global trans node.
      IDs.push_back(rt);
      IDs.push_back(s);
      IDs.push_back(s4);
      IDs.push_back(n);
      g.permuteChildren(globalTrans, IDs);
    }
    if (count == 400){
      // s, s4, n, rt are the initial children for the global trans node.
      // Return to the original order.
      cout << "testgraphics remark: about to permute again.\n";
      IDs.push_back(s);
      IDs.push_back(s4);
      IDs.push_back(n);
      IDs.push_back(rt);
      g.permuteChildren(globalTrans, IDs);
    }
    count++;
    ar_usleep(10000);
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
  // It turns out to not be *that* expensive to create and delete a graphics
  // context. Here we are checking that.
  double t = ar_difftime(ar_time(), time1);
  cout << "Average time to create/delete graphics context = "
       << t/num << " microseconds.\n";

  // Test the reference counting on texture objects.
  arTexture* tex1 = new arTexture();
  arTexture* tex2 = tex1->ref();
  tex1->unref(true);
  cout << "The second texture (ref) has width = " << tex2->getWidth() << "\n";
  tex2->unref(true);

  arSZGClient client;
  client.init(argc, argv);
  if (!client){
    // Failed to connect to szgserver. That's it since we are testing
    // the distributed graphics as well.
    cout << "TestGraphics error: szgserver must be running and user must "
	 << "be dlogin'ed.\n";
    return 1;
  }
  g = new arGraphicsServer();
  if (!g->init(client)){
    cout << "TestGraphics error: graphics server failed to init.\n";
    return 1;
  }
  if (!g->start()){
    cout << "TestGraphics error: graphics server failed to start.\n";
    return 1;
  }
  testDatabase(*g);
  // PLEASE NOTE: it is actually UNSAFE to delete an arDataServer object that
  // is currently receiving connections. Consequently, these objects should
  // never be declared as static globals in an szg application.
 
  peer.setName("test_graphics");
  if (!peer.init(client)){
    cout << "TestGraphics error: could not initialize graphics peer.\n";
    return 1;
  }
  if (!peer.start()){
    cout << "TestGraphics error: could not start graphics peer.\n";
    return 1;
  }
  if (peer.connectToPeer("target")<0){
    cout << "TestGraphics error: peer named \"target\" does not exist.\n";
    peer.closeAllAndReset();;
    return 1;
  }
  peer.pullSerial("target",0,0,
                  AR_TRANSIENT_NODE,AR_TRANSIENT_NODE,AR_TRANSIENT_NODE);
  peer.reset();
  // sleep for 2 seconds.
  ar_usleep(2000000);
  testDatabase(peer);
  cout << "testgraphics remark: about to stop peer.\n";
  peer.closeAllAndReset();
  // NOTE: arGraphicsPeer::stop() should not be used here. Really all it does
  // is closeAllAndReset followed by closing the arSZGClient connection.
  cout << "testgraphics remark: peer has stopped.\n";
}
