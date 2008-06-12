//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arObjectUtilities.h"
#include "arGraphicsServer.h"
#include "arSZGClient.h"
#include "arGraphicsAPI.h"
#include "arGraphicsHeader.h"
#include "arMesh.h"
#include "arOrthoCamera.h"

#include "arGraphicsWindow.h"
#include "arVRCamera.h"
#include "arOrthoCamera.h"
#include "arPerspectiveCamera.h"

#include "arGUIInfo.h"
#include "arGUIWindowManager.h"
#include "arGUIWindow.h"
#include "arTexFont.h"

#include "arXMLParser.h"
#include "arGUIXMLParser.h"

arMatrix4 mouseWorldMatrix;
arGraphicsDatabase* theDatabase = NULL;

enum { PAN=0, ROTATE, ZOOM, SLIDER, NONE };

int mouseTransformID = -1;
int mouseManipState = ROTATE;
int oldState = ROTATE;

bool isPlaying = false;
bool isModelSpinning = false;

float bgColor = 0.;
float _ratio = 1.;
float frameRate = 0.;

int sliderCenterX = -50;
int sliderCenterY = 10; // slider position in pixels

arObject* theObject = NULL;
string arObject::type() const { return "Invalid"; } // avoid warning on darwin / gcc 4.0.1
arTexFont texFont;
arGUIWindowManager* wm = NULL;
arHead head; // not from a tracker

// To use arGraphicsWindow::draw in the arGUIWindow as the draw callback
class arSZGViewRenderCallback : public arGUIRenderCallback
{
  public:
    arSZGViewRenderCallback( void ) {}
    virtual ~arSZGViewRenderCallback( void ) {}
    virtual void operator()( arGraphicsWindow&, arViewport& ) {}
    virtual void operator()( arGUIWindowInfo* ) {}
    virtual void operator()( arGUIWindowInfo* /*wI*/, arGraphicsWindow* gW ) {
      if ( gW ) {
#ifdef UNUSED
        arVector3 size(wm->getWindowSize(wI->getWindowID()));
#endif
        gW->draw();
      }
    }
};

// Drawing functions

void drawHUD()
{
  // menu text
  const arVector3 highlight(1, 1, 1);
  const arVector3 other(0.6, 0.6, 0.6);
  arTextBox box;
  box.width = 45;
  box.color = arVector3(1, 0, 0);
  box.columns = 15;
  box.color = (oldState == PAN) ?  highlight : other;
  box.upperLeft = arVector3(0, 100, -0.1);
  texFont.renderString( "[1] Translate", box);
  box.color = (oldState == ROTATE) ?  highlight : other;
  box.upperLeft = arVector3(0, 95, -0.1);
  texFont.renderString( "[2] Rotate", box);
  box.color = (oldState == ZOOM) ?  highlight : other;
  box.upperLeft = arVector3(0, 90, -0.1);
  texFont.renderString( "[3] Scale", box);

  if (!theObject->supportsAnimation() || theObject->numberOfFrames()<=0)
    return;

  // animation slider
  glColor3f( 0.9f, 0.9f, 0.9f );
  glBegin( GL_QUADS );  // draw box for slider et. al.
  glVertex2f( 100., 0. );
  glVertex2f( 100., 5. );
  glVertex2f( 0.,   5. );
  glVertex2f( 0.,   0.);
  glEnd();

  const float numFrames = theObject->numberOfFrames();
  const float sliderStart = 5.;
  const float sliderEnd   = 85.;
  const float sliderWidth = sliderEnd - sliderStart;
  const float sliderMaxY  = 4.;
  const float sliderMinY = 1.;
  float sliderSpacing = sliderWidth;

  glColor3f( 0.25f, 0.25f, 0.25f);
  glBegin( GL_LINES );  // draw slider bar
  glVertex2f( sliderStart, ( sliderMinY + sliderMaxY ) / 2. );
  glVertex2f( sliderEnd,   ( sliderMinY + sliderMaxY ) / 2. );

  // make each tick mark at least 10 px across onscreen, and geometric series
  // todo: once per window resize, not once per frame
  float stepSize = 1.;
  while ((sliderSpacing = sliderWidth / stepSize) > 10.)
    stepSize *= (int(stepSize) % 2 == 0) ? 2.5 : 2.0;

  // tick marks
  for( float j = 0.; j <= sliderWidth; j += sliderSpacing ) {
    glVertex2f( sliderStart + j, sliderMaxY );
    glVertex2f( sliderStart + j, sliderMinY );
  }
  glVertex2f( sliderEnd, sliderMaxY );
  glVertex2f( sliderEnd, sliderMinY );
  glEnd();

  const float sliderCenterX =
    sliderStart + sliderWidth * float(theObject->currentFrame()) / numFrames;

  glColor3f(0.5f, 0.5f, 0.5f );
  glBegin( GL_QUADS );  // draw slider position
  glVertex2f( sliderCenterX - 0.5f, sliderMaxY + 0.5f );
  glVertex2f( sliderCenterX - 0.5f, sliderMinY - 0.5f );
  glVertex2f( sliderCenterX + 0.5f, sliderMinY - 0.5f );
  glVertex2f( sliderCenterX + 0.5f, sliderMaxY + 0.5f );
  glEnd();

  char displayString[10];
  sprintf( displayString, "%4i", theObject->currentFrame() );
  box.columns = 4;
  box.width = 15;
  box.color = arVector3(0, 0, 0);
  box.upperLeft = arVector3(85, 5, 0);
  texFont.renderString( displayString, box);
}

void display( arGUIWindowInfo*, arGraphicsWindow* )
{
  // loop animation
  if ( isPlaying ) {
    if ( !theObject->nextFrame() ) {
      theObject->setFrame( 0 );
    }
  }

  if ( isModelSpinning ) {
    mouseWorldMatrix = ar_rotationMatrix( arVector3( 0., 1., 0. ), 0.001 ) *
    		       ar_rotationMatrix( arVector3( 1., 0., 0. ), 0.002 ) *
	    	       mouseWorldMatrix;
    dgTransform( mouseTransformID, mouseWorldMatrix );
  }

  glEnable( GL_DEPTH_TEST );
  glEnable( GL_LIGHTING );
  theDatabase->activateLights();
  theDatabase->draw();

  // draw info screen
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  glOrtho( 0, 100, 0, 100, 0, 100);

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glDisable( GL_LIGHTING );
  glDisable( GL_DEPTH_TEST );
  glColor3f( 1., 1., 1. );
  drawHUD();

}

void keyboardCB( arGUIKeyInfo* keyInfo )
{
  if ( !keyInfo ) {
    cerr << "NULL keyInfo in keyboardCB!" << endl;
    return;
  }

  // only perform actions on key release, otherwise everything is done
  // twice, once on press and once on release
  if ( keyInfo->getState() == AR_KEY_DOWN ) {
    return;
  }

  switch( keyInfo->getKey() ) {
    case AR_VK_ESC:
      exit( 0 );
    break;

    case AR_VK_f:
      wm->fullscreenWindow( keyInfo->getWindowID() );
    break;

    case AR_VK_F:
      wm->resizeWindow( keyInfo->getWindowID(), 640, 480 );
    break;

    case AR_VK_0:
      isModelSpinning = !isModelSpinning;
    break;

    case AR_VK_1:
      mouseManipState = oldState = PAN;
    break;

    case AR_VK_2:
      mouseManipState = oldState = ROTATE;
    break;

    case AR_VK_3:
      mouseManipState = oldState = ZOOM;
    break;

    case AR_VK_APOST:
      bgColor = ( bgColor > 0.99f ? 0. : bgColor + 0.25f );
      glClearColor( bgColor, bgColor, bgColor, 1. );
    break;

    case AR_VK_d:
    case AR_VK_o:
      theDatabase->printStructure();
    break;

    case AR_VK_p:
      theDatabase->printStructure( 9 );
    break;

    default:
    break;
  }

  // animation functions
  if ( theObject->supportsAnimation() ) {
    switch( keyInfo->getKey() ) {
      case AR_VK_j:
        theObject->prevFrame();
      break;

      case AR_VK_k:
        isPlaying ^= 1;
      break;

      case AR_VK_l:
        theObject->nextFrame();
      break;

      default:
      break;
    }
  }
}

void windowCB( arGUIWindowInfo* wI )
{
  if ( !wI ) {
    cerr << "NULL windowInfo in windowCB!" << endl;
    return;
  }

  switch( wI->getState() ) {
    case AR_WINDOW_RESIZE:
      wm->setWindowViewport( wI->getWindowID(), 0, 0, wI->getSizeX(), wI->getSizeY() );
    break;

    case AR_WINDOW_CLOSE:
      wm->deleteWindow( wI->getWindowID() );

      if ( !wm->hasActiveWindows() ) {
        exit( 0 );
      }
    break;

    default:
    break;
  }
}

void mouseCB( arGUIMouseInfo* mI )
{
  const arVector3 size(wm->getWindowSize( mI->getWindowID() ));
  const int width = int( size.v[ 0 ] );
  const int height = int( size.v[ 1 ] );

  if ( mI->getState() == AR_MOUSE_DRAG ) {
    const float deltaX = float(mI->getPosX() - mI->getPrevPosX());
    const float deltaY = float(mI->getPosY() - mI->getPrevPosY());

    switch( mouseManipState ) {
      case SLIDER:
      {
        arGUIMouseInfo tMouseInfo( AR_MOUSE_EVENT, AR_GENERIC_STATE,
	  -1, 0, AR_LBUTTON, mI->getPosX(), mI->getPosY(),
	  mI->getPrevPosX(), mI->getPrevPosY() );
        mouseCB( &tMouseInfo );
        return;
      }
      break;

      case PAN:
        mouseWorldMatrix = ar_TM( deltaX/100., deltaY/-100., 0. ) * mouseWorldMatrix;
      break;

      case ROTATE:
      {
        arVector3 rotationAxis(arVector3( 0., 0., 1. ) * arVector3( deltaX, deltaY, 0. ));

        if ( ++rotationAxis > 0 ) {
          float mag = ++rotationAxis;
          rotationAxis /= mag;
          mouseWorldMatrix = ar_ETM( mouseWorldMatrix ) *
   	    ar_RM( arVector3( -1., 0., 0. ), -deltaY / 300. ) *
   	    ar_RM( arVector3( 0., 1., 0. ), deltaX / 300. ) *
  	    ar_ERM( mouseWorldMatrix ) * ar_ESM( mouseWorldMatrix );
        }
      break;
      }

      case ZOOM:
        mouseWorldMatrix = mouseWorldMatrix * ar_SM( 1. + deltaX/100. );
      break;

      default:
      break;
    }

    dgTransform( mouseTransformID, mouseWorldMatrix );
  }

  // the rest is only applicable when the model supports animation
  if ( mI->getButton() != AR_LBUTTON || !theObject->supportsAnimation() ||
      ( theObject->numberOfFrames() < 1 ) ) {
    return;
  }

  // mouse down on the slider bar
  if ( mI->getPosY() > height - 20 && mI->getState() == AR_MOUSE_DOWN ) {
    oldState = mouseManipState;
    mouseManipState = ( mI->getPosX() <= width-57 && mI->getPosX() >= 20 ) ? SLIDER : NONE;
  }

  if ( mouseManipState == SLIDER ) { // we're dragging
    if ( mI->getPosX() < 20 ) {
      theObject->setFrame( 0 );
    }
    else if ( mI->getPosX() > width-57 ) {
      theObject->setFrame( theObject->numberOfFrames() - 1 );
    }
    else {
      theObject->setFrame( int( float( mI->getPosX()-20 ) / float( width-77 ) *
                                float( theObject->numberOfFrames() ) ) );
	  }
  }

  if ( mI->getState() == AR_MOUSE_UP ) {
    mouseManipState = oldState;	// done dragging bar
  }
}

int main( int argc, char** argv ) {
  string fileName;
  if (argc > 1) {
    fileName = string(argv[1]);
  }
  else {
    // Use local config file.
    FILE* configFile = fopen("szgview.txt", "r");
    if (!configFile) {
      cout << "szgview error: no parameter supplied and no szgview.txt config file.\n";
      cout << "usage: " << argv[ 0 ] << " file.{obj|3ds|htr|htr2} [mesh.obj]\n";
      return 1;
    }

    char buf[1024];
    if (fscanf(configFile, "%s", buf) < 1) {
      cout << "szgview error: config file szgview.txt is empty.\n";
      return 1;
    }

    fileName = string(buf);
  }

  theObject = ar_readObjectFromFile( fileName, "" );
  if ( !theObject ) {
    cerr << "Invalid File: " << fileName << endl;
    return 1;
  }

  arObject* theMesh = argc==3 ? ar_readObjectFromFile( argv[2], "" ) : NULL;

  arSZGClient SZGClient;
  SZGClient.init( argc, argv );
  if ( !SZGClient ) {
    // init() may fail.
    cout << "running standalone.\n";
  }
  const string textPath = SZGClient.getAttribute("SZG_RENDER", "text_path");
  theDatabase = new arGraphicsDatabase;
  char texPath[ 256 ] = {0};
#ifndef AR_USE_WIN_32
    getcwd( texPath, 256 );
#endif
  theDatabase->setTexturePath( string( texPath ) );

  arMatrix4 worldMatrix( ar_translationMatrix( 0.0, 5.0, -5.0 ) );

  dgSetGraphicsDatabase( theDatabase );
  dgTransform( "world", "root", worldMatrix );
  dgTransform( "mouse", "world", mouseWorldMatrix );
  mouseTransformID = theDatabase->getNodeID( "mouse" );

  // Add some lights.
  (void) dgLight( "light0", "root", 0, arVector4(0, 1, 0, 0), arVector3( 1, 1, 1 ) );
  (void) dgLight( "light0", "root", 1, arVector4(0, -1, 0, 0), arVector3( 1, 1, 1 ) );
  (void) dgLight( "light0", "root", 2, arVector4(0, 0, 1, 0), arVector3( 1, 1, 1 ) );
  theObject->normalizeModelSize();    // fits into unit sphere

  if ( theObject->type() == "HTR" ) {
    ((arHTR*) theObject)->basicDataSmoothing();
    if ( argc == 3 ) {
      ar_mergeOBJandHTR( (arOBJ*) theMesh, (arHTR*) theObject, "mouse" );
    }
    else {
      ((arHTR*) theObject)->attachMesh( "object", "mouse", true );
    }
  }
  else {
    theObject->attachMesh( "object", "mouse" );
  }

  // theDatabase->prettyDump(); // for debugging

  // FIXME: The window manager is *always* single threaded here.
  // Bad for a host with multiple graphics cards.
  wm = new arGUIWindowManager( windowCB, keyboardCB, mouseCB, NULL, false );

  head.setMatrix( ar_translationMatrix( 0.0, 5.0, 0.0 ) );

  const string whichDisplay = SZGClient.getMode( "graphics" );
  const string displayName = SZGClient.getAttribute( whichDisplay, "name" );

  arGUIXMLParser guiXMLParser( &SZGClient, SZGClient.getGlobalAttribute( displayName ) );

  if ( guiXMLParser.parse() < 0 ) {
    return -1;
  }

  const vector< arGUIXMLWindowConstruct* >* windowConstructs = guiXMLParser.getWindowingConstruct()->getWindowConstructs();

  // Populate callbacks for the gui and graphics windows,
  // and the head for any vr cameras.
  vector< arGUIXMLWindowConstruct*>::const_iterator itr;
  for( itr = windowConstructs->begin(); itr != windowConstructs->end(); ++itr ) {
    (*itr)->getGraphicsWindow()->setInitCallback( new arDefaultWindowInitCallback() );
    (*itr)->getGraphicsWindow()->setDrawCallback( new arDefaultGUIRenderCallback( display ) );
    (*itr)->setGUIDrawCallback( new arSZGViewRenderCallback() );

    vector<arViewport>* viewports = (*itr)->getGraphicsWindow()->getViewports();
    vector<arViewport>::iterator vItr;
    for( vItr = viewports->begin(); vItr != viewports->end(); vItr++ ) {
      if ( vItr->getCamera()->type() == "arVRCamera" ) {
        ((arVRCamera*) vItr->getCamera())->setHead( &head );
      }
    }
  }

  if ( wm->createWindows( guiXMLParser.getWindowingConstruct() ) < 0 ) {
    return -1;
  }

  string fontLocation = ar_fileFind( "courier-bold.ppm", "", textPath );
  if ( fontLocation != "NULL" ) {
    ar_log_remark() << "szgview found szg system font.\n";
  }
  else {
    fontLocation = "courier-bold.txf";
  }
  texFont.load( fontLocation ); // After opengl context is created.

  wm->startWithSwap();
  return 0;
}
