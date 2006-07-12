//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

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

class arGUIXMLValidator {
  public:
    arGUIXMLValidator( const string& name ) : _nodeTypeName(name) {}
    virtual ~arGUIXMLValidator() {
      _attribsVec.clear();
      _childrenVec.clear();
    }
    void addChild( const string& childStr ) { _childrenVec.push_back( childStr ); }
    void addChildren( const arSlashString& childrenStr );
    void addAttributes( const arSlashString& attribsStr );
    bool operator()( TiXmlNode* node );
  protected:
    bool _validateNodeAttributes( TiXmlNode* node );
    bool _validateNodeChildren( TiXmlNode* node );
    string _nodeTypeName;
    vector< string > _attribsVec;
    vector< string > _childrenVec;
};

void arGUIXMLValidator::addAttributes( const arSlashString& attribsStr ) {
  int i;
  for (i=0; i<attribsStr.size(); ++i) {
    _attribsVec.push_back( attribsStr[i] );
  }
}

void arGUIXMLValidator::addChildren( const arSlashString& childrenStr ) {
  int i;
  for (i=0; i<childrenStr.size(); ++i) {
    _childrenVec.push_back( childrenStr[i] );
  }
}

bool arGUIXMLValidator::operator()( TiXmlNode* node ) {
  if (!node) {
    ar_log_debug() << "arGUIXML skipping NULL " << _nodeTypeName << " node.\n";
    return false;
  }
  return _validateNodeAttributes( node ) && 
    _validateNodeChildren( node );
}

bool arGUIXMLValidator::_validateNodeAttributes( TiXmlNode* node ) {
  bool ok = true;
  for (TiXmlAttribute* att = node->ToElement()->FirstAttribute(); att; att = att->Next() ) {
    string name( att->Name() );
    if (find( _attribsVec.begin(), _attribsVec.end(), name ) == _attribsVec.end()) {
      ar_log_warning() << "arGUIXML: unknown attribute '"
                     << name << "' in " << _nodeTypeName << " node.\n"
                     << "\tLegal attributes are:";
      vector< string >::const_iterator iter;
      for (iter = _attribsVec.begin(); iter != _attribsVec.end(); ++iter) {
        ar_log_warning() << " " << *iter;
      }
      ar_log_warning() << ar_endl;
      ok = false;
    }
  }
  return ok;
}

bool arGUIXMLValidator::_validateNodeChildren( TiXmlNode* node ) {
  bool ok = true;
  for (TiXmlNode* child = node->FirstChild(); child; child = child->NextSibling() ) {
    string name( child->Value() );
    if (find( _childrenVec.begin(), _childrenVec.end(), name ) == _childrenVec.end()) {
      ar_log_warning() << "arGUIXML: unknown sub-node '"
                     << name << "' in " << _nodeTypeName << " node.\n"
                     << "\tLegal sub-nodes are:";
      vector< string >::const_iterator iter;
      for (iter = _childrenVec.begin(); iter != _childrenVec.end(); ++iter) {
        ar_log_warning() << " " << *iter;
      }
      ar_log_warning() << ar_endl;
      ok = false;
    }
  }
  return ok;
}

class arGUIXMLDisplayValidator: public arGUIXMLValidator {
  public:
    arGUIXMLDisplayValidator();
};
arGUIXMLDisplayValidator::arGUIXMLDisplayValidator() :
  arGUIXMLValidator("display") {
  addAttributes( "threaded/framelock" );
  addChildren( "szg_window" );
}


class arGUIXMLWindowValidator: public arGUIXMLValidator {
  public:
    arGUIXMLWindowValidator();
};
arGUIXMLWindowValidator::arGUIXMLWindowValidator() :
  arGUIXMLValidator("window") {
  addAttributes( "usenamed" );
  addChildren( "size/position/fullscreen/decorate/stereo/zorder/bpp/title/xdisplay/cursor/szg_viewport_list" );
}


class arGUIXMLViewportListValidator: public arGUIXMLValidator {
  public:
    arGUIXMLViewportListValidator();
};
arGUIXMLViewportListValidator::arGUIXMLViewportListValidator() :
  arGUIXMLValidator("viewportlist") {
  addAttributes( "usenamed/viewmode" );
}


class arGUIXMLViewportValidator: public arGUIXMLValidator {
  public:
    arGUIXMLViewportValidator();
};
arGUIXMLViewportValidator::arGUIXMLViewportValidator() :
  arGUIXMLValidator("viewport") {
  addAttributes( "usenamed" );
  addChildren( "szg_camera/coords/depthclear/colormask/eyesign/ogldrawbuf" );
}


class arGUIXMLScreenValidator: public arGUIXMLValidator {
  public:
    arGUIXMLScreenValidator();
};
arGUIXMLScreenValidator::arGUIXMLScreenValidator() :
  arGUIXMLValidator("screen") {
  addAttributes( "usenamed" );
  addChildren( "center/normal/up/dim/headmounted/tile/usefixedhead/fixedheadpos/fixedheadupangle" );
}


class arGUIXMLCameraValidator: public arGUIXMLValidator {
  public:
    arGUIXMLCameraValidator();
};
arGUIXMLCameraValidator::arGUIXMLCameraValidator() :
  arGUIXMLValidator("camera") {
  addAttributes( "usenamed/type" );
  addChildren( "szg_screen" );
}


class arGUIXMLFrustumCameraValidator: public arGUIXMLValidator {
  public:
    arGUIXMLFrustumCameraValidator();
};
arGUIXMLFrustumCameraValidator::arGUIXMLFrustumCameraValidator() :
  arGUIXMLValidator("frustum") {
  addAttributes( "left/right/bottom/top/near/far" );
}


class arGUIXMLLookatCameraValidator: public arGUIXMLValidator {
  public:
    arGUIXMLLookatCameraValidator();
};
arGUIXMLLookatCameraValidator::arGUIXMLLookatCameraValidator() :
  arGUIXMLValidator("lookat") {
  addAttributes( "viewx/viewy/viewz/lookatx/lookaty/lookatz/upx/upy/upz" );
}

class arGUIXMLVector2Validator: public arGUIXMLValidator {
  public:
    arGUIXMLVector2Validator( const string& name,
                              const string& x="x",
                              const string& y="y" );
};
arGUIXMLVector2Validator::arGUIXMLVector2Validator(
                              const string& name,
                              const string& x,
                              const string& y ) :
  arGUIXMLValidator(name) {
  addAttributes( x+"/"+y );
}


class arGUIXMLVector3Validator: public arGUIXMLValidator {
  public:
    arGUIXMLVector3Validator( const string& name,
                              const string& x,
                              const string& y,
                              const string& z );
};
arGUIXMLVector3Validator::arGUIXMLVector3Validator(
                              const string& name,
                              const string& x,
                              const string& y,
                              const string& z ) :
  arGUIXMLValidator(name) {
  addAttributes( x+"/"+y+"/"+z );
}


class arGUIXMLVector4Validator: public arGUIXMLValidator {
  public:
    arGUIXMLVector4Validator( const string& name,
                              const string& x,
                              const string& y,
                              const string& z,
                              const string& w );
};
arGUIXMLVector4Validator::arGUIXMLVector4Validator(
                              const string& name,
                              const string& x,
                              const string& y,
                              const string& z,
                              const string& w ) :
  arGUIXMLValidator(name) {
  addAttributes( x+"/"+y+"/"+z+"/"+w );
}


class arGUIXMLValueValidator: public arGUIXMLValidator {
  public:
    arGUIXMLValueValidator( const string& name );
};
arGUIXMLValueValidator::arGUIXMLValueValidator( const string& name ) :
  arGUIXMLValidator(name) {
  addAttributes( "value" );
}


class arGUIXMLAttributeValueValidator {
  public:
    arGUIXMLAttributeValueValidator( const string& nodeName, const arSlashString& valuesStr );
    void addValues( const arSlashString& valuesStr );
    bool operator()( const string& valueStr );
  protected:
    string _nodeName;
    vector< string > _valuesVec;
};
arGUIXMLAttributeValueValidator::arGUIXMLAttributeValueValidator( 
  const string& nodeName,
  const arSlashString& valuesStr) :
  _nodeName( nodeName ) {
    addValues( valuesStr );
  }
void arGUIXMLAttributeValueValidator::addValues( const arSlashString& valuesStr ) {
  for (int i=0; i<valuesStr.size(); ++i) {
    _valuesVec.push_back( valuesStr[i] );
  }
}
bool arGUIXMLAttributeValueValidator::operator()( const string& valueStr ) {
  if (find( _valuesVec.begin(), _valuesVec.end(), valueStr ) != _valuesVec.end())
    return true;

  ar_log_warning() << "arGUIXML: invalid value '"
		 << valueStr << "' in " << _nodeName << " attribute.\n"
		 << "\tLegal values are:";
  vector< string >::const_iterator iter;
  for (iter = _valuesVec.begin(); iter != _valuesVec.end(); ++iter) {
    ar_log_warning() << " " << *iter;
  }
  ar_log_warning() << ar_endl;
  return false;
}


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
  // Depending on how windows have been reloaded and who copied
  // around _graphisWindow, this may be unsafe.
  delete _graphicsWindow;
  delete _windowConfig;
}

arGUIWindowingConstruct::arGUIWindowingConstruct( int threaded, int useFramelock,
                                                  vector< arGUIXMLWindowConstruct* >* windowConstructs ) :
  _threaded( threaded ),
  _useFramelock( useFramelock ),
  _windowConstructs( windowConstructs )
{
}

arGUIWindowingConstruct::~arGUIWindowingConstruct( void )
{
}

arGUIXMLParser::arGUIXMLParser( arSZGClient* SZGClient,
                                const string& config ) :
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

void arGUIXMLParser::setConfig( const string& config )
{
  if( _config == config )
    return;

  if( !config.length() || config == "NULL" ) {
    ar_log_remark() << "arGUIXML defaulting to minimum config.\n";
    _config = _mininumConfig;
  }
  else
    _config = config;

  // Don't just append the new config string to the old config strings.
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

TiXmlNode* arGUIXMLParser::_getNamedNode( const char* name, const string& nodeType )
{
  if (!name || !_SZGClient)
    return NULL;

  // caller will own this and should delete it
  TiXmlDocument* nodeDoc = new TiXmlDocument();
  const string nodeDesc = _SZGClient->getGlobalAttribute( name );

  if( !nodeDesc.length() || nodeDesc == "NULL" ) {
    ar_log_warning() << "arGUIXML: non-existent 'usenamed' node: " << name << ar_endl;
    return NULL;
  }

  // create a usable node out of the xml string
  nodeDoc->Parse( nodeDesc.c_str() );
  if (nodeDoc->Error()) {
    _reportParseError( nodeDoc, nodeDesc );
  }
  if( !nodeDoc->FirstChild() ) {
    ar_log_warning() << "arGUIXML: invalid node pointer: " << name << ar_endl;
    return NULL;
  }
  string nodeTypeName( nodeDoc->FirstChild()->Value() );
  if (nodeTypeName != nodeType) {
    ar_log_warning() << "arGUIXML: " << nodeType << " 'usenamed=" << name << "' "
                   << "\n\trefers to a record of type " << nodeTypeName
                   << ", not " << nodeType << "." << ar_endl;
    return NULL;
  }

  return nodeDoc->FirstChild();
}

void arGUIXMLParser::_reportParseError( TiXmlDocument* nodeDoc, const string& nodeDesc ) {
  unsigned rowNum = nodeDoc->ErrorRow()-1;
  //UNUSED int colNum = nodeDoc->ErrorCol()-1;
  //UNUSED string::size_type curPos(0);
  string errLine( nodeDesc );
  vector< string > lines;
  string line;
  istringstream ist;
  ist.str( nodeDesc );
  while (getline( ist, line, '\n' )) {
    lines.push_back( line );
  }
  bool ok = false;
  if (lines.size() > rowNum) {
    errLine = lines[rowNum];
    ok = true;
  }
  ar_log_warning() << "arGUIXMLParser: " << nodeDoc->ErrorDesc() << "\n";
  if (ok) {
    ar_log_warning() << "in line:  " << errLine << "\n";
  }
  ar_log_warning() << "(Use '-szg log=DEBUG' to see the whole XML chunk).\n";
  ar_log_debug() << "Somewhere in the following XML:\n\t" << nodeDesc << ar_endl;
  // The parser poorly localizes errors.
}

arVector3 arGUIXMLParser::_attributearVector3( TiXmlNode* node,
                                               const string& name,
                                               const string& x,
                                               const string& y,
                                               const string& z )
{
  arVector3 vec;
  if( !node || !node->ToElement() )
    return vec;

  arGUIXMLVector3Validator validator( name, x, y, z );
  validator( node );
  node->ToElement()->Attribute( x.c_str(), &vec[ 0 ] );
  node->ToElement()->Attribute( y.c_str(), &vec[ 1 ] );
  node->ToElement()->Attribute( z.c_str(), &vec[ 2 ] );
  return vec;
}

arVector4 arGUIXMLParser::_attributearVector4( TiXmlNode* node,
                                               const string& name,
                                               const string& x,
                                               const string& y,
                                               const string& z,
                                               const string& w )
{
  arVector4 vec;
  if( !node || !node->ToElement() )
    return vec;

  arGUIXMLVector4Validator validator( name, x, y, z, w );
  validator( node );
  node->ToElement()->Attribute( x.c_str(), &vec[ 0 ] );
  node->ToElement()->Attribute( y.c_str(), &vec[ 1 ] );
  node->ToElement()->Attribute( z.c_str(), &vec[ 2 ] );
  node->ToElement()->Attribute( w.c_str(), &vec[ 3 ] );
  return vec;
}

bool arGUIXMLParser::_attributeBool( TiXmlNode* node,
                                     const string& value )
{
  if( !node || !node->ToElement() )
    return false;

  // Don't pass NULL to strcmp.
  if( !node->ToElement()->Attribute( value.c_str() ) )
    return false;

  string attrVal( node->ToElement()->Attribute( value.c_str() ) );
  if ((attrVal != "yes")&&(attrVal != "no")&&(attrVal != "true")&&(attrVal != "false")) {
    ar_log_warning() << "arGUIXML: attribute '" << value << "' should be one of "
                   << "yes/no/true/false, but is instead '" << attrVal << "'.\n";
    return false;
  }

  return attrVal == "true" || attrVal == "yes";
}

int arGUIXMLParser::_configureScreen( arGraphicsScreen& screen,
                                      TiXmlNode* screenNode )
{
  if( !screenNode || !screenNode->ToElement() ) {
    // not necessarily an error, <szg_screen> could legitimately not exist and
    // in that case let the caller use the screen as it was passed in
    ar_log_remark() << "arGUIXML ignoring missing screen description.\n";
    return 0;
  }

  arGUIXMLScreenValidator screenValidator;
  
  // check if this is a pointer to another screen
  TiXmlNode* namedNode = _getNamedNode( screenNode->ToElement()->Attribute( "usenamed" ),
                                        "szg_screen" );
  if( namedNode ) {
    ar_log_debug() << "arGUIXML using named screen "
                    << screenNode->ToElement()->Attribute( "usenamed" ) << ar_endl;
    screenNode = namedNode;
  }

  TiXmlNode* screenElement = NULL;

  // <center x="float" y="float" z="float" />
  if( (screenElement = screenNode->FirstChild( "center" )) ) {
    arVector3 vec = _attributearVector3( screenElement, "screen 'center'" );
    screen.setCenter( vec );
  }

  // <normal x="float" y="float" z="float" />
  if( (screenElement = screenNode->FirstChild( "normal" )) ) {
    arVector3 vec = _attributearVector3( screenElement, "screen 'normal'" );
    screen.setNormal( vec );
  }

  // <up x="float" y="float" z="float" />
  if( (screenElement = screenNode->FirstChild( "up" )) ) {
    arVector3 vec = _attributearVector3( screenElement, "screen 'up'" );
    screen.setUp( vec );
  }

  // <dim width="float" height="float" />
  if( (screenElement = screenNode->FirstChild( "dim" )) &&
      screenElement->ToElement() ) {
    float dim[ 2 ] = { 0.0f };
    arGUIXMLVector2Validator dimVal( "screen 'dim'", "width", "height" );
    dimVal( screenElement );
    screenElement->ToElement()->Attribute( "width",  &dim[ 0 ] );
    screenElement->ToElement()->Attribute( "height", &dim[ 1 ] );

    screen.setDimensions( dim[ 0 ], dim[ 1 ] );
  }

  // <headmounted value="true|false|yes|no" />
  if( (screenElement = screenNode->FirstChild( "headmounted" )) ) {
    arGUIXMLValueValidator hmdValidator( "screen 'headmounted'" );
    hmdValidator( screenElement );
    screen.setHeadMounted( _attributeBool( screenElement ) );
  }

  // <tile tilex="integer" numtilesx="integer" tiley="integer" numtilesy="integer" />
  if( (screenElement = screenNode->FirstChild( "tile" )) ) {
    arVector4 vec = _attributearVector4( screenElement, "screen 'tile'",
                                         "tilex", "numtilesx",
                                         "tiley", "numtilesy" );
    screen.setTile( vec );
  }

  // <usefixedhead value="allow|always|ignore" />
  if( (screenElement = screenNode->FirstChild( "usefixedhead" )) &&
      screenElement->ToElement()) {
    arGUIXMLValueValidator useFixValidator( "screen 'usefixedhead'" );
    useFixValidator( screenElement );
    if (screenElement->ToElement()->Attribute( "value" ) ) {
      string useFixStr( screenElement->ToElement()->Attribute( "value" ) );
      arGUIXMLAttributeValueValidator fixHeadValValidator( "screen 'usefixedhead'", "allow/ignore/always" );
      fixHeadValValidator( useFixStr );
      screen.setUseFixedHeadMode( useFixStr );
    }
  }

  // <fixedheadpos x="float" y="float" z="float" />
  if( (screenElement = screenNode->FirstChild( "fixedheadpos" )) ) {
    arVector3 vec = _attributearVector3( screenElement, "screen 'fixedheadpos'" );
    screen.setFixedHeadPosition( vec );
  }

  // <fixedheadupangle value="float" />
  if( (screenElement = screenNode->FirstChild( "fixedheadupangle" )) &&
      screenElement->ToElement() ) {
    arGUIXMLValueValidator fixAngValidator( "screen 'fixedheadupangle'" );
    fixAngValidator( screenElement );
    float angle;
    screenElement->ToElement()->Attribute( "value", &angle );

    screen.setFixedHeadHeadUpAngle( angle );
  }

  if (!screenValidator( screenNode )) {
    ar_log_warning() << "arGUIXMLParser: invalid attribute or field in screen node.\n";
    return 0;
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
    ar_log_remark() << "arGUIXML defaulting to arVRCamera for missing cameraNode.\n";
    return new arVRCamera();
  }

  arGUIXMLCameraValidator cameraValidator;
  bool validation(true);
  
  // check if this is a pointer to another camera
  TiXmlNode* namedNode = _getNamedNode( cameraNode->ToElement()->Attribute( "usenamed" ),
                                        "szg_camera" );
  if( namedNode ) {
    string useNamed( cameraNode->ToElement()->Attribute( "usenamed" ) );
    ar_log_debug() << "arGUIXML using named camera " << useNamed << ".\n";
    cameraNode = namedNode;
  }

  TiXmlNode* cameraElement = NULL;
  string cameraType( "vr" );

  if( cameraNode->ToElement()->Attribute( "type" ) ) {
    cameraType = cameraNode->ToElement()->Attribute( "type" );
  }

  arGUIXMLAttributeValueValidator camTypeValidator( "camera 'type'", "vr/ortho/perspective" );
  camTypeValidator( cameraType );

  if( _configureScreen( screen, cameraNode->FirstChild( "szg_screen" ) ) < 0 ) {
    // print warning, return default camera + screen
  }

  arCamera* camera = NULL;
  if( cameraType == "vr" ) {
    camera = new arVRCamera();

  } else if (cameraType == "ortho" || cameraType == "perspective") {
    arFrustumCamera* camF;
    if (cameraType == "ortho") {
      camF = (arFrustumCamera*)new arOrthoCamera();
    } else {
      camF = (arFrustumCamera*)new arPerspectiveCamera();
    }
    cameraValidator.addChildren( "frustum/lookat/sides/clipping/position/target/up" );

    // <frustum left="float" right="float" bottom="float" top="float" near="float" far="float" />
    if( (cameraElement = cameraNode->FirstChild( "frustum" )) &&
        cameraElement->ToElement() ) {
      arGUIXMLFrustumCameraValidator frustumValidator;
      validation = frustumValidator( cameraElement );
      if (!validation) {
        ar_log_warning() << "arGUIXMLParser: invalid attribute or field in frustum node.\n";
      }
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
      arGUIXMLLookatCameraValidator lookatValidator;
      validation = lookatValidator( cameraElement );
      if (!validation) {
        ar_log_warning() << "arGUIXMLParser: invalid attribute or field in lookat node.\n";
      }
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
    if( (cameraElement = cameraNode->FirstChild( "sides" ))) {
      camF->setSides( _attributearVector4( cameraElement, "sides", "left", "right", "bottom", "top" ) );
    }

    // <clipping near="float" far="float" />
    if( (cameraElement = cameraNode->FirstChild( "clipping" )) &&
         cameraElement->ToElement() ) {
      arGUIXMLVector2Validator clipValidator( "camera 'clipping'", "near", "far" );
      clipValidator( cameraElement );
      float planes[ 2 ] = { 0.0f };
      cameraElement->ToElement()->Attribute( "near", &planes[ 0 ] );
      cameraElement->ToElement()->Attribute( "far",  &planes[ 1 ] );
      camF->setNearFar( planes[ 0 ], planes[ 1 ] );
    }

    // <position x="float" y="float" z="float" />
    if( (cameraElement = cameraNode->FirstChild( "position" )) ) {
      camF->setPosition( _attributearVector3( cameraElement, "camera 'position'" ) );
    }      

    // <target x="float" y="float" z="float" />
    if( (cameraElement = cameraNode->FirstChild( "target" )) ) {
      camF->setTarget( _attributearVector3( cameraElement, "camera 'target'" ) );
    }

    // <up x="float" y="float" z="float" />
    if( (cameraElement = cameraNode->FirstChild( "up" )) ) {
      camF->setUp( _attributearVector3( cameraElement, "camera 'up'" ) );
    }
    
    camera = camF;
  } else {
    ar_log_warning() << "arGUIXMLParser defaulting to arVRCamera for unknown camera type \""
                     << cameraType << "\"\n";
    camera = new arVRCamera();
  }

  validation &= cameraValidator( cameraNode );
  if (!validation) {
    ar_log_warning() << "arGUIXMLParser: invalid attribute or field in camera node.\n";
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
//    ar_log_warning() << "arGUIXML: failed to parse at line " << _doc.ErrorRow() << ar_endl;
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
    ar_log_warning() << "arGUIXML: malformed <szg_display> node.\n";
    return -1;
  }

  arGUIXMLDisplayValidator displayValidator;
  displayValidator( szgDisplayNode );

  // <threaded value="true|false|yes|no" />
  if( szgDisplayNode->ToElement()->Attribute( "threaded" ) ) {
    _windowingConstruct->setThreaded( _attributeBool( szgDisplayNode->ToElement(), "display 'threaded'" ) ? 1 : 0 );
  }

  // <framelock value="wildcat" />
  if( szgDisplayNode->ToElement()->Attribute( "framelock" ) ) {
    string framelock = szgDisplayNode->ToElement()->Attribute( "framelock" );
    arGUIXMLAttributeValueValidator fLockValidator( "display 'framelock'", "wildcat/none" );
    fLockValidator( framelock );
    _windowingConstruct->setUseFramelock( framelock == "wildcat" ? 1 : 0 );
  }

  // iterate over all <szg_window> elements
  for( TiXmlNode* windowNode = szgDisplayNode->FirstChild( "szg_window" ); windowNode;
       windowNode = windowNode->NextSibling() ) {

    arGUIXMLWindowValidator windowValidator;
    TiXmlNode* windowElement = NULL;

    // save the current position in the list of <szg_window> siblings (to be
    // restored if this windowNode is a 'pointer')
    TiXmlNode* savedWindowNode = windowNode;

    // is this a pointer to another window?
    TiXmlNode* namedWindowNode = _getNamedNode( windowNode->ToElement()->Attribute( "usenamed" ),
                                                "szg_window" );
    if( namedWindowNode ) {
      string namedWindow( windowNode->ToElement()->Attribute( "usenamed" ) );
      ar_log_debug() << "arGUIXML using named window " << namedWindow << ".\n";
      windowNode = namedWindowNode;
    }

    if( !windowNode->ToElement() ) {
      ar_log_warning() << "arGUIXML: skipping invalid window element.\n";
      continue;
    }

    arGUIWindowConfig* windowConfig = new arGUIWindowConfig();

    // <size width="integer" height="integer" />
    if( (windowElement = windowNode->FirstChild( "size" )) &&
         windowElement->ToElement() ) {
      arGUIXMLVector2Validator winSizeValidator( "window 'size'", "width", "height" );
      winSizeValidator( windowElement );
      int width, height;
      windowElement->ToElement()->Attribute( "width",  &width );
      windowElement->ToElement()->Attribute( "height", &height );
      windowConfig->setSize( width, height );
    }

    // <position x="integer" y="integer" />
    if( (windowElement = windowNode->FirstChild( "position" )) &&
         windowElement->ToElement() ) {
      arGUIXMLVector2Validator winPosValidator( "window 'position'" );
      winPosValidator( windowElement );
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
      ar_log_debug() << "windowConfig->setDecorate( " << _attributeBool( windowElement )
                     << " ).\n";
      windowConfig->setDecorate( _attributeBool( windowElement ) );
    }

    // <stereo value="true|false|yes|no" />
    if( (windowElement = windowNode->FirstChild( "stereo" )) ) {
      windowConfig->setStereo( _attributeBool( windowElement ) );
    }

    // <zorder value="normal|top|topmost" />
    if( (windowElement = windowNode->FirstChild( "zorder" )) &&
         windowElement->ToElement()) {
      arGUIXMLValueValidator zOrdValidator( "zorder" );
      zOrdValidator( windowElement );
      if (windowElement->ToElement()->Attribute( "value" ) ) {
        const string zorder(windowElement->ToElement()->Attribute("value"));
        arGUIXMLAttributeValueValidator zOrdValValidator( "zorder", "normal/top/topmost" );
        zOrdValValidator( zorder );
        const arZOrder arzorder =
          (zorder == "normal") ? AR_ZORDER_NORMAL :
          (zorder == "topmost") ? AR_ZORDER_TOPMOST :
          /* default, if( zorder == "top" ) */ AR_ZORDER_TOP;

        windowConfig->setZOrder( arzorder );
      }
    }

    // <bpp value="integer" />
    if( (windowElement = windowNode->FirstChild( "bpp" )) &&
        windowElement->ToElement() ) {
      arGUIXMLValueValidator bppValidator( "bpp" );
      bppValidator( windowElement );
      int bpp;
      windowElement->ToElement()->Attribute( "value", &bpp );
      windowConfig->setBpp( bpp );
    }

    // <title value="string" />
    if( (windowElement = windowNode->FirstChild( "title" )) &&
         windowElement->ToElement()) {
      arGUIXMLValueValidator titleValidator( "title" );
      titleValidator( windowElement );
      if (windowElement->ToElement()->Attribute( "value" ) ) {
        windowConfig->setTitle( windowElement->ToElement()->Attribute( "value" ) );
      }
    }

    // <xdisplay value="string" />
    if( (windowElement = windowNode->FirstChild( "xdisplay" )) &&
        windowElement->ToElement()) {
      arGUIXMLValueValidator xDispValidator( "xdisplay" );
      xDispValidator( windowElement );
      if (windowElement->ToElement()->Attribute( "value" ) ) {
        windowConfig->setXDisplay( windowElement->ToElement()->Attribute( "value" ) );
      }
    }

    // <cursor value="arrow|none|help|wait" />
    if( (windowElement = windowNode->FirstChild( "cursor")) &&
        windowElement->ToElement()) {
      arGUIXMLValueValidator cursValidator( "cursor" );
      cursValidator( windowElement );
      string initialCursor("arrow");
      if (windowElement->ToElement()->Attribute( "value" ) ) {
        initialCursor = windowElement->ToElement()->Attribute( "value" );
      }
      arGUIXMLAttributeValueValidator cursValValidator( "cursor", "none/help/wait/arrow" );
      cursValValidator( initialCursor );

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
      else if( initialCursor == "arrow" ) {
        cursor = AR_CURSOR_ARROW;
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

    string viewMode( "normal" );

    // run through all the viewports in the viewport list
    TiXmlNode* viewportListNode = windowNode->FirstChild( "szg_viewport_list" );
    TiXmlNode* namedViewportListNode = NULL;

    arGUIXMLViewportListValidator vpListValidator;

    // possible not to have a <szg_viewport_list>, so the viewportListNode
    // pointer needs to be checked from here on out
    if( viewportListNode ) {
      // check if this is a pointer to another viewportlist
      namedViewportListNode = _getNamedNode( viewportListNode->ToElement()->Attribute( "usenamed" ),
                                             "szg_viewport_list" );

      if( namedViewportListNode ) {
        string namedViewportList( namedViewportListNode->ToElement()->Attribute( "usenamed" ) );
        ar_log_debug() << "arGUIXML using named viewportList " << namedViewportList << ".\n";
        viewportListNode = namedViewportListNode;
      }

      if( !viewportListNode->ToElement() ) {
        ar_log_warning() << "arGUIXML: invalid viewportlist element.\n";
        return -1;
      }

      // determine which viewmode was specified, anything other than "custom"
      // doesn't need any viewports to actually be listed
      // <viewmode value="normal|anaglyph|walleyed|crosseyed|overunder|custom" />
      if( viewportListNode->ToElement()->Attribute( "viewmode" ) ) {
        viewMode = viewportListNode->ToElement()->Attribute( "viewmode" );
        arGUIXMLAttributeValueValidator vModeValValidator( "viewmode", 
            "normal/walleyed/crosseyed/anaglyph/overunder/custom" );
        vModeValValidator( viewMode );
            
      }
    }

    if( viewMode == "custom" ) {
      vpListValidator.addChild( "szg_viewport" );

      TiXmlNode* viewportNode = NULL;

      // if the user specified a viewmode of 'custom' but then didn't specify
      // any <szg_viewport>'s - that is an error! (the only way we can get here
      // is if viewportListNode /does/ exist, no need to check it again)
      if( !(viewportNode = viewportListNode->FirstChild( "szg_viewport" ) ) ) {
        // malformed!, delete currentwindow, print warning, continue with next window tag
        ar_log_warning() << "arGUIXML: viewmode is custom, but no <szg_viewport> tags.\n";
        return -1;
      }

      // clear out the 'standard' viewport
      _parsedWindowConstructs.back()->getGraphicsWindow()->clearViewportList();

      // iterate over all <szg_viewport> elements
      for( viewportNode = viewportListNode->FirstChild( "szg_viewport" ); viewportNode;
           viewportNode = viewportNode->NextSibling( "szg_viewport" ) ) {
        arGUIXMLViewportValidator vpValidator;

        TiXmlNode* savedViewportNode = viewportNode;

        // check if this is a pointer to another viewport
        TiXmlNode* namedViewportNode = _getNamedNode( viewportNode->ToElement()->Attribute( "usenamed" ),
                                                      "szg_viewport" );
        if( namedViewportNode ) {
          ar_log_debug() << "arGUIXML using named viewport "
                          << viewportNode->ToElement()->Attribute( "usenamed" ) << ".\n";
          viewportNode = namedViewportNode;
        }

        if( !viewportNode->ToElement() ) {
          ar_log_warning() << "arGUIXML skipping invalid viewport element.\n";
          continue;
        }

        TiXmlNode* viewportElement = NULL;

        // configure the camera and possibly the camera's screen
        arCamera* camera = NULL;
        arGraphicsScreen screen;

        if( !(camera = _configureCamera( screen, viewportNode->FirstChild( "szg_camera" ) )) ) {
          // should never happen, configureCamera should always return at least /something/
          ar_log_warning() << "arGUIXML custom configureCamera failed.\n";
        }

        arViewport viewport;

        viewport.setCamera( camera );
        viewport.setScreen( screen );

        // <coords left="float" bottom="float" width="float" height="float" />
        if( (viewportElement = viewportNode->FirstChild( "coords" )) ) {
          arVector4 vec = _attributearVector4( viewportElement, "viewport 'coords'", "left", "bottom", "width", "height" );
          viewport.setViewport( vec );
        }

        // <depthclear value="true|false|yes|no" />
        if( (viewportElement = viewportNode->FirstChild( "depthclear" )) ) {
          viewport.clearDepthBuffer( _attributeBool( viewportElement ) );
        }

        // <colormask R="true|false|yes|no" G="true|false|yes|no" B="true|false|yes|no" A="true|false|yes|no" />
        if( (viewportElement = viewportNode->FirstChild( "colormask" )) ) {
          arGUIXMLVector4Validator cmValidator( "viewport 'colormask'", "R", "G", "B", "A" );
          cmValidator( viewportElement );
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
          arGUIXMLValueValidator esignValidator( "viewport 'eyesign'" );
          esignValidator( viewportElement );
          float eyesign;
          viewportElement->ToElement()->Attribute( "value", &eyesign );
          viewport.setEyeSign( eyesign );
        }

        // <ogldrawbuf value="GL_NONE|GL_FRONT_LEFT|GL_FRONT_RIGHT|GL_BACK_LEFT|GL_BACK_RIGHT|GL_FRONT|GL_BACK|GL_LEFT|GL_RIGHT|GL_FRONT_AND_BACK" />
        if( (viewportElement = viewportNode->FirstChild( "ogldrawbuf" )) &&
            viewportElement->ToElement() ) {
          arGUIXMLValueValidator oglBufValidator( "viewport 'ogldrawbuf'" );
          oglBufValidator( viewportElement );
          GLenum ogldrawbuf;
          const string buf(viewportElement->ToElement()->Attribute( "value" ));
          arGUIXMLAttributeValueValidator oglValValidator( "viewport 'ogldrawbuf'",
             string("GL_NONE/GL_FRONT_LEFT/GL_FRONT_RIGHT/GL_BACK_LEFT/GL_BACK_RIGHT") +
             string("/GL_FRONT/GL_BACK/GL_LEFT/GL_RIGHT/GL_FRONT_AND_BACK") );
          oglValValidator( buf );
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
          else {
            ogldrawbuf = GL_NONE;
          }

          viewport.setDrawBuffer( ogldrawbuf );
        }

        vpValidator( viewportNode );

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
    } else {
      // even though there's no viewport specified in this case, there could
      // still be a camera+screen tag that needs to be parsed
      vpListValidator.addChild( "szg_camera" );

      arGraphicsScreen screen;

      arCamera* camera = _configureCamera(
        screen, viewportListNode ? viewportListNode->FirstChild( "szg_camera" ) : NULL );
      if (!camera) {
        // should never happen, configureCamera should always return at least /something/
        ar_log_warning() << "arGUIXML configureCamera failed.\n";
      }

      // viewports added by setViewMode will use this camera and screen
      _parsedWindowConstructs.back()->getGraphicsWindow()->setCamera( camera );
      _parsedWindowConstructs.back()->getGraphicsWindow()->setScreen( screen );

      // set up the appropriate viewports
      if( !_parsedWindowConstructs.back()->getGraphicsWindow()->setViewMode( viewMode ) ) {
        ar_log_warning() << "arGUIXML setViewMode failed.\n";
      }

      if( camera ) {
        // setCamera copied and now owns the camera, so safe to delete
        delete camera;
      }
    }

    vpListValidator( viewportListNode );
    windowValidator( windowNode );

    // if windowNode was changed because it was a 'pointer' to another window,
    // revert it back to its position in the list of sibling windows for this
    // <szg_display>
    windowNode = savedWindowNode;

    if (namedWindowNode)
      delete namedWindowNode;
    if (namedViewportListNode)
      delete namedViewportListNode;
  }

  _windowingConstruct->setWindowConstructs( &_parsedWindowConstructs );
  return 0;
}
