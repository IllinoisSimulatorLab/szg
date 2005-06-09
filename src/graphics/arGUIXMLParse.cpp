//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for detils
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGUIXMLParse.h"
#include "arVRCamera.h"
#include "arPerspectiveCamera.h"
#include "arOrthoCamera.h"

std::string minimumConfig( "<szg_display><szg_window /></szg_display>" );

TiXmlNode* getNamedNode( arSZGClient& SZGClient, const char* name )
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
                              const std::string& x, const std::string& y,
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

arVector4 AttributearVector4( TiXmlNode* node,
                              const std::string& x, const std::string& y,
                              const std::string& z, const std::string& w )
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

bool AttributeBool( TiXmlNode* node, const std::string& value )
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

int configureScreen( arSZGClient& SZGClient, arGraphicsScreen& screen,
                     TiXmlNode* screenNode )
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

arCamera* configureCamera( arSZGClient& SZGClient, arGraphicsScreen& screen,
                           TiXmlNode* cameraNode )
{
  // caller owns this and should delete it
  arCamera* camera;

  if( !cameraNode || !cameraNode->ToElement() ) {
    // not necessarily an error, the camera node could legitimately not exist
    // in which case a default camera should be returned
    std::cout << "NULL or invalid cameraNode, creating default arVRCamera (with default screen)" << std::endl;
    camera = new arVRCamera();
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
    camera = new arVRCamera();
  }
  else {
    std::cout << "unknown camera type, creating default arVRCamera" << std::endl;
    camera = new arVRCamera();
  }

  if( namedNode && namedNode->GetDocument() ) {
    delete namedNode->GetDocument();
  }

  camera->setScreen( &screen );

  return camera;
}

int parseGUIXML( arGUIWindowManager* wm,
                 std::map<int, arGraphicsWindow* >& windows,
                 arSZGClient& SZGClient,
                 const std::string& config )
{
  std::cout << "Parsing GUI XML config: " << std::endl
            << config << std::endl << std::endl;

  TiXmlDocument doc;

  if( config == "NULL" ) {
    std::cout << "config == NULL, using minimum config" << std::endl;
    doc.Parse( minimumConfig.c_str() );
  }
  else {
    doc.Parse( config.c_str() );
  }

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
      std::cout << "Invalid window, skipping" << std::endl;
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

  std::cout << "Done parsing" << std::endl;

  return 0;
}