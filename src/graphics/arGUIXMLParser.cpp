//********************************************************
// Syzygy is licensed under the BSD license v2
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
#include "arLogStream.h"
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
  if( _config == config )
    return;

  if( !config.length() || config == "NULL" ) {
    ar_log_remark() << "arGUIXML remark: defaulting to minimum config.\n";
    _config = _mininumConfig;
  }
  else
    _config = config;

  // NOTE: It is very important to clear the document first. Otherwise, this
  // new config string will just be appended to the end of old config strings,
  // which is NOT what is wanted.
  _doc.Clear();

  _doc.Parse( _config.c_str() );
  if (_doc.Error()) {
    _reportParseError( &_doc, _config );
  }
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

TiXmlNode* arGUIXMLParser::_getNamedNode( const char* name )
{
  if (!name || !_SZGClient)
    return NULL;

  // caller will own this and should delete it
  TiXmlDocument* nodeDoc = new TiXmlDocument();
  const std::string nodeDesc = _SZGClient->getGlobalAttribute( name );

  if( !nodeDesc.length() || nodeDesc == "NULL" ) {
    ar_log_error() << "arGUIXML error: node points to non-existent node: " << name << ar_endl;
    return NULL;
  }

  // create a usable node out of the xml string
  nodeDoc->Parse( nodeDesc.c_str() );
  if (nodeDoc->Error()) {
    _reportParseError( nodeDoc, nodeDesc );
  }
  if( !nodeDoc->FirstChild() ) {
    ar_log_error() << "arGUIXML error: invalid node pointer: " << name << ar_endl;
    return NULL;
  }

  return nodeDoc->FirstChild();
}

void arGUIXMLParser::_reportParseError( TiXmlDocument* nodeDoc, const std::string& nodeDesc ) {
  int rowNum = nodeDoc->ErrorRow()-1;
  int colNum = nodeDoc->ErrorCol()-1;
  std::string::size_type curPos(0);
  std::string errLine( nodeDesc );
  bool stat(false);
  std::vector< std::string > lines;
  std::string line;
  istringstream ist;
  ist.str( nodeDesc );
  while (getline( ist, line, '\n' )) {
    lines.push_back( line );
  }
  if (lines.size() > rowNum) {
    errLine = lines[rowNum];
    stat = true;
  }
  ar_log_error() << "arGUIXMLParser: TiXmlDocument reported the following error:\n\t"
    << nodeDoc->ErrorDesc() << "\n";
  if (stat) {
    ar_log_error() << "In the following line:\n\t" << errLine << "\n";
  }
  ar_log_error() << "(Use the arg sequence '-szg log=DEBUG' to see the whole XML chunk).\n";
  ar_log_debug() << "Somewhere in the following XML:\n\t" << nodeDesc << ar_endl;
  ar_log_debug() << "(TiXmlDocument isn't always good at localizing errors).\n";
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
                                      TiXmlNode* screenNode )
{
  if( !screenNode || !screenNode->ToElement() ) {
    // not necessarily an error, <szg_screen> could legitimately not exist and
    // in that case let the caller use the screen as it was passed in
    ar_log_remark() << "arGUIXML remark: ignoring missing screen description.\n";
    return 0;
  }

  // check if this is a pointer to another screen
  TiXmlNode* namedNode = _getNamedNode( screenNode->ToElement()->Attribute( "usenamed" ) );
  if( namedNode ) {
    ar_log_debug() << "arGUIXML remark: using named screen "
                    << screenNode->ToElement()->Attribute( "usenamed" ) << ar_endl;
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
  // caller owns return value and should delete it

  if( !cameraNode || !cameraNode->ToElement() ) {
    // not necessarily an error, the camera node could legitimately not exist
    // in which case a default camera should be returned
    ar_log_remark() << "arGUIXML remark: defaulting to arVRCamera for missing cameraNode.\n";
    return new arVRCamera();
  }

  // check if this is a pointer to another camera
  TiXmlNode* namedNode = _getNamedNode( cameraNode->ToElement()->Attribute( "usenamed" ) );
  if( namedNode ) {
    ar_log_debug() << "arGUIXML remark: using named camera "
                    << cameraNode->ToElement()->Attribute( "usenamed" ) << ".\n";
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

  arCamera* camera = NULL;
  if( cameraType == "vr" ) {
    camera = new arVRCamera();
  }
  else if( cameraType == "ortho" || cameraType == "perspective" ) {
    arFrustumCamera* camF = new arFrustumCamera();

    // <frustum left="float" right="float" bottom="float" top="float" near="float" far="float" />
    if( (cameraElement = cameraNode->FirstChild( "frustum" )) &&
        cameraElement->ToElement() ) {
      float ortho[ 6 ] = { 0.0f };
      cameraElement->ToElement()->Attribute( "left",   &ortho[ 0 ] );
      cameraElement->ToElement()->Attribute( "right",  &ortho[ 1 ] );
      cameraElement->ToElement()->Attribute( "bottom", &ortho[ 2 ] );
      cameraElement->ToElement()->Attribute( "top",    &ortho[ 3 ] );
      cameraElement->ToElement()->Attribute( "near",   &ortho[ 4 ] );
      cameraElement->ToElement()->Attribute( "far",    &ortho[ 5 ] );
      camF->setFrustum( ortho );
    }

    // <lookat viewx="float" viewy="float" viewz="float" lookatx="float" lookaty="float" lookatz="float" upx="float" upy="float" upz="float" />
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
      camF->setLook( look );
    }

    // <sides left="float" right="float" bottom="float" top="float" />
    if( (cameraElement = cameraNode->FirstChild( "sides" )))
      camF->setSides( _attributearVector4( cameraElement, "left", "right", "bottom", "top" ) );

    // <clipping near="float" far="float" />
    if( (cameraElement = cameraNode->FirstChild( "clipping" )) &&
         cameraElement->ToElement() ) {
      float planes[ 2 ] = { 0.0f };
      cameraElement->ToElement()->Attribute( "near", &planes[ 0 ] );
      cameraElement->ToElement()->Attribute( "far",  &planes[ 1 ] );
      camF->setNearFar( planes[ 0 ], planes[ 1 ] );
    }

    // <position x="float" y="float" z="float" />
    if( (cameraElement = cameraNode->FirstChild( "position" )) )
      camF->setPosition( _attributearVector3( cameraElement ) );

    // <target x="float" y="float" z="float" />
    if( (cameraElement = cameraNode->FirstChild( "target" )) )
      camF->setTarget( _attributearVector3( cameraElement ) );

    // <up x="float" y="float" z="float" />
    if( (cameraElement = cameraNode->FirstChild( "up" )) )
      camF->setUp( _attributearVector3( cameraElement ) );

    camera = camF;
  }
  else {
    ar_log_error() << "arGUIXML warning: defaulting to arVRCamera for unknown camera type \""
                     << cameraType << "\"\n";
    camera = new arVRCamera();
  }

  if (namedNode)
    delete namedNode;

  camera->setScreen( &screen );
  return camera;
}

int arGUIXMLParser::parse( void )
{
  //  Should have already complained about any errors.
//  if( _doc.Error() ) {
//    ar_log_error() << "arGUIXML error: failed to parse at line " << _doc.ErrorRow() << ar_endl;
//    return -1;
//  }

  // clear out any previous parsing constructs
  // the graphicsWindow's and drawcallback's are externally owned, but this
  // *will* cause a leak of the windowconfig's (and obviously the
  // arGUIWindowConstruct pointers themselves)
  _parsedWindowConstructs.clear();

  // get a reference to <szg_display>
  TiXmlNode* szgDisplayNode = _doc.FirstChild();

  if( !szgDisplayNode || !szgDisplayNode->ToElement() ) {
    ar_log_error() << "arGUIXML error: malformed <szg_display> node.\n";
    return -1;
  }

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
    TiXmlNode* windowElement = NULL;

    // save the current position in the list of <szg_window> siblings (to be
    // restored if this windowNode is a 'pointer')
    TiXmlNode* savedWindowNode = windowNode;

    // is this a pointer to another window?
    TiXmlNode* namedWindowNode = _getNamedNode( windowNode->ToElement()->Attribute( "usenamed" ) );
    if( namedWindowNode ) {
      ar_log_debug() << "arGUIXML remark: using named window "
                      << windowNode->ToElement()->Attribute( "usenamed" ) << ".\n";
      windowNode = namedWindowNode;
    }

    if( !windowNode->ToElement() ) {
      ar_log_error() << "arGUIXML warning: skipping invalid window element.\n";
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
      const std::string zorder(windowElement->ToElement()->Attribute("value"));
      const arZOrder arzorder =
	(zorder == "normal") ? AR_ZORDER_NORMAL :
	(zorder == "topmost") ? AR_ZORDER_TOPMOST :
	/* default, if( zorder == "top" ) */ AR_ZORDER_TOP;

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

    _parsedWindowConstructs.push_back( new arGUIXMLWindowConstruct( windowConfig, new arGraphicsWindow(), NULL ) );

    // AARGH, this is annoying
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
        ar_log_debug() << "arGUIXML remark: using named viewportList "
                        << viewportListNode->ToElement()->Attribute( "usenamed" ) << ".\n";
        viewportListNode = namedViewportListNode;
      }

      if( !viewportListNode->ToElement() ) {
        ar_log_error() << "arGUIXML error: invalid viewportlist element.\n";
        return -1;
      }

      // determine which viewmode was specified, anything other than "custom"
      // doesn't need any viewports to actually be listed
      // <viewmode value="normal|anaglyph|walleyed|crosseyed|overunder|custom" />
      if( viewportListNode->ToElement()->Attribute( "viewmode" ) ) {
        viewMode = viewportListNode->ToElement()->Attribute( "viewmode" );
      }
    }

    if( viewMode == "custom" ) {
      TiXmlNode* viewportNode = NULL;

      // if the user specified a viewmode of 'custom' but then didn't specify
      // any <szg_viewport>'s - that is an error! (the only way we can get here
      // is if viewportListNode /does/ exist, no need to check it again)
      if( !(viewportNode = viewportListNode->FirstChild( "szg_viewport" ) ) ) {
        // malformed!, delete currentwindow, print warning, continue with next window tag
        ar_log_error() << "arGUIXML error: viewmode is custom, but no <szg_viewport> tags.\n";
        return -1;
      }

      // clear out the 'standard' viewport
      _parsedWindowConstructs.back()->getGraphicsWindow()->clearViewportList();

      // iterate over all <szg_viewport> elements
      for( viewportNode = viewportListNode->FirstChild( "szg_viewport" ); viewportNode;
           viewportNode = viewportNode->NextSibling( "szg_viewport" ) ) {
        TiXmlNode* savedViewportNode = viewportNode;

        // check if this is a pointer to another viewport
        TiXmlNode* namedViewportNode = _getNamedNode( viewportNode->ToElement()->Attribute( "usenamed" ) );
        if( namedViewportNode ) {
          ar_log_debug() << "arGUIXML remark: using named viewport "
                          << viewportNode->ToElement()->Attribute( "usenamed" ) << ".\n";
          viewportNode = namedViewportNode;
        }

        if( !viewportNode->ToElement() ) {
          ar_log_error() << "arGUIXML warning: skipping invalid viewport element.\n";
          continue;
        }

        TiXmlNode* viewportElement = NULL;

        // configure the camera and possibly the camera's screen
        arCamera* camera = NULL;
        arGraphicsScreen screen;

        if( !(camera = _configureCamera( screen, viewportNode->FirstChild( "szg_camera" ) )) ) {
          // should never happen, configureCamera should always return at least /something/
          ar_log_error() << "arGUIXML warning: custom configureCamera failed.\n";
        }

        arViewport viewport;

        viewport.setCamera( camera );
        viewport.setScreen( screen );

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

          const std::string buf(viewportElement->ToElement()->Attribute( "value" ));
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
      arGraphicsScreen screen;

      arCamera* camera = _configureCamera(
        screen, viewportListNode ? viewportListNode->FirstChild( "szg_camera" ) : NULL );
      if (!camera) {
        // should never happen, configureCamera should always return at least /something/
        ar_log_error() << "arGUIXML warning: configureCamera failed.\n";
      }

      // viewports added by setViewMode will use this camera and screen
      _parsedWindowConstructs.back()->getGraphicsWindow()->setCamera( camera );
      _parsedWindowConstructs.back()->getGraphicsWindow()->setScreen( screen );

      // set up the appropriate viewports
      if( !_parsedWindowConstructs.back()->getGraphicsWindow()->setViewMode( viewMode ) ) {
        ar_log_error() << "arGUIXML warning: setViewMode failed.\n";
      }

      if( camera ) {
        // setCamera copied and now owns the camera, so safe to delete
        delete camera;
      }
    }

    // if windowNode was changed because it was a 'pointer' to another window,
    // revert it back to its position in the list of sibling windows for this
    // <szg_display>
    windowNode = savedWindowNode;

    if (namedWindowNode)
      delete namedWindowNode;
    if (namedViewportListNode)
      delete namedViewportListNode;
  }

  ar_log_remark() << "arGUIXML remark: parsed.\n";
  _windowingConstruct->setWindowConstructs( &_parsedWindowConstructs );
  return 0;
}
