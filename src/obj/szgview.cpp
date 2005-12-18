//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
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

#include <iostream>
#include <string>
#include <vector>

arMatrix4 mouseWorldMatrix = arMatrix4();

arGraphicsDatabase* theDatabase = NULL;

enum { PAN=0, ROTATE, ZOOM, SLIDER, NONE };

int mouseTransformID;
int mouseManipState = ROTATE, oldState = ROTATE;
bool isPlaying = false, isModelSpinning = false;
float bgColor = 0.0f, _ratio = 1.0f, frameRate = 0.0f;

int _width = 0, _height = 0;
int sliderCenterX = -50, sliderCenterY = 10; // slider position in pixels

arObject *theObject = NULL;

arTexFont texFont;

arGUIWindowManager* wm = NULL;

// since we don't get this from a tracker just make a sensible default one
arHead head;

// forward declaration
void display( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow );

// To be able to use arGraphicsWindow::draw in the arGUIWindow as the draw
// callback
class arSZGViewRenderCallback : public arGUIRenderCallback
{
  public:

    arSZGViewRenderCallback( void ) { }

    virtual ~arSZGViewRenderCallback( void )  { }

    virtual void operator()( arGraphicsWindow&, arViewport& ) { }

    virtual void operator()( arGUIWindowInfo* windowInfo ) { }

    virtual void operator()( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow ) {
      if( graphicsWindow ) {
        // HACK because the width/height could be different between windows but
        // because we can't pass anything to the graphicswindow draw the
        // globals need to be set here
        arVector3 size = wm->getWindowSize( windowInfo->getWindowID() );
        _width =  int( size[ 0 ] );
        _height = int( size[ 1 ] );

        graphicsWindow->draw();
      }
    }

  private:

};


// Drawing functions

void showRasterString( float x, float y, char* s )
{
  glRasterPos2f( x, y );
  for (char c; (c = *s) != '\0'; ++s)
    glutBitmapCharacter( GLUT_BITMAP_9_BY_15, c );
}

void drawHUD( int width, int height )
{
  char displayString[ 128 ] = { 0 };

  glColor3f( 0.9f, 0.9f, 0.9f );
  glBegin( GL_QUADS );  // draw box to hold text
    glVertex2f( 0.0f, 100.0f );
    glVertex2f( 0.0f, 100.0f * (1.0f - 20.0f / float( height ) ) );
    glVertex2f( 100.0f, 100.0f * ( 1.0f - 20.0f / float( height ) ) );
    glVertex2f( 100.0f, 100.0f );
  glEnd();

  float startPos = 0, endPos = 0;
  switch( oldState ) {
    case PAN:
     startPos = (         0.0f ) * 100.0f / float( width );
		 endPos   = ( 15.0f * 9.0f ) * 100.0f / float( width );
    break;

    case ROTATE:
     startPos = ( 16.0f * 9.0f ) * 100.0f / float( width );
		 endPos   = ( 28.0f * 9.0f ) * 100.0f / float( width );
    break;

    case ZOOM:
     startPos = ( 29.0f * 9.0f ) * 100.0f / float( width );
		 endPos   = ( 40.0f * 9.0f ) * 100.0f / float( width );
    break;
  };

  glColor3f( 0.75f, 0.75f, 0.80f );
  glBegin( GL_QUADS );  // draw highlighted box
    glVertex2f( startPos, 100.0f );
    glVertex2f( startPos, 100.0f * ( 1.0f - 20.0f / float( height ) ) );
    glVertex2f( endPos,   100.0f * ( 1.0f - 20.0f / float( height ) ) );
    glVertex2f( endPos,   100.0f );
  glEnd();

  // menu text
  glColor3f( 1.0f, 0.0f, 0.0f );
  sprintf( displayString, " [1] Translate  [2] Rotate  [3] Scale" );
  // showRasterString( 0.0f, 100.0f * ( 1.0f - 15.0f / float( height) ), displayString );
  texFont.renderString2D( displayString, 0.0f, 0.96875f, 0.5625f, 0.041666f );
  // texFont.renderString2D( displayString, 0.0f, 0.96875f, 1.0f / 66.0f, 0.041666f, true );

  // animation slider
  if( theObject->supportsAnimation() && ( theObject->numberOfFrames() > 0 ) ) {
    glColor3f( 0.9f, 0.9f, 0.9f );
    glBegin( GL_QUADS );  // draw box for slider et. al.
      glVertex2f( 100.0f, 0.0f );
      glVertex2f( 100.0f, 100.0f * (20.0f / float( height ) ) );
      glVertex2f( 0.0f,   100.0f * (20.0f / float( height ) ) );
      glVertex2f( 0.0f,   0.0f);
    glEnd();

    glColor3f( 1.0f, 1.0f, 1.0f );
    glBegin( GL_QUADS );  // draw box for current frame number
      glVertex2f( 100.0f - 2.0f * 100.0f / float( width ), 100.0f * 02.0f / float( height ) );
      glVertex2f( 100.0f - 2.0f * 100.0f / float( width ), 100.0f * 18.0f / float( height ) );
      glVertex2f( 100.0f - ( 4.0f + 4.0f * 9.0f ) * 100.0f / float( width ), 100.0f * 20.0f / float( height ) );
      glVertex2f( 100.0f - ( 4.0f + 4.0f * 9.0f ) * 100.0f / float( width ), 100.0f * 02.0f / float( height ) );
    glEnd();

    float numFrames = theObject->numberOfFrames();
    float sliderStart = 2.0f * 9.0f * 100.0f / float( width );
    float sliderEnd   = 100.0f - ( 4.0f + 6.0f * 9.0f ) * 100.0f / float( width );
    float sliderWidth = sliderEnd - sliderStart;
    float sliderMaxY  = 100.0f * 15.0f / float( height ), sliderMinY = 100.0f * 5.0f / float( height );
    float sliderSpacing = sliderWidth;

    glColor3f( 0.25f, 0.25f, 0.25f);
    glBegin( GL_LINES );  // draw slider bar
      glVertex2f( sliderStart, ( sliderMinY + sliderMaxY ) / 2.0f );
      glVertex2f( sliderEnd,   ( sliderMinY + sliderMaxY ) / 2.0f );

      /// \todo Only do this once per window resize isntead of every frame
      float stepSize = 1.0f;
      // make each tick mark at least 10 px across onscreen, and geometric series
      for( ; ; ) {
        sliderSpacing = ( sliderWidth * float( width ) / 100.0f ) / stepSize;

        if( sliderSpacing <= 10.0f ) // && sliderSpacing <= 20.)
	        break;
        else if( int( stepSize ) % 2 == 0 )
	        stepSize *= 2.5f;
        else
	        stepSize *= 2.0f;
      }

      // draw tick marks
      for( float j = 0.0f; j <= sliderWidth; j += sliderSpacing ) {
        glVertex2f( sliderStart + j, sliderMaxY );
        glVertex2f( sliderStart + j, sliderMinY );
      }

      glVertex2f( sliderEnd, sliderMaxY );
      glVertex2f( sliderEnd, sliderMinY );
    glEnd();

    float sliderCenter = ( sliderStart + float( theObject->currentFrame() ) / numFrames * sliderWidth );
    sliderCenterX = int( width * sliderCenter / 100.0f ); // for the mouse func

    glColor3f(0.5f, 0.5f, 0.5f );
    glBegin( GL_QUADS );  // draw slider position
      glVertex2f( sliderCenter - 5.0f * 100.0f / float( width ), sliderMaxY + 4.0f * 100.0f / float( height ) );
      glVertex2f( sliderCenter - 5.0f * 100.0f / float( width ), sliderMinY - 4.0f * 100.0f / float( height ) );
      glVertex2f( sliderCenter + 5.0f * 100.0f / float( width ), sliderMinY - 4.0f * 100.0f / float( height ) );
      glVertex2f( sliderCenter + 5.0f * 100.0f / float( width ), sliderMaxY + 4.0f * 100.0f / float( height ) );
    glEnd();

    glColor3f( 0.0f, 0.0f, 0.0f );
    sprintf( displayString, "%4i", theObject->currentFrame() );
    showRasterString( 100.0f - ( 2.0f + 4.0f * 9.0f ) * 100.0f / float( width ),
                      100.0f * 2.0f / float( height ), displayString );
  }
}

void display( arGUIWindowInfo* windowInfo, arGraphicsWindow* graphicsWindow )
{
  // loop animation
  if( isPlaying ) {
    if( !theObject->nextFrame() ) {
      theObject->setFrame( 0 );
    }
  }

  if( isModelSpinning ) {
    mouseWorldMatrix = ar_rotationMatrix( arVector3( 0.0f, 1.0f, 0.0f ), 0.001f ) *
    			             ar_rotationMatrix( arVector3( 1.0f, 0.0f, 0.0f ), 0.002f ) *
	    		             mouseWorldMatrix;
    dgTransform( mouseTransformID, mouseWorldMatrix );
  }

  /*
  arPerspectiveCamera camera;
  camera.setSides(-0.03 * double( _ratio ), 0.03 * double( _ratio ),
                  -0.03, 0.03);
  camera.setNearFar(0.1,100);
  camera.setPosition(0,0,-5);
  camera.setTarget(0,0,0);
  camera.setUp(0,1,0);
  // set up the viewing transformation
  camera.loadViewMatrices();
  */

  glEnable( GL_DEPTH_TEST );
  glEnable( GL_LIGHTING );
  theDatabase->activateLights();
  theDatabase->draw();

  // draw info screen
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  gluOrtho2D( 0, 100, 0, 100 );

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  glDisable( GL_LIGHTING );
  glDisable( GL_DEPTH_TEST );
  glColor3f( 1.0f, 1.0f, 1.0f );
  drawHUD( _width, _height );

  /*
  if( windowInfo ) {
    wm->swapWindowBuffer( windowInfo->_windowID );
  }
  else {
    // NOTE: this will not have the expected result if there is more than
    // one window!!!
    wm->swapAllWindowBuffers();
  }
  */
}

void keyboardCB( arGUIKeyInfo* keyInfo )
{
  if( !keyInfo ) {
    std::cerr << "NULL keyInfo in keyboardCB!" << std::endl;
    return;
  }

  // only perform actions on key release, otherwise everything is done
  // twice, once on press and once on release
  if( keyInfo->getState() == AR_KEY_DOWN ) {
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
      bgColor = ( bgColor > 0.99f ? 0.0f : bgColor + 0.25f );
	    glClearColor( bgColor, bgColor, bgColor, 1.0f );
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
  if( theObject->supportsAnimation() ) {
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


void windowCB( arGUIWindowInfo* windowInfo )
{
  if( !windowInfo ) {
    std::cerr << "NULL windowInfo in windowCB!" << std::endl;
    return;
  }

  switch( windowInfo->getState() ) {
    case AR_WINDOW_RESIZE:
      wm->setWindowViewport( windowInfo->getWindowID(), 0, 0, windowInfo->getSizeX(), windowInfo->getSizeY() );
    break;

    case AR_WINDOW_CLOSE:
      wm->deleteWindow( windowInfo->getWindowID() );

      if( !wm->hasActiveWindows() ) {
        exit( 0 );
      }
    break;

    default:
    break;
  }
}

void mouseCB( arGUIMouseInfo* mouseInfo )
{
  arVector3 size = wm->getWindowSize( mouseInfo->getWindowID() );
  int width = int( size[ 0 ] );
  int height = int( size[ 1 ] );

  if( mouseInfo->getState() == AR_MOUSE_DRAG ) {
    const int deltaX = mouseInfo->getPosX() - mouseInfo->getPrevPosX();
    const int deltaY = mouseInfo->getPosY() - mouseInfo->getPrevPosY();

    switch( mouseManipState ) {
      case SLIDER:
      {
        arGUIMouseInfo* tMouseInfo = new arGUIMouseInfo( AR_MOUSE_EVENT, AR_GENERIC_STATE );
        tMouseInfo->setButton( AR_LBUTTON );
        tMouseInfo->setPosX( mouseInfo->getPosX() );
        tMouseInfo->setPosY( mouseInfo->getPosY() );
        tMouseInfo->setPrevPosX( mouseInfo->getPrevPosX() );
        tMouseInfo->setPrevPosY( mouseInfo->getPrevPosY() );

        mouseCB( tMouseInfo );

        delete tMouseInfo;
        return;
      }
      break;

      case PAN:
        mouseWorldMatrix = ar_translationMatrix( deltaX * 0.01f, deltaY * -0.01f, 0.0f ) *
			                     mouseWorldMatrix;
      break;

      case ROTATE:
      {
        arVector3 rotationAxis = arVector3( 0.0f, 0.0f, 1.0f ) * arVector3( float( deltaX ), float( deltaY ), 0 );

        if( ++rotationAxis > 0 ) {
          float mag = ++rotationAxis;
          rotationAxis = rotationAxis / mag;

          mouseWorldMatrix = ar_extractTranslationMatrix( mouseWorldMatrix ) *
   		                       ar_rotationMatrix( arVector3( -1.0f, 0.0f, 0.0f ), float( -deltaY ) / 300.0f ) *
   		                       ar_rotationMatrix( arVector3( 0.0f, 1.0f, 0.0f ), float( deltaX ) / 300.0f ) *
  		                       ar_extractRotationMatrix( mouseWorldMatrix ) *
  			                     ar_extractScaleMatrix( mouseWorldMatrix );
        }
      break;
      }

      case ZOOM:
        mouseWorldMatrix = mouseWorldMatrix * ar_scaleMatrix( 1.0f + 0.01f * float( deltaX ) );
      break;

      default:
      break;
    }

    dgTransform( mouseTransformID, mouseWorldMatrix );
  }

  // the rest is only applicable when the model supports animation
  if( mouseInfo->getButton() != AR_LBUTTON || !theObject->supportsAnimation() ||
      ( theObject->numberOfFrames() < 1 ) ) {
    return;
  }

  // mouse down on the slider bar
  if( mouseInfo->getPosY() > height - 20 && mouseInfo->getState() == AR_MOUSE_DOWN ) {
    oldState = mouseManipState;
    if( mouseInfo->getPosX() <= width - 57 && mouseInfo->getPosX() >= 20 ) {
      mouseManipState = SLIDER;
    }
    else {
      mouseManipState = NONE;
    }
  }

  if( mouseManipState == SLIDER ) { // we're dragging
    if( mouseInfo->getPosX() < 20 ) {
      theObject->setFrame( 0 );
    }
    else if( mouseInfo->getPosX() > width - 57 ) {
      theObject->setFrame( theObject->numberOfFrames() - 1 );
    }
    else {
      theObject->setFrame( int( float( mouseInfo->getPosX() - 20 ) / float( width - 77 ) *
                                float( theObject->numberOfFrames() ) ) );
	  }
  }

  if( mouseInfo->getState() == AR_MOUSE_UP ) {
    mouseManipState = oldState;	// done dragging bar
  }
}

int main( int argc, char** argv ){
  string fileName;
  if (argc == 1){
    // Attempt to open local config file.
    FILE* configFile = fopen("szgview.txt", "r");
    if (!configFile){
      cout << "szgview error: no parameter supplied and no szgview.txt config "
	   << "file.\n";
      cout << "usage: " << argv[ 0 ] << " file.{obj|3ds|htr|htr2} "
	   << "[mesh.obj]\n";
      return 1;
    }
    char buf[1024];
    if (fscanf(configFile, "%s", buf) < 1){
      // Config file is empty. This is an error.
      cout << "szgview error: config file szgview.txt is empty.\n";
      return 1;
    }
    fileName = string(buf);
  }
  else{
    // Using the first parameter as the file name.
    fileName = string(argv[1]);
  }
   

  theObject = arReadObjectFromFile( fileName.c_str(), "" );

  if( !theObject ) {
    std::cerr << "Invalid File: " << fileName.c_str() << std::endl;
    return 1;
  }

  arObject *theMesh = argc == 3 ? arReadObjectFromFile( argv[ 2 ], "" ) : NULL;

  arSZGClient SZGClient;
  SZGClient.init( argc, argv );
  if( !SZGClient ) {
    // It is OK if the arSZGClient fails to init.
    std::cout << "RUNNING IN STANDALONE MODE" << std::endl;
    // return 1;
  }
  string textPath = SZGClient.getAttribute("SZG_RENDER","text_path");

  theDatabase = new arGraphicsDatabase;
  char texPath[ 256 ] = {0};

  #ifndef AR_USE_WIN_32
    getcwd( texPath, 256 );
  #else
    texPath[ 0 ] = '\0';
  #endif

  theDatabase->setTexturePath( std::string( texPath ) );

  /*
  _width = 640;  // sensible defaults
  _height = 480;

  float sizeBuffer[ 2 ] = {0};

  // should use the screen name passed to the arSZGClient
  string screenName = SZGClient.getMode( "graphics" );
  if( SZGClient.getAttributeFloats( screenName, "size", sizeBuffer, 2 ) ) {
    _width = int( sizeBuffer[ 0 ] );
    _height = int( sizeBuffer[ 1 ] );
  }
  */

  arMatrix4 worldMatrix( ar_translationMatrix( 0.0, 5.0, -5.0 ) );

  dgSetGraphicsDatabase( theDatabase );
  dgTransform( "world", "root", worldMatrix );
  dgTransform( "mouse", "world", mouseWorldMatrix );
  mouseTransformID = theDatabase->getNodeID( "mouse" );

  // Go ahead and add some lights.
  //lightDirection = arVector3( 0.0f,-5.0f, 0.0f);   // dir of primary light
  //lightDirection /= ++lightDirection;
  // use as dir, not pos
  //arVector4 lightDir( lightDirection[ 0 ], lightDirection[ 1 ], lightDirection[ 2 ], 0.0f );
  (void) dgLight( "light0", "root", 0, arVector4(0,1,0,0), arVector3( 1,1,1 ) );
  (void) dgLight( "light0", "root", 1, arVector4(0,-1,0,0), arVector3( 1,1,1 ) );
  (void) dgLight( "light0", "root", 2, arVector4(0,0,1,0), arVector3( 1,1,1 ) );
  theObject->normalizeModelSize();    // fits into unit sphere

  if( theObject->type() == "HTR" ) {
    ((arHTR*) theObject)->basicDataSmoothing();
    if( argc == 3 ) {
      attachOBJToHTRToNodeInDatabase( (arOBJ*) theMesh,(arHTR*) theObject, "mouse" );
    }
    else {
      ((arHTR*) theObject)->attachMesh( "object", "mouse", true );
    }
  }
  else {
    theObject->attachMesh( "object", "mouse" );
  }

  /* Useful for debugging */
  // theDatabase->prettyDump();

  // FIXME: The window manager is *always* single threaded here.
  // This might not really make sense for computers with mutliple graphics
  // cards.
  wm = new arGUIWindowManager( windowCB, keyboardCB, mouseCB, NULL, false );

  // set up the head we'll use throughout
  head.setMatrix( ar_translationMatrix( 0.0, 5.0, 0.0 ) );

  std::string whichDisplay = SZGClient.getMode( "graphics" );
  std::string displayName = SZGClient.getAttribute( whichDisplay, "name" );

  std::cout << "Using display: " << displayName << std::endl;

  arGUIXMLParser guiXMLParser( &SZGClient,
                               SZGClient.getGlobalAttribute( displayName ) );

  // first parse the xml
  if( guiXMLParser.parse() < 0 ) {
    return -1;
  }

  const std::vector< arGUIXMLWindowConstruct* >* windowConstructs = guiXMLParser.getWindowingConstruct()->getWindowConstructs();

  // populate the callbacks for both the gui and graphics windows and the head
  // for any vr cameras
  std::vector< arGUIXMLWindowConstruct*>::const_iterator itr;
  for( itr = windowConstructs->begin(); itr != windowConstructs->end(); itr++ ) {
    (*itr)->getGraphicsWindow()->setInitCallback( new arDefaultWindowInitCallback() );
    (*itr)->getGraphicsWindow()->setDrawCallback( new arDefaultGUIRenderCallback( display ) );
    (*itr)->setGUIDrawCallback( new arSZGViewRenderCallback() );

    std::vector<arViewport>* viewports = (*itr)->getGraphicsWindow()->getViewports();
    std::vector<arViewport>::iterator vItr;
    for( vItr = viewports->begin(); vItr != viewports->end(); vItr++ ) {
      if( vItr->getCamera()->type() == "arVRCamera" ) {
        ((arVRCamera*) vItr->getCamera())->setHead( &head );
      }
    }
  }

  if( wm->createWindows( guiXMLParser.getWindowingConstruct() ) < 0 ) {
    return -1;
  }

  // must be done *after* an opengl context is created!
  std::string fontLocation = ar_fileFind( "courier_bold.txf", "", textPath );
  if( fontLocation != "NULL" ) {
    std::cout << "szgview remark: found szg system font.\n";
    texFont.loadFont( fontLocation );
  }
  else {
    texFont.loadFont( "courier_bold.txf" );
  }

  // wm->startWithoutSwap();
  wm->startWithSwap();

  return 0;
}
