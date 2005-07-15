//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for detils
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGUIXMLParser.h"
#include "arGUIWindowManager.h"
#include "arGUIWindow.h"
#include "arGraphicsScreen.h"
#include "arGraphicsWindow.h"
#include "arViewport.h"
#include "arCamera.h"
#include "arVRCamera.h"
#include "arPerspectiveCamera.h"
#include "arOrthoCamera.h"

#include <iostream>

arGUIXMLWindowConstruct::arGUIXMLWindowConstruct( arGUIWindowConfig* windowConfig,
                                                  arGraphicsWindow* graphicsWindow,
                                                  arGUIRenderCallback* guiDrawCallback ) :
  _windowConfig( windowConfig ),
  _graphicsWindow( graphicsWindow ),
  _guiDrawCallback( guiDrawCallback )
{

}

arGUIXMLWindowConstruct::~arGUIXMLWindowConstruct( void )
{
  // depending on how windows have been reloaded and who might have copied
  // around the graphisWindow pointer, this may or may not be safe...
  delete _graphicsWindow;
  delete _windowConfig;
}

arGUIWindowingConstruct::arGUIWindowingConstruct( int threaded, int useFramelock,
                                                     std::vector< arGUIXMLWindowConstruct* >* windowConstructs ) :
  _threaded( threaded ),
  _useFramelock( useFramelock ),
  _windowConstructs( windowConstructs )
{

}

arGUIWindowingConstruct::~arGUIWindowingConstruct( void )
{

}

arGUIXMLParser::arGUIXMLParser( arSZGClient* SZGClient,
                                const std::string& config ) :
  _SZGClient( SZGClient ),
  // _config( config ),
  _mininumConfig( "<szg_display><szg_window /></szg_display>" )
{
  setConfig( config );

  _windowingConstruct = new arGUIWindowingConstruct();
}

arGUIXMLParser::~arGUIXMLParser( void )
{
  _doc.Clear();
}

void arGUIXMLParser::setConfig( const std::string& config )
{
  if( _config == config ) {
    // return;
  }

  _config = config;

  if( !_config.length() || _config == "NULL" ) {
    std::cout << "config is NULL or empty, using minimum config" << std::endl;
    _config = _mininumConfig;
  }

  // NOTE: It is very important to clear the document first. Otherwise, this
  // new config string will just be appended to the end of old config strings,
  // which is NOT what is wanted.
  _doc.Clear();

  _doc.Parse( _config.c_str() );
}

/*
int arGUIXMLParser::numberOfWindows( void )
{
  int count = 0;

  if( _doc.Error() ) {
    return count;
  }

  // get a reference to <szg_display>
  TiXmlNode* szgDisplayNode = _doc.FirstChild();

  if( !szgDisplayNode ) {
    return count;
  }

  // iterate over all <szg_window> elements
  for( TiXmlNode* windowNode = szgDisplayNode->FirstChild( "szg_window" ); windowNode;
       windowNode = windowNode->NextSibling() ) {
    count++;
  }

  return count;
}
*/

// NOTE: this function can and often *does* return NULL!
TiXmlNode* arGUIXMLParser::_getNamedNode( const char* name )
{
  if( !name || !_SZGClient ) {
    return NULL;
  }

  // caller will own this and should delete it
  TiXmlDocument* nodeDoc = new TiXmlDocument();

  std::string nodeDesc = _SZGClient->getGlobalAttribute( name );

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

arVector3 arGUIXMLParser::_attributearVector3( TiXmlNode* node,
                                               const std::string& x,
                                               const std::string& y,
                                               const std::string& z )
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

arVector4 arGUIXMLParser::_attributearVector4( TiXmlNode* node,
                                               const std::string& x,
                                               const std::string& y,
                                               const std::string& z,
                                               const std::string& w )
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

bool arGUIXMLParser::_attributeBool( TiXmlNode* node,
                                     const std::string& value )
{
  bool result = false;

  if( !node || !node->ToElement() ) {
    return result;
  }

  // passing NULL to strcmp below is undefined
  if( !node->ToElement()->Attribute( value.c_str() ) ) {
    return result;
  }

  if( !strcmp( node->ToElement()->Attribute( value.c_str() ), "true" ) ||
      !strcmp( node->ToElement()->Attribute( value.c_str() ), "yes" ) ) {
    result = true;
  }

  return result;
}

int arGUIXMLParser::_configureScreen( arGraphicsScreen& screen,
                                      TiXmlNode* screenNode ){

  if( !screenNode || !screenNode->ToElement() ) {
    // not necessarily an error, <szg_screen> could legitimately not exist and
    // in that case let the caller use the screen as it was passed in
    std::cout << "NULL or invalid screen description, "
	      << "returning passed in arGraphicsScreen" << std::endl;
    return 0;
  }

  std::cout << "configuring screen" << std::endl;

  // check if this is a pointer to another screen
  TiXmlNode* namedNode = _getNamedNode( screenNode->ToElement()->Attribute( "usenamed" ) );
  if( namedNode ) {
    screenNode = namedNode;
  }

  TiXmlNode* screenElement = NULL;

  // <center x="float" y="float" z="float" />
  if( (screenElement = screenNode->FirstChild( "center" )) ) {
    arVector3 vec = _attributearVector3( screenElement );
    screen.setCenter( vec );
  }

  // <normal x="float" y="float" z="float" />
  if( (screenElement = screenNode->FirstChild( "normal" )) ) {
    arVector3 vec = _attributearVector3( screenElement );
    screen.setNormal( vec );
  }

  // <up x="float" y="float" z="float" />
  if( (screenElement = screenNode->FirstChild( "up" )) ) {
    arVector3 vec = _attributearVector3( screenElement );
    screen.setUp( vec );
  }

  // <dim width="float" height="float" />
  if( (screenElement = screenNode->FirstChild( "dim" )) &&
      screenElement->ToElement() ) {
    float dim[ 2 ] = { 0.0f };
    screenElement->ToElement()->Attribute( "width",  &dim[ 0 ] );
    screenElement->ToElement()->Attribute( "height", &dim[ 1 ] );

    screen.setDimensions( dim[ 0 ], dim[ 1 ] );
  }

  // <headmounted value="true|false|yes|no" />
  if( (screenElement = screenNode->FirstChild( "headmounted" )) ) {
    screen.setHeadMounted( _attributeBool( screenElement ) );
  }

  // <tile tilex="integer" numtilesx="integer" tiley="integer" numtilesy="integer" />
  if( (screenElement = screenNode->FirstChild( "tile" )) ) {
    arVector4 vec = _attributearVector4( screenElement, "tilex", "numtilesx",
                                                        "tiley", "numtilesy" );
    screen.setTile( vec );
  }

  // <usefixedhead value="allow|always|ignore" />
  if( (screenElement = screenNode->FirstChild( "usefixedhead" )) &&
      screenElement->ToElement() &&
      screenElement->ToElement()->Attribute( "value" ) ) {
    screen.setUseFixedHeadMode( screenElement->ToElement()->Attribute( "value" ) );
  }

  // <fixedheadpos x="float" y="float" z="float" />
  if( (screenElement = screenNode->FirstChild( "fixedheadpos" )) ) {
    arVector3 vec = _attributearVector3( screenElement );
    screen.setFixedHeadPosition( vec );
  }

  // <fixedheadupangle value="float" />
  if( (screenElement = screenNode->FirstChild( "fixedheadupangle" )) &&
      screenElement->ToElement() ) {
    float angle;
    screenElement->ToElement()->Attribute( "value", &angle );

    screen.setFixedHeadHeadUpAngle( angle );
  }

  if( namedNode ) {
    delete namedNode;
  }

  return 0;
}

arCamera* arGUIXMLParser::_configureCamera( arGraphicsScreen& screen,
                                            TiXmlNode* cameraNode )
{
  // caller owns this and should delete it
  arCamera* camera = NULL;

  if( !cameraNode || !cameraNode->ToElement() ) {
    // not necessarily an error, the camera node could legitimately not exist
    // in which case a default camera should be returned
    std::cout << "NULL or invalid cameraNode, creating default arVRCamera (with default screen)" << std::endl;
    camera = new arVRCamera();
    return camera;
  }

  std::cout << "configuring camera" << std::endl;

  // check if this is a pointer to another camera
  TiXmlNode* namedNode = _getNamedNode( cameraNode->ToElement()->Attribute( "usenamed" ) );
  if( namedNode ) {
    cameraNode = namedNode;
  }

  TiXmlNode* cameraElement = NULL;

  std::string cameraType( "vr" );

  if( cameraNode->ToElement()->Attribute( "type" ) ) {
    cameraType = cameraNode->ToElement()->Attribute( "type" );
  }

  if( _configureScreen( screen, cameraNode->FirstChild( "szg_screen" ) ) < 0 ) {
    // print warning, return default camera + screen
  }

  if( cameraType == "ortho" || cameraType == "perspective" ) {
    std::cout << "creating " << cameraType << " camera" << std::endl;

    if( cameraType == "ortho" ) {
      camera = new arOrthoCamera();
    }
    else {
      camera = new arPerspectiveCamera();
    }

    if( (cameraElement = cameraNode->FirstChild( "frustum" )) &&
        cameraElement->ToElement() ) {
      float ortho[ 6 ] = { 0.0f };
      cameraElement->ToElement()->Attribute( "left",   &ortho[ 0 ] );
      cameraElement->ToElement()->Attribute( "right",  &ortho[ 1 ] );
      cameraElement->ToElement()->Attribute( "bottom", &ortho[ 2 ] );
      cameraElement->ToElement()->Attribute( "top",    &ortho[ 3 ] );
      cameraElement->ToElement()->Attribute( "near",   &ortho[ 4 ] );
      cameraElement->ToElement()->Attribute( "far",    &ortho[ 5 ] );

      if( cameraType == "ortho" ) {
        ((arOrthoCamera*) camera)->setFrustum( ortho );
      }
      else {
        ((arPerspectiveCamera*) camera)->setFrustum( ortho );
      }
    }

    if( (cameraElement = cameraNode->FirstChild( "lookat" )) &&
        cameraElement->ToElement() ) {
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

      if( cameraType == "ortho" ) {
        ((arOrthoCamera*) camera)->setLook( look );
      }
      else {
        ((arPerspectiveCamera*) camera)->setLook( look );
      }
    }

    if( (cameraElement = cameraNode->FirstChild( "sides" )) ) {
      arVector4 vec = _attributearVector4( cameraElement, "left", "right", "bottom", "sides" );

      if( cameraType == "ortho" ) {
        ((arOrthoCamera*) camera)->setSides( vec );
      }
      else {
        ((arPerspectiveCamera*) camera)->setSides( vec );
      }
    }

    if( (cameraElement = cameraNode->FirstChild( "clipping" )) &&
         cameraElement->ToElement() ) {
      float planes[ 2 ] = { 0.0f };
      cameraElement->ToElement()->Attribute( "near", &planes[ 0 ] );
      cameraElement->ToElement()->Attribute( "far",  &planes[ 1 ] );

      if( cameraType == "ortho" ) {
        ((arOrthoCamera*) camera)->setNearFar( planes[ 0 ], planes[ 1 ] );
      }
      else {
        ((arPerspectiveCamera*) camera)->setNearFar( planes[ 0 ], planes[ 1 ] );
      }
    }

    if( (cameraElement = cameraNode->FirstChild( "position" )) ) {
      arVector3 vec = _attributearVector3( cameraElement );

      if( cameraType == "ortho" ) {
        ((arOrthoCamera*) camera)->setPosition( vec );
      }
      else {
        ((arPerspectiveCamera*) camera)->setPosition( vec );
      }
    }

    if( (cameraElement = cameraNode->FirstChild( "target" )) ) {
      arVector3 vec = _attributearVector3( cameraElement );

      if( cameraType == "ortho" ) {
        ((arOrthoCamera*) camera)->setTarget( vec );
      }
      else {
        ((arPerspectiveCamera*) camera)->setTarget( vec );
      }
    }

    if( (cameraElement = cameraNode->FirstChild( "up" )) ) {
      arVector3 vec = _attributearVector3( cameraElement );

      if( cameraType == "ortho" ) {
        ((arOrthoCamera*) camera)->setUp( vec );
      }
      else {
        ((arPerspectiveCamera*) camera)->setUp( vec );
      }
    }
  }
  else if( cameraType == "vr" ) {
    std::cout << "creating vr camera" << std::endl;
    camera = new arVRCamera();
  }
  else {
    std::cout << "unknown camera type, creating default arVRCamera" << std::endl;
    camera = new arVRCamera();
  }

  if( namedNode ) {
    delete namedNode;
  }

  camera->setScreen( &screen );

  return camera;
}

int arGUIXMLParser::parse( void )
{
  if( _doc.Error() ) {
    std::cout << "error in parsing gui config xml on line: " << _doc.ErrorRow() << std::endl;
    return -1;
  }

  std::cout << "parsing gui config xml: " << std::endl;
  _doc.Print();

  // clear out any previous parsings
  // the graphicsWindow's and drawcallback's are externally owned, but this
  // *will* cause a leak of the windowconfig's (and obviously the
  // arGUIWindowConstruct pointers themselves)
  _parsedWindowConstructs.clear();

  // get a reference to <szg_display>
  TiXmlNode* szgDisplayNode = _doc.FirstChild();

  if( !szgDisplayNode || !szgDisplayNode->ToElement() ) {
    std::cout << "malformed <szg_display> node" << std::endl;
    return -1;
  }

  // Before any windows are created, set the threading mode and framelock

  // <threaded value="true|false|yes|no" />
  if( szgDisplayNode->ToElement()->Attribute( "threaded" ) ) {
    _windowingConstruct->setThreaded( _attributeBool( szgDisplayNode->ToElement(), "threaded" ) ? 1 : 0 );
  }

  // <framelock value="wildcat" />
  if( szgDisplayNode->ToElement()->Attribute( "framelock" ) ) {
    std::string framelock = szgDisplayNode->ToElement()->Attribute( "framelock" );
    _windowingConstruct->setUseFramelock( framelock == "wildcat" ? 1 : 0 );
  }

  // iterate over all <szg_window> elements
  for( TiXmlNode* windowNode = szgDisplayNode->FirstChild( "szg_window" ); windowNode;
       windowNode = windowNode->NextSibling() ) {
    std::cout << "parsing window" << std::endl;

    TiXmlNode* windowElement = NULL;

    // save the current position in the list of <szg_window> siblings (to be
    // restored if this windowNode is a 'pointer')
    TiXmlNode* savedWindowNode = windowNode;

    // check if this is a pointer to another window
    TiXmlNode* namedWindowNode = _getNamedNode( windowNode->ToElement()->Attribute( "usenamed" ) );
    if( namedWindowNode ) {
      windowNode = namedWindowNode;
    }

    if( !windowNode->ToElement() ) {
      std::cout << "Invalid window, skipping" << std::endl;
      continue;
    }

    arGUIWindowConfig* windowConfig = new arGUIWindowConfig();

    // <size width="integer" height="integer" />
    if( (windowElement = windowNode->FirstChild( "size" )) &&
         windowElement->ToElement() ) {
      int width, height;
      windowElement->ToElement()->Attribute( "width",  &width );
      windowElement->ToElement()->Attribute( "height", &height );
      windowConfig->setSize( width, height );
    }

    // <position x="integer" y="integer" />
    if( (windowElement = windowNode->FirstChild( "position" )) &&
         windowElement->ToElement() ) {
      int x, y;
      windowElement->ToElement()->Attribute( "x", &x );
      windowElement->ToElement()->Attribute( "y", &y );
      windowConfig->setPos( x, y );
    }

    // <fullscreen value="true|false|yes|no" />
    if( (windowElement = windowNode->FirstChild( "fullscreen" )) ) {
      windowConfig->setFullscreen( _attributeBool( windowElement ) );
    }

    // <decorate value="true|false|yes|no" />
    if( (windowElement = windowNode->FirstChild( "decorate" )) ) {
      windowConfig->setDecorate( _attributeBool( windowElement ) );
    }

    // <stereo value="true|false|yes|no" />
    if( (windowElement = windowNode->FirstChild( "stereo" )) ) {
      windowConfig->setStereo( _attributeBool( windowElement ) );
    }

    // <zorder value="normal|top|topmost" />
    if( (windowElement = windowNode->FirstChild( "zorder" )) &&
         windowElement->ToElement() &&
         windowElement->ToElement()->Attribute( "value" ) ) {
      std::string zorder = windowElement->ToElement()->Attribute( "value" );

      arZOrder arzorder;
      if( zorder == "normal" ) {
        arzorder = AR_ZORDER_NORMAL;
      }
      else if( zorder == "top" ) {
        arzorder = AR_ZORDER_TOP;
      }
      else if( zorder == "topmost" ) {
        arzorder = AR_ZORDER_TOPMOST;
      }

      windowConfig->setZOrder( arzorder );
    }

    // <bpp value="integer" />
    if( (windowElement = windowNode->FirstChild( "bpp" )) &&
        windowElement->ToElement() ) {
      int bpp;
      windowElement->ToElement()->Attribute( "value", &bpp );
      windowConfig->setBpp( bpp );
    }

    // <title value="string" />
    if( (windowElement = windowNode->FirstChild( "title" )) &&
         windowElement->ToElement() &&
         windowElement->ToElement()->Attribute( "value" ) ) {
      windowConfig->setTitle( windowElement->ToElement()->Attribute( "value" ) );
    }

    // <xdisplay value="string" />
    if( (windowElement = windowNode->FirstChild( "xdisplay" )) &&
        windowElement->ToElement() &&
        windowElement->ToElement()->Attribute( "value" ) ) {
      windowConfig->setXDisplay( windowElement->ToElement()->Attribute( "value" ) );
    }

    // <cursor value="arrow|none|help|wait" />
    if( (windowElement = windowNode->FirstChild( "cursor")) &&
        windowElement->ToElement() &&
        windowElement->ToElement()->Attribute( "value" ) ) {
      string initialCursor = windowElement->ToElement()->Attribute( "value" );

      arCursor cursor;
      if( initialCursor == "none" ) {
        cursor = AR_CURSOR_NONE;
      }
      else if( initialCursor == "help" ) {
        cursor = AR_CURSOR_HELP;
      }
      else if( initialCursor == "wait" ) {
        cursor = AR_CURSOR_WAIT;
      }
      else {
        // The default is for there to be an arrow cursor.
        cursor = AR_CURSOR_ARROW;
      }

      windowConfig->setCursor( cursor );
    }

    std::cout << "TITLE: " << windowConfig->getTitle() << std::endl;
    std::cout << "WIDTH: " << windowConfig->getWidth() << std::endl;
    std::cout << "HEIGHT: " << windowConfig->getHeight() << std::endl;
    std::cout << "X: " << windowConfig->getPosX() << std::endl;
    std::cout << "Y: " << windowConfig->getPosY() << std::endl;
    std::cout << "FULLSCREEN: " << windowConfig->getFullscreen() << std::endl;
    std::cout << "STEREO: " << windowConfig->getStereo() << std::endl;
    std::cout << "ZORDER: " << windowConfig->getZOrder() << std::endl;
    std::cout << "BPP: " << windowConfig->getBpp() << std::endl;
    std::cout << "XDISPLAY: " << windowConfig->getXDisplay() << std::endl;
    std::cout << "CURSOR: " << windowConfig->getCursor() << std::endl;

    _parsedWindowConstructs.push_back( new arGUIXMLWindowConstruct( windowConfig, new arGraphicsWindow(), NULL ) );

    _parsedWindowConstructs.back()->getGraphicsWindow()->useOGLStereo( windowConfig->getStereo() );

    std::string viewMode( "normal" );

    // run through all the viewports in the viewport list
    TiXmlNode* viewportListNode = windowNode->FirstChild( "szg_viewport_list" );
    TiXmlNode* namedViewportListNode = NULL;

    // possible not to have a <szg_viewport_list>, so the viewportListNode
    // pointer needs to be checked from here on out
    if( viewportListNode ) {
      // check if this is a pointer to another viewportlist
      namedViewportListNode = _getNamedNode( viewportListNode->ToElement()->Attribute( "usenamed" ) );

      if( namedViewportListNode ) {
        viewportListNode = namedViewportListNode;
      }

      if( !viewportListNode->ToElement() ) {
        std::cout << "invalid viewportlist" << std::endl;
        return -1;
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
        return -1;
      }

      // clear out the 'standard' viewport
      _parsedWindowConstructs.back()->getGraphicsWindow()->clearViewportList();

      // iterate over all <szg_viewport> elements
      for( viewportNode = viewportListNode->FirstChild( "szg_viewport" ); viewportNode;
           viewportNode = viewportNode->NextSibling( "szg_viewport" ) ) {
        std::cout << "creating viewport" << std::endl;
        TiXmlNode* savedViewportNode = viewportNode;

        // check if this is a pointer to another window
        TiXmlNode* namedViewportNode = _getNamedNode( viewportNode->ToElement()->Attribute( "usenamed" ) );
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

        if( !(camera = _configureCamera( screen, viewportNode->FirstChild( "szg_camera" ) )) ) {
          // should never happen, configureCamera should always return at least /something/
          std::cout << "custom configureCamera failure" << std::endl;
        }

        arViewport viewport;

        viewport.setCamera( camera );
        viewport.setScreen( screen );

        // fill in viewport parameters from viewportNode elements
        // <coords left="float" bottom="float" width="float" height="float" />
        if( (viewportElement = viewportNode->FirstChild( "coords" )) ) {
          arVector4 vec = _attributearVector4( viewportElement, "left", "bottom", "width", "height" );
          viewport.setViewport( vec );
        }

        // <depthclear value="true|false|yes|no" />
        if( (viewportElement = viewportNode->FirstChild( "depthclear" )) ) {
          viewport.clearDepthBuffer( _attributeBool( viewportElement ) );
        }

        // <colormask R="true|false|yes|no" G="true|false|yes|no" B="true|false|yes|no" A="true|false|yes|no" />
        if( (viewportElement = viewportNode->FirstChild( "colormask" )) ) {
          bool colorMask[ 4 ];
          colorMask[ 0 ] = _attributeBool( viewportElement, "R" );
          colorMask[ 1 ] = _attributeBool( viewportElement, "G" );
          colorMask[ 2 ] = _attributeBool( viewportElement, "B" );
          colorMask[ 3 ] = _attributeBool( viewportElement, "A" );
          viewport.setColorMask( colorMask[ 0 ], colorMask[ 1 ], colorMask[ 2 ], colorMask[ 3 ] );
        }

        // <eyesign value="float" />
        if( (viewportElement = viewportNode->FirstChild( "eyesign" )) &&
            viewportElement->ToElement() ) {
          float eyesign;
          viewportElement->ToElement()->Attribute( "value", &eyesign );
          viewport.setEyeSign( eyesign );
        }

        // <ogldrawbuf value="GL_NONE|GL_FRONT_LEFT|GL_FRONT_RIGHT|GL_BACK_LEFT|GL_BACK_RIGHT|GL_FRONT|GL_BACK|GL_LEFT|GL_RIGHT|GL_FRONT_AND_BACK" />
        if( (viewportElement = viewportNode->FirstChild( "ogldrawbuf" )) &&
            viewportElement->ToElement() ) {
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
        _parsedWindowConstructs.back()->getGraphicsWindow()->addViewport( viewport );

        // if viewportNode was a pointer, revert it back so that its
        // siblings can be traversed properly
        viewportNode = savedViewportNode;

        if( namedViewportNode ) {
          delete namedViewportNode;
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

      if( !(camera = _configureCamera( screen,
                                       viewportListNode ? viewportListNode->FirstChild( "szg_camera" ) : NULL )) ) {
        // should never happen, configureCamera should always return at least /something/
        std::cout << "non-custom configureCamera failure" << std::endl;
      }

      // viewports added by setViewMode will use this camera and screen
      _parsedWindowConstructs.back()->getGraphicsWindow()->setCamera( camera );
      _parsedWindowConstructs.back()->getGraphicsWindow()->setScreen( screen );

      // set up the appropriate viewports
      if( !_parsedWindowConstructs.back()->getGraphicsWindow()->setViewMode( viewMode ) ) {
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

    if( namedWindowNode ) {
      delete namedWindowNode;
    }
    if( namedViewportListNode ) {
      delete namedViewportListNode;
    }
  }

  std::cout << "Done parsing" << std::endl;

  _windowingConstruct->setWindowConstructs( &_parsedWindowConstructs );

  return 0;
}
