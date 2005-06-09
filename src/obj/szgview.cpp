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

#include <iostream>

arMatrix4 mouseWorldMatrix = arMatrix4();
arMatrix4 lightTransformMatrix = arMatrix4();
arVector3 lightDirection = arVector3();

arGraphicsDatabase* theDatabase = NULL;

enum { PAN=0, ROTATE, ZOOM, SLIDER, NONE };

int mouseTransformID, lightTransformID, light0ID;
int mouseManipState = ROTATE, oldState = ROTATE;
bool isPlaying = false, isModelSpinning = false, isLightSpinning = false;
float bgColor = 0.0f, _ratio = 1.0f, frameRate = 0.0f;

int _width = 0, _height = 0;
int sliderCenterX = -50, sliderCenterY = 10; // slider position in pixels

arObject *theObject = NULL;

arTexFont texFont;

arGUIWindowManager* wm = NULL;

// window id's along with their associated arGraphicsWindows
std::map<int, arGraphicsWindow* > windows;

// since we don't get this from a tracker just make a sensible default one
arHead head;

// forward declaration
void drawCB( arGUIWindowInfo* windowInfo );

// To be able to use arGraphicsWindow::draw in the arGUIWindow as the draw
// callback
class arSZGViewRenderCallback : public arGUIRenderCallback
{
  public:

    arSZGViewRenderCallback( arGraphicsWindow* gw ) :
      _graphicsWindow( gw ) { }

    virtual ~arSZGViewRenderCallback( void )  { }

    virtual void operator()( arGraphicsWindow&, arViewport& ) {
      if( _graphicsWindow ) {
        _graphicsWindow->draw();
      }
    }

    virtual void operator()( arGUIWindowInfo* windowInfo ) {
      if( _graphicsWindow ) {
        // HACK because the width/height could be different between windows but
        // because we can't pass anything to the graphicswindow drawCB the
        // globals need to be set here
        arVector3 size = wm->getWindowSize( windowInfo->_windowID );
        _width =  int( size[ 0 ] );
        _height = int( size[ 1 ] );

        _graphicsWindow->draw();
      }
    }

  private:

    arGraphicsWindow* _graphicsWindow;

};

TiXmlNode* getNamedNode( arSZGClient& SZGClient, const char* name = NULL )
{
  if( !name ) {
    return NULL;
  }

  // caller will own this and should delete it
  TiXmlDocument* nodeDoc = new TiXmlDocument();

  std::string nodeDesc = SZGClient.getGlobalAttribute( name );

  if( !nodeDesc.length() || nodeDesc == "NULL" ) {
    std::cout << "node points to non-existent node: " << name << std::endl;
    return NULL;
  }

  std::cout << "creating node from pointer to: " << name << std::endl;

  // create a usable node out of the xml string
  nodeDoc->Parse( nodeDesc.c_str() );
  if( !nodeDoc->FirstChild() ) {
    std::cout << "node pointer: " << name << " is invalid" << std::endl;
    return NULL;
  }

  return nodeDoc->FirstChild();
}

arVector3 AttributearVector3( TiXmlNode* node,
                              const std::string& x = "x", const std::string& y = "y", const std::string& z = "z" )
{
  arVector3 vec;

  if( !node || !node->ToElement() ) {
    return vec;
  }

  node->ToElement()->Attribute( x.c_str(), &vec[ 0 ] );
  node->ToElement()->Attribute( y.c_str(), &vec[ 1 ] );
  node->ToElement()->Attribute( z.c_str(), &vec[ 2 ] );

  return vec;
}

arVector4 AttributearVector4( TiXmlNode* node, const std::string& x = "x", const std::string& y = "y",
                                               const std::string& z = "z", const std::string& w = "w" )
{
  arVector4 vec;

  if( !node || !node->ToElement() ) {
    return vec;
  }

  node->ToElement()->Attribute( x.c_str(), &vec[ 0 ] );
  node->ToElement()->Attribute( y.c_str(), &vec[ 1 ] );
  node->ToElement()->Attribute( z.c_str(), &vec[ 2 ] );
  node->ToElement()->Attribute( w.c_str(), &vec[ 3 ] );

  return vec;
}

bool AttributeBool( TiXmlNode* node, const std::string& value = "value" )
{
  bool result = false;

  if( !node || !node->ToElement() ) {
    return result;
  }

  // err, pasing strcmp NULL's is undefined...
  if( !strcmp( node->ToElement()->Attribute( value.c_str() ), "true" ) ||
      !strcmp( node->ToElement()->Attribute( value.c_str() ), "yes" ) ) {
    result = true;
  }

  return result;
}

int configureScreen( arSZGClient& SZGClient, arGraphicsScreen& screen, TiXmlNode* screenNode = NULL )
{
  if( !screenNode || !screenNode->ToElement() ) {
    // not necessarily an error, <szg_screen> could legitimately not exist and
    // in that case let the caller use the screen as it was passed in
    std::cout << "NULL or invalid screen, returning passed in arGraphicsScreen" << std::endl;
    return 0;
  }

  std::cout << "configuring screen" << std::endl;

  // check if this is a pointer to another screen
  TiXmlNode* namedNode = getNamedNode( SZGClient, screenNode->ToElement()->Attribute( "usenamed" ) );
  if( namedNode ) {
    screenNode = namedNode;
  }

  TiXmlNode* screenElement = NULL;

  // <center x="float" y="float" z="float" />
  if( screenElement = screenNode->FirstChild( "center" ) ) {
    arVector3 vec = AttributearVector3( screenElement );
    screen.setCenter( vec );
  }

  // <normal x="float" y="float" z="float" />
  if( screenElement = screenNode->FirstChild( "normal" ) ) {
    arVector3 vec = AttributearVector3( screenElement );
    screen.setNormal( vec );
  }

  // <up x="float" y="float" z="float" />
  if( screenElement = screenNode->FirstChild( "up" ) ) {
    arVector3 vec = AttributearVector3( screenElement );
    screen.setUp( vec );
  }

  // <dim width="float" height="float" />
  if( screenElement = screenNode->FirstChild( "dim" ) ) {
    float dim[ 2 ] = { 0.0f };
    screenElement->ToElement()->Attribute( "width",  &dim[ 0 ] );
    screenElement->ToElement()->Attribute( "height", &dim[ 1 ] );

    screen.setDimensions( dim[ 0 ], dim[ 1 ] );
  }

  // <headmounted value="true|false|yes|no" />
  if( screenElement = screenNode->FirstChild( "headmounted" ) ) {
    screen.setHeadMounted( AttributeBool( screenElement ) );
  }

  // <tile tilex="integer" numtilesx="integer" tiley="integer" numtilesy="integer" />
  if( screenElement = screenNode->FirstChild( "tile" ) ) {
    arVector4 vec = AttributearVector4( screenElement, "tilex", "numtilesx", "tiley", "numtilesy" );
    screen.setTile( vec );
  }

  // <usefixedhead value="allow|always|ignore" />
  if( screenElement = screenNode->FirstChild( "usefixedhead" ) ) {
    screen.setUseFixedHeadMode( screenElement->ToElement()->Attribute( "value" ) );
  }

  // <fixedheadpos x="float" y="float" z="float" />
  if( screenElement = screenNode->FirstChild( "fixedheadpos" ) ) {
    arVector3 vec = AttributearVector3( screenElement );
    screen.setFixedHeadPosition( vec );
  }

  // <fixedheadupangle value="float" />
  if( screenElement = screenNode->FirstChild( "fixedheadupangle" ) ) {
    float angle;
    screenElement->ToElement()->Attribute( "value", &angle );

    screen.setFixedHeadHeadUpAngle( angle );
  }

  if( namedNode && namedNode->GetDocument() ) {
    delete namedNode->GetDocument();
  }

  return 0;
}

arCamera* configureCamera( arSZGClient& SZGClient, arGraphicsScreen& screen, TiXmlNode* cameraNode = NULL )
{
  // caller owns this and should delete it
  arCamera* camera;

  if( !cameraNode || !cameraNode->ToElement() ) {
    // not necessarily an error, the camera node could legitimately not exist
    // in which case a default camera should be returned
    std::cout << "NULL or invalid cameraNode, creating default arVRCamera (with default screen)" << std::endl;
    camera = new arVRCamera( &head );
    return camera;
  }

  std::cout << "configuring camera" << std::endl;

  // check if this is a pointer to another camera
  TiXmlNode* namedNode = getNamedNode( SZGClient, cameraNode->ToElement()->Attribute( "usenamed" ) );
  if( namedNode ) {
    cameraNode = namedNode;
  }

  TiXmlNode* cameraElement = NULL;

  std::string cameraType( "vr" );

  if( cameraNode->ToElement()->Attribute( "type" ) ) {
    cameraType = cameraNode->ToElement()->Attribute( "type" );
  }

  if( configureScreen( SZGClient, screen, cameraNode->FirstChild( "szg_screen" ) ) < 0 ) {
    // print warning, return default camera + screen
  }

  if( cameraType == "ortho" || cameraType == "perspective" ) {
    std::cout << "creating " << cameraType << " camera" << std::endl;
    if( cameraType == "ortho" ) {
      #define CAM_CAST( x ) ((arOrthoCamera*) x)
      camera = new arOrthoCamera();
    }
    else if( cameraType== "perspective" ) {
      #define CAM_CAST( x ) ((arPerspectiveCamera*) x)
      camera = new arPerspectiveCamera();
    }

    if( cameraElement = cameraNode->FirstChild( "frustum" ) ) {
      float ortho[ 6 ] = { 0.0f };
      cameraElement->ToElement()->Attribute( "left",   &ortho[ 0 ] );
      cameraElement->ToElement()->Attribute( "right",  &ortho[ 1 ] );
      cameraElement->ToElement()->Attribute( "bottom", &ortho[ 2 ] );
      cameraElement->ToElement()->Attribute( "top",    &ortho[ 3 ] );
      cameraElement->ToElement()->Attribute( "near",   &ortho[ 4 ] );
      cameraElement->ToElement()->Attribute( "far",    &ortho[ 5 ] );

      CAM_CAST( camera )->setFrustum( ortho );
    }

    if( cameraElement = cameraNode->FirstChild( "lookat" ) ) {
      float look[ 9 ] = { 0.0f };
      cameraElement->ToElement()->Attribute( "viewx",   &look[ 0 ] );
      cameraElement->ToElement()->Attribute( "viewy",   &look[ 1 ] );
      cameraElement->ToElement()->Attribute( "viewz",   &look[ 2 ] );
      cameraElement->ToElement()->Attribute( "lookatx", &look[ 3 ] );
      cameraElement->ToElement()->Attribute( "lookaty", &look[ 4 ] );
      cameraElement->ToElement()->Attribute( "lookatz", &look[ 5 ] );
      cameraElement->ToElement()->Attribute( "upx",     &look[ 6 ] );
      cameraElement->ToElement()->Attribute( "upy",     &look[ 7 ] );
      cameraElement->ToElement()->Attribute( "upz",     &look[ 8 ] );

      CAM_CAST( camera )->setLook( look );
    }

    if( cameraElement = cameraNode->FirstChild( "sides" ) ) {
      arVector4 vec = AttributearVector4( cameraElement, "left", "right", "bottom", "sides" );
      CAM_CAST( camera )->setSides( vec );
    }

    if( cameraElement = cameraNode->FirstChild( "clipping" ) ) {
      float planes[ 2 ] = { 0.0f };
      cameraElement->ToElement()->Attribute( "near", &planes[ 0 ] );
      cameraElement->ToElement()->Attribute( "far",  &planes[ 1 ] );

      CAM_CAST( camera )->setNearFar( planes[ 0 ], planes[ 1 ] );
    }

    if( cameraElement = cameraNode->FirstChild( "position" ) ) {
      arVector3 vec = AttributearVector3( cameraElement );
      CAM_CAST( camera )->setPosition( vec );
    }

    if( cameraElement = cameraNode->FirstChild( "target" ) ) {
      arVector3 vec = AttributearVector3( cameraElement );
      CAM_CAST( camera )->setTarget( vec );
    }

    if( cameraElement = cameraNode->FirstChild( "up" ) ) {
      arVector3 vec = AttributearVector3( cameraElement );
      CAM_CAST( camera )->setUp( vec );
    }
  }
  else if( cameraType == "vr" ) {
    std::cout << "creating vr camera" << std::endl;
    camera = new arVRCamera( &head );
  }
  else {
    std::cout << "unknown camera type, creating default arVRCamera" << std::endl;
    camera = new arVRCamera( &head );
  }

  if( namedNode && namedNode->GetDocument() ) {
    delete namedNode->GetDocument();
  }

  camera->setScreen( &screen );

  return camera;
}

int parseConfig( arSZGClient& SZGClient, const std::string& config )
{
  std::cout << "parsing config: " << std::endl
            << config << std::endl << std::endl;

  TiXmlDocument doc;
  doc.Parse( config.c_str() );
  TiXmlHandle docHandle( &doc );

  doc.Print();
  std::cout << std::endl;

  // get a reference to <szg_display>
  TiXmlNode* szgDisplayNode = doc.FirstChild();

  // iterate over all <szg_window> elements
  for( TiXmlNode* windowNode = szgDisplayNode->FirstChild( "szg_window" ); windowNode; windowNode = windowNode->NextSibling() ) {
    std::cout << "creating window" << std::endl;

    TiXmlNode* windowElement = NULL;

    // save the current position in the list of <szg_window> siblings (to be
    // restored if this windowNode is a 'pointer')
    TiXmlNode* savedWindowNode = windowNode;

    // check if this is a pointer to another window
    TiXmlNode* namedWindowNode = getNamedNode( SZGClient, windowNode->ToElement()->Attribute( "usenamed" ) );
    if( namedWindowNode ) {
      windowNode = namedWindowNode;
    }

    if( !windowNode->ToElement() ) {
      std::cout << "invalid window, skipping" << std::endl;
      continue;
    }

    arGUIWindowConfig windowConfig;

    // <size width="integer" height="integer" />
    if( windowElement = windowNode->FirstChild( "size" ) ) {
      windowElement->ToElement()->Attribute( "width",  &windowConfig._width );
      windowElement->ToElement()->Attribute( "height", &windowConfig._height );
    }

    // <position x="integer" y="integer" />
    if( windowElement = windowNode->FirstChild( "position" ) ) {
      windowElement->ToElement()->Attribute( "x", &windowConfig._x );
      windowElement->ToElement()->Attribute( "y", &windowConfig._y );
    }

    // <fullscreen value="true|false|yes|no" />
    if( windowElement = windowNode->FirstChild( "fullscreen" ) ) {
      windowConfig._fullscreen = AttributeBool( windowElement );
    }

    // <decorate value="true|false|yes|no" />
    if( windowElement = windowNode->FirstChild( "decorate" ) ) {
      windowConfig._decorate = AttributeBool( windowElement );
    }

    // <stereo value="true|false|yes|no" />
    if( windowElement = windowNode->FirstChild( "stereo" ) ) {
      windowConfig._stereo = AttributeBool( windowElement );
    }

    // <topmost value="true|false|yes|no" />
    if( windowElement = windowNode->FirstChild( "topmost" ) ) {
      windowConfig._topmost = AttributeBool( windowElement );
    }

    // <bpp value="integer" />
    if( windowElement = windowNode->FirstChild( "bpp" ) ) {
      windowElement->ToElement()->Attribute( "value", &windowConfig._bpp );
    }

    // <title value="string" />
    if( windowElement = windowNode->FirstChild( "title" ) ) {
      windowConfig._title = windowElement->ToElement()->Attribute( "value" );
    }

    // <xdisplay value="string" />
    if( windowElement = windowNode->FirstChild( "xdisplay" ) ) {
      windowConfig._XDisplay = windowElement->ToElement()->Attribute( "value" );
    }

    std::cout << "TITLE: " << windowConfig._title << std::endl;
    std::cout << "WIDTH: " << windowConfig._width << std::endl;
    std::cout << "HEIGHT: " << windowConfig._height << std::endl;
    std::cout << "X: " << windowConfig._x << std::endl;
    std::cout << "Y: " << windowConfig._y << std::endl;
    std::cout << "FULLSCREEN: " << windowConfig._fullscreen << std::endl;
    std::cout << "STEREO: " << windowConfig._stereo << std::endl;
    std::cout << "TOPMOST: " << windowConfig._topmost << std::endl;
    std::cout << "BPP: " << windowConfig._bpp << std::endl;
    std::cout << "XDISPLAY: " << windowConfig._XDisplay << std::endl;

    int winID = wm->addWindow( windowConfig );
    if( winID < 0 ) {
      std::cout << "addWindow failure" << std::endl;
    }

    std::cout << "creating arGraphicsWindow" << std::endl;
    // create the associated arGraphicsWindow
    windows[ winID ] = new arGraphicsWindow();

    // register this program's draw callback with the arGraphicsWindow
    windows[ winID ]->setDrawCallback( new arDefaultGUIRenderCallback( drawCB ) );
    windows[ winID ]->setInitCallback( new arDefaultWindowInitCallback() );

    // register the arGraphicsWindow's draw callback with the arGUIWindow
    wm->registerDrawCallback( winID, new arSZGViewRenderCallback( windows[ winID ] ) );

    windows[ winID ]->useOGLStereo( wm->isStereo( winID ) );

    std::string viewMode( "normal" );

    // run through all the viewports in the viewport list
    TiXmlNode* viewportListNode = windowNode->FirstChild( "szg_viewport_list" );
    TiXmlNode* namedViewportListNode = NULL;

    // possible not to have a <szg_viewport_list>, so the viewportListNode
    // pointer needs to be checked from here on out
    if( viewportListNode ) {
      // check if this is a pointer to another viewportlist
      namedViewportListNode = getNamedNode( SZGClient, viewportListNode->ToElement()->Attribute( "usenamed" ) );
      if( namedViewportListNode ) {
        viewportListNode = namedViewportListNode;
      }

      if( !viewportListNode->ToElement() ) {
        std::cout << "invalid viewportlist" << std::endl;
      }

      // determine which viewmode was specified, anything other than "custom"
      // doesn't need any viewports to actually be listed
      // <viewmode value="normal|anaglyph|walleyed|crosseyed|overunder|custom" />
      if( viewportListNode->ToElement()->Attribute( "viewmode" ) ) {
        viewMode = viewportListNode->ToElement()->Attribute( "viewmode" );
      }
    }

    std::cout << "using viewmode: " << viewMode << std::endl;

    if( viewMode == "custom" ) {
      TiXmlNode* viewportNode = NULL;

      // if the user specified a viewmode of 'custom' but then didn't specify
      // any <szg_viewport>'s - that is an error! (the only way we can get here
      // is if viewportListNode /does/ exist, no need to check it again)
      if( !(viewportNode = viewportListNode->FirstChild( "szg_viewport" ) ) ) {
        // malformed!, delete currentwindow, print warning, continue on to next window tage
        std::cout << "viewmode == custom, but no <szg_viewport> tags!" << std::endl;
      }

      // clear out the 'standard' viewport
      windows[ winID ]->clearViewportList();

      // iterate over all <szg_viewport> elements
      for( viewportNode = viewportListNode->FirstChild( "szg_viewport" ); viewportNode; viewportNode = viewportNode->NextSibling( "szg_viewport" ) ) {
        std::cout << "creating viewport" << std::endl;
        TiXmlNode* savedViewportNode = viewportNode;

        // check if this is a pointer to another window
        TiXmlNode* namedViewportNode = getNamedNode( SZGClient, viewportNode->ToElement()->Attribute( "usenamed" ) );
        if( namedViewportNode ) {
          viewportNode = namedViewportNode;
        }

        if( !viewportNode->ToElement() ) {
          std::cout << "invalid viewport, skipping" << std::endl;
          continue;
        }

        TiXmlNode* viewportElement = NULL;

        // configure the camera and possibly the camera's screen
        arCamera* camera = NULL;
        arGraphicsScreen screen;

        if( !(camera = configureCamera( SZGClient, screen, viewportNode->FirstChild( "szg_camera" ) )) ) {
          // should never happen, configureCamera should always return at least /something/
          std::cout << "custom configureCamera failure" << std::endl;
        }

        arViewport viewport;

        viewport.setCamera( camera );
        viewport.setScreen( screen );

        // fill in viewport parameters from viewportNode elements
        // <coords left="float" bottom="float" width="float" height="float" />
        if( ( viewportElement = viewportNode->FirstChild( "coords" ) ) && viewportElement->ToElement() ) {
          arVector4 vec = AttributearVector4( viewportElement, "left", "bottom", "width", "height" );
          viewport.setViewport( vec );
        }

        // <depthclear value="true|false|yes|no" />
        if( ( viewportElement = viewportNode->FirstChild( "depthclear" ) ) && viewportElement->ToElement() ) {
          viewport.clearDepthBuffer( AttributeBool( viewportElement ) );
        }

        // <colormask R="true|false|yes|no" G="true|false|yes|no" B="true|false|yes|no" A="true|false|yes|no" />
        if( ( viewportElement = viewportNode->FirstChild( "colormask" ) ) && viewportElement->ToElement() ) {
          bool colorMask[ 4 ];
          colorMask[ 0 ] = AttributeBool( viewportElement, "R" );
          colorMask[ 1 ] = AttributeBool( viewportElement, "G" );
          colorMask[ 2 ] = AttributeBool( viewportElement, "B" );
          colorMask[ 3 ] = AttributeBool( viewportElement, "A" );
          viewport.setColorMask( colorMask[ 0 ], colorMask[ 1 ], colorMask[ 2 ], colorMask[ 3 ] );
        }

        // <eyesign value="float" />
        if( ( viewportElement = viewportNode->FirstChild( "eyesign" ) ) && viewportElement->ToElement() ) {
          float eyesign;
          viewportElement->ToElement()->Attribute( "value", &eyesign );
          viewport.setEyeSign( eyesign );
        }

        // <ogldrawbuf value="GL_NONE|GL_FRONT_LEFT|GL_FRONT_RIGHT|GL_BACK_LEFT|GL_BACK_RIGHT|GL_FRONT|GL_BACK|GL_LEFT|GL_RIGHT|GL_FRONT_AND_BACK" />
        if( ( viewportElement = viewportNode->FirstChild( "ogldrawbuf" ) ) && viewportElement->ToElement() ) {
          GLenum ogldrawbuf;

          std::string buf = viewportElement->ToElement()->Attribute( "value" );
          if( buf == "GL_NONE" )                ogldrawbuf = GL_NONE;
          else if( buf == "GL_FRONT_LEFT" )     ogldrawbuf = GL_FRONT_LEFT;
          else if( buf == "GL_FRONT_RIGHT" )    ogldrawbuf = GL_FRONT_RIGHT;
          else if( buf == "GL_BACK_LEFT" )      ogldrawbuf = GL_BACK_LEFT;
          else if( buf == "GL_BACK_RIGHT" )     ogldrawbuf = GL_BACK_RIGHT;
          else if( buf == "GL_FRONT" )          ogldrawbuf = GL_FRONT;
          else if( buf == "GL_BACK" )           ogldrawbuf = GL_BACK;
          else if( buf == "GL_LEFT" )           ogldrawbuf = GL_LEFT;
          else if( buf == "GL_RIGHT" )          ogldrawbuf = GL_RIGHT;
          else if( buf == "GL_FRONT_AND_BACK" ) ogldrawbuf = GL_FRONT_AND_BACK;
          else                                  ogldrawbuf = GL_NONE;

          viewport.setDrawBuffer( ogldrawbuf );
        }

        std::cout << "adding custom viewport" << std::endl;
        windows[ winID ]->addViewport( viewport );

        // if viewportNode was a pointer, revert it back so that its
        // siblings can be traversed properly
        viewportNode = savedViewportNode;

        if( namedViewportNode && namedViewportNode->GetDocument() ) {
          delete namedViewportNode->GetDocument();
        }
        if( camera ) {
          // setCamera made a copy and now 'owns' the camera, safe to delete
          delete camera;
        }
      }
    }
    else {
      // even though there's no viewport specified in this case, there could
      // still be a camera+screen tag that needs to be parsed
      arCamera* camera = NULL;
      arGraphicsScreen screen;

      if( !(camera = configureCamera( SZGClient, screen, viewportListNode ? viewportListNode->FirstChild( "szg_camera" ) : NULL )) ) {
        // should never happen, configureCamera should always return at least /something/
        std::cout << "non-custom configureCamera failure" << std::endl;
      }

      // viewports added by setViewMode will use this camera and screen
      windows[ winID ]->setCamera( camera );
      windows[ winID ]->setScreen( screen );

      // set up the appropriate viewports
      if( !windows[ winID ]->setViewMode( viewMode ) ) {
        std::cout << "setViewMode failure!" << std::endl;
      }

      if( camera ) {
        // setCamera made a copy and now 'owns' the camera, safe to delete
        delete camera;
      }
    }

    // if windowNode was changed because it was a 'pointer' to another window,
    // revert it back to its position in the list of sibling windows for this
    // <szg_display>
    windowNode = savedWindowNode;

    if( namedWindowNode && namedWindowNode->GetDocument() ) {
      delete namedWindowNode->GetDocument();
    }
    if( namedViewportListNode && namedViewportListNode->GetDocument() ) {
      delete namedViewportListNode->GetDocument();
    }
  }

  std::cout << "done parsing" << std::endl;

  return 0;
}

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

void drawCB( arGUIWindowInfo* windowInfo )
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

  if( isLightSpinning ) {
    lightTransformMatrix = ar_rotationMatrix( arVector3( 1.0f, 0.0f, 0.0f ), 0.05f ) *
    			                 ar_rotationMatrix( arVector3( 0.0f, 1.0f, 0.0f ), 0.07f ) *
    			                 ar_rotationMatrix( arVector3( 0.0f, 0.0f, 1.0f ), 0.03f ) *
	    		                 lightTransformMatrix;

    arVector3 tmp( lightTransformMatrix * lightDirection );
    dgLight( light0ID, 0, arVector4( tmp[ 0 ], tmp[ 1 ], tmp[ 2 ], 0 ), arVector3( 1.0f, 1.0f, 0.95f ) );
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
  if( keyInfo->_state == AR_KEY_DOWN ) {
    return;
  }

  switch( keyInfo->_key ) {
    case AR_VK_ESC:
      exit( 0 );
    break;

    case AR_VK_f:
      wm->fullscreenWindow( keyInfo->_windowID );
    break;

    case AR_VK_F:
      wm->resizeWindow( keyInfo->_windowID, 640, 480 );
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

    case AR_VK_9:
      isLightSpinning = !isLightSpinning;
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
    switch( keyInfo->_key  ) {
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

  switch( windowInfo->_state ) {
    case AR_WINDOW_RESIZE:
      wm->setWindowViewport( windowInfo->_windowID, 0, 0, windowInfo->_sizeX, windowInfo->_sizeY );
    break;

    case AR_WINDOW_CLOSE:
      wm->deleteWindow( windowInfo->_windowID );

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
  arVector3 size = wm->getWindowSize( mouseInfo->_windowID );
  int width = int( size[ 0 ] );
  int height = int( size[ 1 ] );

  if( mouseInfo->_state == AR_MOUSE_DRAG ) {
    const int deltaX = mouseInfo->_posX - mouseInfo->_prevPosX;
    const int deltaY = mouseInfo->_posY - mouseInfo->_prevPosY;

    switch( mouseManipState ) {
      case SLIDER:
      {
        arGUIMouseInfo* tMouseInfo = new arGUIMouseInfo( AR_MOUSE_EVENT, AR_GENERIC_STATE );
        tMouseInfo->_button = AR_LBUTTON;
        tMouseInfo->_posX = mouseInfo->_posX;
        tMouseInfo->_posY = mouseInfo->_posY;
        tMouseInfo->_prevPosX = mouseInfo->_prevPosX;
        tMouseInfo->_prevPosY = mouseInfo->_prevPosY;

        mouseCB( tMouseInfo );

        delete tMouseInfo;
        return;
      }
      break;

      case PAN:
        mouseWorldMatrix = ar_translationMatrix( deltaX * -0.01f, deltaY * -0.01f, 0.0f ) *
			                     mouseWorldMatrix;
      break;

      case ROTATE:
      {
        arVector3 rotationAxis = arVector3( 0.0f, 0.0f, 1.0f ) * arVector3( float( deltaX ), float( deltaY ), 0 );

        if( ++rotationAxis > 0 ) {
          float mag = ++rotationAxis;
          rotationAxis = rotationAxis / mag;

          mouseWorldMatrix = ar_extractTranslationMatrix( mouseWorldMatrix ) *
   		                       ar_rotationMatrix( arVector3( -1.0f, 0.0f, 0.0f ), float( deltaY ) / 300.0f ) *
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
  if( mouseInfo->_button != AR_LBUTTON || !theObject->supportsAnimation() ||
      ( theObject->numberOfFrames() < 1 ) ) {
    return;
  }

  // mouse down on the slider bar
  if( mouseInfo->_posY > height - 20 && mouseInfo->_state == AR_MOUSE_DOWN ) {
    oldState = mouseManipState;
    if( mouseInfo->_posX <= width - 57 && mouseInfo->_posX >= 20 ) {
      mouseManipState = SLIDER;
    }
    else {
      mouseManipState = NONE;
    }
  }

  if( mouseManipState == SLIDER ) { // we're dragging
    if( mouseInfo->_posX < 20 ) {
      theObject->setFrame( 0 );
    }
    else if( mouseInfo->_posX > width - 57 ) {
      theObject->setFrame( theObject->numberOfFrames() - 1 );
    }
    else {
      theObject->setFrame( int( float( mouseInfo->_posX - 20 ) / float( width - 77 ) *
                                float( theObject->numberOfFrames() ) ) );
	  }
  }

  if( mouseInfo->_state == AR_MOUSE_UP ) {
    mouseManipState = oldState;	// done dragging bar
  }
}

int main( int argc, char** argv )
{
  if( argc < 2 ) {
    std::cerr << "usage: " << argv[ 0 ] << " file.{obj|3ds|htr} [mesh.obj]\n";
    return 1;
  }

  theObject = arReadObjectFromFile( argv[ 1 ], "" );

  if( !theObject ) {
    std::cerr << "Invalid File: " << argv[ 1 ] << std::endl;
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

  theDatabase->setTexturePath( string( texPath ) );

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
  //UNUSED int worldTransformID = theDatabase->getNodeID( "world" );
  dgTransform( "mouse", "world", mouseWorldMatrix );
  mouseTransformID = theDatabase->getNodeID( "mouse" );

  // lightTransformMatrix = ar_translationMatrix( 0.0, 5.0, 0.0 );
  dgTransform( "lightTransform", "world", lightTransformMatrix );
  lightTransformID = theDatabase->getNodeID( "lightTransform" );

  lightDirection = arVector3( 0.0f, 5.0f, 0.0f);   // dir of primary light
  lightDirection /= ++lightDirection;

  // use as dir, not pos
  arVector4 lightDir( lightDirection[ 0 ], lightDirection[ 1 ], lightDirection[ 2 ], 0.0f );
  light0ID = dgLight( "light0", "lightTransform", 0, lightDir, arVector3( 1.0f, 1.0f, 0.95f ) );
  theObject->normalizeModelSize();    // fits into unit sphere
  // theObject->setTransform( ar_translationMatrix( 0.0, 5.0, -5.0 ) );

  if( theObject->type() == "HTR" ) {
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

  wm = new arGUIWindowManager( windowCB, keyboardCB, mouseCB, true );

  // set up the head we'll use throughout
  head.setMatrix( ar_translationMatrix( 0.0, 5.0, 0.0 ) );

  string displayName = SZGClient.getAttribute( "SZG_DISPLAY0", "name" );

  std::cout << "Using display: " << displayName << std::endl;

  parseConfig( SZGClient, SZGClient.getGlobalAttribute( displayName ) );

  /*
  arGUIWindowConfig windowConfig;
  windowConfig._x = 50;
  windowConfig._y = 50;
  windowConfig._width = ( _width <= 0 ) ? 640 : _width;
  windowConfig._height = ( _height <= 0 ) ? 480 : _height;
  _ratio = double(windowConfig._width) / double(windowConfig._height);
  windowConfig._fullscreen = false;
  windowConfig._decorate = true;
  windowConfig._stereo = false;
  windowConfig._topmost = false;
  windowConfig._title = "szgview";

  int winID = wm->addWindow( windowConfig, new arDefaultGUIRenderCallback( drawCB ) );
  */

  // must be done *after* an opengl context is created!
  string fontLocation = ar_fileFind("courier_bold.txf","",textPath);
  if (fontLocation != "NULL"){
    cout << "szgview remark: found szg system font.\n";
    texFont.loadFont(fontLocation);
  }
  else{
    texFont.loadFont( "courier_bold.txf" );
  }

  // wm->startWithoutSwap();
  wm->startWithSwap();

  return 0;
}
