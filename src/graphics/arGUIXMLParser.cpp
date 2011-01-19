//@+leo-ver=4-thin
//@+node:jimc.20100409112755.216:@thin graphics\arGUIXMLParser.cpp
//@@language c++
//@@tabwidth -4
//@+others
//@+node:jimc.20100409112755.217:arGUIXMLParser declarations
//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for detils
//********************************************************

#include "arPrecompiled.h"

#include "arCamera.h"
#include "arGUIWindow.h"
#include "arGUIWindowManager.h"
#include "arGUIXMLParser.h"
#include "arGraphicsScreen.h"
#include "arGraphicsWindow.h"
#include "arLogStream.h"
#include "arOrthoCamera.h"
#include "arPerspectiveCamera.h"
#include "arTexture.h"
#include "arVRCamera.h"
#include "arViewport.h"
#include "arSTLalgo.h"


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

//@-node:jimc.20100409112755.217:arGUIXMLParser declarations
//@+node:jimc.20100409112755.218:arGUIXMLValidator::addAttributes
void arGUIXMLValidator::addAttributes( const arSlashString& attribsStr ) {
  for (int i=0; i<attribsStr.size(); ++i) {
    _attribsVec.push_back( attribsStr[i] );
  }
}
//@-node:jimc.20100409112755.218:arGUIXMLValidator::addAttributes
//@+node:jimc.20100409112755.219:arGUIXMLValidator::addChildren

void arGUIXMLValidator::addChildren( const arSlashString& childrenStr ) {
  for (int i=0; i<childrenStr.size(); ++i) {
    _childrenVec.push_back( childrenStr[i] );
  }
}
//@-node:jimc.20100409112755.219:arGUIXMLValidator::addChildren
//@+node:jimc.20100409112755.220:arGUIXMLValidator::operator

bool arGUIXMLValidator::operator()( TiXmlNode* node ) {
  if (!node) {
    ar_log_debug() << "arGUIXML skipping NULL " << _nodeTypeName << " node.\n";
    return false;
  }
  return _validateNodeAttributes( node ) && 
    _validateNodeChildren( node );
}
//@-node:jimc.20100409112755.220:arGUIXMLValidator::operator
//@+node:jimc.20100409112755.221:arGUIXMLValidator::_validateNodeAttributes

bool arGUIXMLValidator::_validateNodeAttributes( TiXmlNode* node ) {
  bool ok = true;
  for (TiXmlAttribute* att = node->ToElement()->FirstAttribute(); att; att = att->Next() ) {
    string name( att->Name() );
    if (find( _attribsVec.begin(), _attribsVec.end(), name ) == _attribsVec.end()) {
      ar_log_error() << "arGUIXMLParser: unknown attribute '"
                     << name << "' in " << _nodeTypeName << " node.\n"
                     << "\tLegal attributes are:";
      vector< string >::const_iterator iter;
      for (iter = _attribsVec.begin(); iter != _attribsVec.end(); ++iter) {
        ar_log_error() << " " << *iter;
      }
      ar_log_error() << ar_endl;
      ok = false;
    }
  }
  return ok;
}
//@-node:jimc.20100409112755.221:arGUIXMLValidator::_validateNodeAttributes
//@+node:jimc.20100409112755.222:arGUIXMLValidator::_validateNodeChildren

bool arGUIXMLValidator::_validateNodeChildren( TiXmlNode* node ) {
  bool ok = true;
  for (TiXmlNode* child = node->FirstChild(); child; child = child->NextSibling() ) {
    string name( child->Value() );
    if (find( _childrenVec.begin(), _childrenVec.end(), name ) == _childrenVec.end()) {
      ar_log_error() << "arGUIXMLParser: unknown sub-node '"
                     << name << "' in " << _nodeTypeName << " node.\n"
                     << "\tLegal sub-nodes are:";
      vector< string >::const_iterator iter;
      for (iter = _childrenVec.begin(); iter != _childrenVec.end(); ++iter) {
        ar_log_error() << " " << *iter;
      }
      ar_log_error() << ar_endl;
      ok = false;
    }
  }
  return ok;
}
//@-node:jimc.20100409112755.222:arGUIXMLValidator::_validateNodeChildren
//@+node:jimc.20100409112755.223:arGUIXMLDisplayValidator::arGUIXMLDisplayValidator

class arGUIXMLDisplayValidator: public arGUIXMLValidator {
  public:
    arGUIXMLDisplayValidator();
};
arGUIXMLDisplayValidator::arGUIXMLDisplayValidator() :
  arGUIXMLValidator("display") {
  addAttributes( "threaded/framelock/extra_stereo_bufferswap" );
  addChildren( "szg_window" );
}
//@-node:jimc.20100409112755.223:arGUIXMLDisplayValidator::arGUIXMLDisplayValidator
//@+node:jimc.20100409112755.224:arGUIXMLWindowValidator::arGUIXMLWindowValidator


class arGUIXMLWindowValidator: public arGUIXMLValidator {
  public:
    arGUIXMLWindowValidator();
};
arGUIXMLWindowValidator::arGUIXMLWindowValidator() :
  arGUIXMLValidator("window") {
  addAttributes( "usenamed" );
  addChildren( "size/position/fullscreen/decorate/stereo/zorder/bpp/title/xdisplay/cursor/szg_viewport_list" );
}
//@-node:jimc.20100409112755.224:arGUIXMLWindowValidator::arGUIXMLWindowValidator
//@+node:jimc.20100409112755.225:arGUIXMLViewportListValidator::arGUIXMLViewportListValidator


class arGUIXMLViewportListValidator: public arGUIXMLValidator {
  public:
    arGUIXMLViewportListValidator();
};
arGUIXMLViewportListValidator::arGUIXMLViewportListValidator() :
  arGUIXMLValidator("viewportlist") {
  addAttributes( "usenamed/viewmode" );
}
//@-node:jimc.20100409112755.225:arGUIXMLViewportListValidator::arGUIXMLViewportListValidator
//@+node:jimc.20100409112755.226:arGUIXMLViewportValidator::arGUIXMLViewportValidator


class arGUIXMLViewportValidator: public arGUIXMLValidator {
  public:
    arGUIXMLViewportValidator();
};
arGUIXMLViewportValidator::arGUIXMLViewportValidator() :
  arGUIXMLValidator("viewport") {
  addAttributes( "usenamed" );
  addChildren( "szg_camera/coords/depthclear/colormask/eyesign/ogldrawbuf" );
}
//@-node:jimc.20100409112755.226:arGUIXMLViewportValidator::arGUIXMLViewportValidator
//@+node:jimc.20100409112755.227:arGUIXMLScreenValidator::arGUIXMLScreenValidator


class arGUIXMLScreenValidator: public arGUIXMLValidator {
  public:
    arGUIXMLScreenValidator();
};
arGUIXMLScreenValidator::arGUIXMLScreenValidator() :
  arGUIXMLValidator("screen") {
  addAttributes( "usenamed" );
  addChildren( "center/normal/up/dim/headmounted/tile/usefixedhead/fixedheadpos/fixedheadupangle" );
}
//@-node:jimc.20100409112755.227:arGUIXMLScreenValidator::arGUIXMLScreenValidator
//@+node:jimc.20100409112755.228:arGUIXMLCameraValidator::arGUIXMLCameraValidator


class arGUIXMLCameraValidator: public arGUIXMLValidator {
  public:
    arGUIXMLCameraValidator();
};
arGUIXMLCameraValidator::arGUIXMLCameraValidator() :
  arGUIXMLValidator("camera") {
  addAttributes( "usenamed/type" );
  addChildren( "szg_screen" );
}
//@-node:jimc.20100409112755.228:arGUIXMLCameraValidator::arGUIXMLCameraValidator
//@+node:jimc.20100409112755.229:arGUIXMLFrustumCameraValidator::arGUIXMLFrustumCameraValidator


class arGUIXMLFrustumCameraValidator: public arGUIXMLValidator {
  public:
    arGUIXMLFrustumCameraValidator();
};
arGUIXMLFrustumCameraValidator::arGUIXMLFrustumCameraValidator() :
  arGUIXMLValidator("frustum") {
  addAttributes( "left/right/bottom/top/near/far" );
}
//@-node:jimc.20100409112755.229:arGUIXMLFrustumCameraValidator::arGUIXMLFrustumCameraValidator
//@+node:jimc.20100409112755.230:arGUIXMLLookatCameraValidator::arGUIXMLLookatCameraValidator


class arGUIXMLLookatCameraValidator: public arGUIXMLValidator {
  public:
    arGUIXMLLookatCameraValidator();
};
arGUIXMLLookatCameraValidator::arGUIXMLLookatCameraValidator() :
  arGUIXMLValidator("lookat") {
  addAttributes( "viewx/viewy/viewz/lookatx/lookaty/lookatz/upx/upy/upz" );
}
//@-node:jimc.20100409112755.230:arGUIXMLLookatCameraValidator::arGUIXMLLookatCameraValidator
//@+node:jimc.20100409112755.231:arGUIXMLVector2Validator::arGUIXMLVector2Validator

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
//@-node:jimc.20100409112755.231:arGUIXMLVector2Validator::arGUIXMLVector2Validator
//@+node:jimc.20100409112755.232:arGUIXMLVector3Validator::arGUIXMLVector3Validator


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
//@-node:jimc.20100409112755.232:arGUIXMLVector3Validator::arGUIXMLVector3Validator
//@+node:jimc.20100409112755.233:arGUIXMLVector4Validator::arGUIXMLVector4Validator


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
//@-node:jimc.20100409112755.233:arGUIXMLVector4Validator::arGUIXMLVector4Validator
//@+node:jimc.20100409112755.234:arGUIXMLValueValidator::arGUIXMLValueValidator


class arGUIXMLValueValidator: public arGUIXMLValidator {
  public:
    arGUIXMLValueValidator( const string& name );
};
arGUIXMLValueValidator::arGUIXMLValueValidator( const string& name ) :
  arGUIXMLValidator(name) {
  addAttributes( "value" );
}
//@-node:jimc.20100409112755.234:arGUIXMLValueValidator::arGUIXMLValueValidator
//@+node:jimc.20100409112755.235:arGUIXMLAttributeValueValidator::arGUIXMLAttributeValueValidator


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
//@-node:jimc.20100409112755.235:arGUIXMLAttributeValueValidator::arGUIXMLAttributeValueValidator
//@+node:jimc.20100409112755.236:arGUIXMLAttributeValueValidator::addValues
void arGUIXMLAttributeValueValidator::addValues( const arSlashString& valuesStr ) {
  for (int i=0; i<valuesStr.size(); ++i) {
    _valuesVec.push_back( valuesStr[i] );
  }
}
//@-node:jimc.20100409112755.236:arGUIXMLAttributeValueValidator::addValues
//@+node:jimc.20100409112755.237:arGUIXMLAttributeValueValidator::operator
bool arGUIXMLAttributeValueValidator::operator()( const string& valueStr ) {
  if (find( _valuesVec.begin(), _valuesVec.end(), valueStr ) != _valuesVec.end())
    return true;

  ar_log_error() << "arGUIXMLParser: invalid value '"
                 << valueStr << "' in " << _nodeName << " attribute.\n"
                 << "\tLegal values are:";
  vector< string >::const_iterator iter;
  for (iter = _valuesVec.begin(); iter != _valuesVec.end(); ++iter) {
    ar_log_error() << " " << *iter;
  }
  ar_log_error() << ar_endl;
  return false;
}
//@-node:jimc.20100409112755.237:arGUIXMLAttributeValueValidator::operator
//@+node:jimc.20100409112755.238:arGUIXMLWindowConstruct::arGUIXMLWindowConstruct

arGUIXMLWindowConstruct::arGUIXMLWindowConstruct( arGUIWindowConfig* windowConfig,
                                                  arGraphicsWindow* graphicsWindow,
                                                  arGUIRenderCallback* guiDrawCallback ) :
  _windowConfig( windowConfig ),
  _graphicsWindow( graphicsWindow ),
  _guiDrawCallback( guiDrawCallback )
{
}
//@-node:jimc.20100409112755.238:arGUIXMLWindowConstruct::arGUIXMLWindowConstruct
//@+node:jimc.20100409112755.239:arGUIXMLWindowConstruct

arGUIXMLWindowConstruct::~arGUIXMLWindowConstruct( void )
{
  // Depending on how windows have been reloaded and who copied
  // around _graphisWindow, this may be unsafe.
  delete _graphicsWindow;
  delete _windowConfig;
}
//@-node:jimc.20100409112755.239:arGUIXMLWindowConstruct
//@+node:jimc.20100409112755.240:arGUIWindowingConstruct::arGUIWindowingConstruct

arGUIWindowingConstruct::arGUIWindowingConstruct( int threaded, int useFramelock,
                                                  int useExtraBufSwap,
                                                  vector< arGUIXMLWindowConstruct* >* windowConstructs ) :
  _threaded( threaded ),
  _useFramelock( useFramelock ),
  _useExtraStereoBufferSwap( useExtraBufSwap ),
  _windowConstructs( windowConstructs )
{
}
//@-node:jimc.20100409112755.240:arGUIWindowingConstruct::arGUIWindowingConstruct
//@+node:jimc.20100409112755.241:arGUIWindowingConstruct

arGUIWindowingConstruct::~arGUIWindowingConstruct( void )
{
}
//@-node:jimc.20100409112755.241:arGUIWindowingConstruct
//@+node:jimc.20100409112755.242:arGUIXMLParser::arGUIXMLParser

arGUIXMLParser::arGUIXMLParser( arSZGClient* SZGClient,
                                const string& displayName ) :
  _SZGClient( SZGClient ),
  _mininumConfig( "<szg_display><szg_window /></szg_display>" )
{
  setDisplayName( displayName );
  _windowingConstruct = new arGUIWindowingConstruct();
}
//@-node:jimc.20100409112755.242:arGUIXMLParser::arGUIXMLParser
//@+node:jimc.20100409112755.243:arGUIXMLParser

arGUIXMLParser::~arGUIXMLParser( void )
{
  _doc.Clear();
}
//@-node:jimc.20100409112755.243:arGUIXMLParser
//@+node:jimc.20100409112755.244:arGUIXMLParser::setDisplayName

void arGUIXMLParser::setDisplayName( const string& displayName )
{
  ar_setTextureAllowNotPowOf2( 
    _SZGClient->getAttribute("SZG_RENDER", "allow_texture_not_pow2") != string("false") );

  if ( displayName == _displayName )
    return;

  if ( displayName.empty() || displayName == "NULL" ) {
    ar_log_remark() << "arGUIXML using default displayName.\n";
    _displayName = _mininumConfig;
  }
  else
    _displayName = displayName;

  // Delete any old displayName strings.
  _doc.Clear();

  // Append the new displayName string.
  _doc.Parse( _displayName.c_str() );
  if (_doc.Error())
    _reportParseError( &_doc, _displayName );
}
//@-node:jimc.20100409112755.244:arGUIXMLParser::setDisplayName
//@+node:jimc.20100409112755.245:arGUIXMLParser::_getNamedNode

/*
int arGUIXMLParser::numberOfWindows( void )
{
  int count = 0;

  if ( _doc.Error() ) {
    return count;
  }

  // get a reference to <szg_display>
  TiXmlNode* szgDisplayNode = _doc.FirstChild();

  if ( !szgDisplayNode ) {
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

  if ( !nodeDesc.length() || nodeDesc == "NULL" ) {
    ar_log_error() << "arGUIXMLParser: no 'usenamed' node: " << name << ar_endl;
    return NULL;
  }

  // create a usable node out of the xml string
  nodeDoc->Parse( nodeDesc.c_str() );
  if (nodeDoc->Error()) {
    _reportParseError( nodeDoc, nodeDesc );
  }
  if ( !nodeDoc->FirstChild() ) {
    ar_log_error() << "arGUIXMLParser: invalid node pointer: " << name << ar_endl;
    return NULL;
  }
  string nodeTypeName( nodeDoc->FirstChild()->Value() );
  if (nodeTypeName != nodeType) {
    ar_log_error() << "arGUIXMLParser: " << nodeType << " 'usenamed=" << name << "' "
                   << "\n\trefers to a record of type " << nodeTypeName
                   << ", not " << nodeType << "." << ar_endl;
    return NULL;
  }

  return nodeDoc->FirstChild();
}
//@-node:jimc.20100409112755.245:arGUIXMLParser::_getNamedNode
//@+node:jimc.20100409112755.246:arGUIXMLParser::_reportParseError

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
  ar_log_error() << "arGUIXMLParser: " << nodeDoc->ErrorDesc() << "\n";
  if (ok) {
    ar_log_error() << "in line:  " << errLine << "\n";
  }
  ar_log_error() << "(Use '-szg log=DEBUG' to see the whole XML chunk).\n";
  ar_log_debug() << "Somewhere in the following XML:\n\t" << nodeDesc << ar_endl;
  // The parser poorly localizes errors.
}
//@-node:jimc.20100409112755.246:arGUIXMLParser::_reportParseError
//@+node:jimc.20100409112755.247:arGUIXMLParser::_attributearVector3

arVector3 arGUIXMLParser::_attributearVector3( TiXmlNode* node,
                                               const string& name,
                                               const string& x,
                                               const string& y,
                                               const string& z )
{
  arVector3 vec;
  if ( !node || !node->ToElement() )
    return vec;

  arGUIXMLVector3Validator validator( name, x, y, z );
  validator( node );
  node->ToElement()->Attribute( x.c_str(), &vec[ 0 ] );
  node->ToElement()->Attribute( y.c_str(), &vec[ 1 ] );
  node->ToElement()->Attribute( z.c_str(), &vec[ 2 ] );
  return vec;
}
//@-node:jimc.20100409112755.247:arGUIXMLParser::_attributearVector3
//@+node:jimc.20100409112755.248:arGUIXMLParser::_attributearVector4

arVector4 arGUIXMLParser::_attributearVector4( TiXmlNode* node,
                                               const string& name,
                                               const string& x,
                                               const string& y,
                                               const string& z,
                                               const string& w )
{
  arVector4 vec;
  if ( !node || !node->ToElement() )
    return vec;

  arGUIXMLVector4Validator validator( name, x, y, z, w );
  validator( node );
  node->ToElement()->Attribute( x.c_str(), &vec[ 0 ] );
  node->ToElement()->Attribute( y.c_str(), &vec[ 1 ] );
  node->ToElement()->Attribute( z.c_str(), &vec[ 2 ] );
  node->ToElement()->Attribute( w.c_str(), &vec[ 3 ] );
  return vec;
}
//@-node:jimc.20100409112755.248:arGUIXMLParser::_attributearVector4
//@+node:jimc.20100409112755.249:arGUIXMLParser::_attributeBool

bool arGUIXMLParser::_attributeBool( TiXmlNode* node,
                                     const string& value )
{
  if ( !node || !node->ToElement() ) {
    ar_log_debug() << "_attributeBool() invalide node.\n";
    return false;
  }

  const char* pch = node->ToElement()->Attribute( value.c_str() );
  if (!pch) {
    ar_log_debug() << "_attributeBool() failed to get attribute value string for '"
                   << value << "'.\n";
    return false;
  }

  const string attrVal( pch );
  if ((attrVal != "yes")&&(attrVal != "no")&&(attrVal != "true")&&(attrVal != "false")) {
    ar_log_error() << "arGUIXML expected attribute '" << value <<
      "' to be one of yes/no/true/false, not '" << attrVal << "'.  Defaulting to 'no'.\n";
    return false;
  }
  ar_log_debug() << "_attributeBool() value for attribute '" << value
                 << "' = '" << attrVal << "'\n";

  return attrVal == "true" || attrVal == "yes";
}
//@-node:jimc.20100409112755.249:arGUIXMLParser::_attributeBool
//@+node:jimc.20100409112755.250:arGUIXMLParser::_configureScreen

int arGUIXMLParser::_configureScreen( arGraphicsScreen& screen,
                                      TiXmlNode* screenNode )
{
  if ( !screenNode || !screenNode->ToElement() ) {
    // not necessarily an error, <szg_screen> could legitimately not exist and
    // in that case let the caller use the screen as it was passed in
    ar_log_remark() << "arGUIXML ignoring missing screen description.\n";
    return 0;
  }

  arGUIXMLScreenValidator screenValidator;

  // check if this is a pointer to another screen
  TiXmlNode* namedNode = _getNamedNode( screenNode->ToElement()->Attribute( "usenamed" ),
                                        "szg_screen" );
  if ( namedNode ) {
    ar_log_debug() << "arGUIXML using named screen "
                    << screenNode->ToElement()->Attribute( "usenamed" ) << ar_endl;
    screenNode = namedNode;
  }

  TiXmlNode* screenElement = NULL;

  // <center x="float" y="float" z="float" />
  if ( (screenElement = screenNode->FirstChild( "center" )) ) {
    arVector3 vec = _attributearVector3( screenElement, "screen 'center'" );
    screen.setCenter( vec );
  }

  // <normal x="float" y="float" z="float" />
  if ( (screenElement = screenNode->FirstChild( "normal" )) ) {
    arVector3 vec = _attributearVector3( screenElement, "screen 'normal'" );
    screen.setNormal( vec );
  }

  // <up x="float" y="float" z="float" />
  if ( (screenElement = screenNode->FirstChild( "up" )) ) {
    arVector3 vec = _attributearVector3( screenElement, "screen 'up'" );
    screen.setUp( vec );
  }

  // <dim width="float" height="float" />
  if ( (screenElement = screenNode->FirstChild( "dim" )) &&
      screenElement->ToElement() ) {
    float dim[ 2 ] = { 0.0f };
    arGUIXMLVector2Validator dimVal( "screen 'dim'", "width", "height" );
    dimVal( screenElement );
    screenElement->ToElement()->Attribute( "width",  &dim[ 0 ] );
    screenElement->ToElement()->Attribute( "height", &dim[ 1 ] );

    screen.setDimensions( dim[ 0 ], dim[ 1 ] );
  }

  // <headmounted value="true|false|yes|no" />
  if ( (screenElement = screenNode->FirstChild( "headmounted" )) ) {
    arGUIXMLValueValidator hmdValidator( "screen 'headmounted'" );
    hmdValidator( screenElement );
    screen.setHeadMounted( _attributeBool( screenElement ) );
  }

  // <tile tilex="integer" numtilesx="integer" tiley="integer" numtilesy="integer" />
  if ( (screenElement = screenNode->FirstChild( "tile" )) ) {
    arVector4 vec = _attributearVector4( screenElement, "screen 'tile'",
                                         "tilex", "numtilesx",
                                         "tiley", "numtilesy" );
    screen.setTile( vec );
  }

  // <usefixedhead value="allow|always|ignore" />
  if ( (screenElement = screenNode->FirstChild( "usefixedhead" )) &&
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
  if ( (screenElement = screenNode->FirstChild( "fixedheadpos" )) ) {
    arVector3 vec = _attributearVector3( screenElement, "screen 'fixedheadpos'" );
    screen.setFixedHeadPosition( vec );
  }

  // <fixedheadupangle value="float" />
  if ( (screenElement = screenNode->FirstChild( "fixedheadupangle" )) &&
      screenElement->ToElement() ) {
    arGUIXMLValueValidator fixAngValidator( "screen 'fixedheadupangle'" );
    fixAngValidator( screenElement );
    float angle;
    screenElement->ToElement()->Attribute( "value", &angle );

    screen.setFixedHeadHeadUpAngle( angle );
  }

  if (!screenValidator( screenNode )) {
    ar_log_error() << "arGUIXMLParser: invalid attribute or field in screen node.\n";
    return 0;
  }

  if ( namedNode ) {
    delete namedNode;
  }

  return 0;
}
//@-node:jimc.20100409112755.250:arGUIXMLParser::_configureScreen
//@+node:jimc.20100409112755.251:arGUIXMLParser::_configureCamera

arCamera* arGUIXMLParser::_configureCamera( arGraphicsScreen& screen,
                                            TiXmlNode* cameraNode )
{
  // caller owns return value and should delete it

  if ( !cameraNode || !cameraNode->ToElement() ) {
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
  if ( namedNode ) {
    string useNamed( cameraNode->ToElement()->Attribute( "usenamed" ) );
    ar_log_debug() << "arGUIXML using named camera " << useNamed << ".\n";
    cameraNode = namedNode;
  }

  TiXmlNode* cameraElement = NULL;
  string cameraType( "vr" );

  if ( cameraNode->ToElement()->Attribute( "type" ) ) {
    cameraType = cameraNode->ToElement()->Attribute( "type" );
  }

  arGUIXMLAttributeValueValidator camTypeValidator( "camera 'type'", "vr/ortho/perspective" );
  camTypeValidator( cameraType );

  if ( _configureScreen( screen, cameraNode->FirstChild( "szg_screen" ) ) < 0 ) {
    // print warning, return default camera + screen
  }

  arCamera* camera = NULL;
  if ( cameraType == "vr" ) {
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
    if ( (cameraElement = cameraNode->FirstChild( "frustum" )) &&
        cameraElement->ToElement() ) {
      arGUIXMLFrustumCameraValidator frustumValidator;
      validation = frustumValidator( cameraElement );
      if (!validation) {
        ar_log_error() << "arGUIXMLParser: invalid attribute or field in frustum node.\n";
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
    if ( (cameraElement = cameraNode->FirstChild( "lookat" )) &&
        cameraElement->ToElement() ) {
      arGUIXMLLookatCameraValidator lookatValidator;
      validation = lookatValidator( cameraElement );
      if (!validation) {
        ar_log_error() << "arGUIXMLParser: invalid attribute or field in lookat node.\n";
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
    if ( (cameraElement = cameraNode->FirstChild( "sides" ))) {
      camF->setSides( _attributearVector4( cameraElement, "sides", "left", "right", "bottom", "top" ) );
    }

    // <clipping near="float" far="float" />
    if ( (cameraElement = cameraNode->FirstChild( "clipping" )) &&
         cameraElement->ToElement() ) {
      arGUIXMLVector2Validator clipValidator( "camera 'clipping'", "near", "far" );
      clipValidator( cameraElement );
      float planes[ 2 ] = { 0.0f };
      cameraElement->ToElement()->Attribute( "near", &planes[ 0 ] );
      cameraElement->ToElement()->Attribute( "far",  &planes[ 1 ] );
      camF->setNearFar( planes[ 0 ], planes[ 1 ] );
    }

    // <position x="float" y="float" z="float" />
    if ( (cameraElement = cameraNode->FirstChild( "position" )) ) {
      camF->setPosition( _attributearVector3( cameraElement, "camera 'position'" ) );
    }      

    // <target x="float" y="float" z="float" />
    if ( (cameraElement = cameraNode->FirstChild( "target" )) ) {
      camF->setTarget( _attributearVector3( cameraElement, "camera 'target'" ) );
    }

    // <up x="float" y="float" z="float" />
    if ( (cameraElement = cameraNode->FirstChild( "up" )) ) {
      camF->setUp( _attributearVector3( cameraElement, "camera 'up'" ) );
    }

    camera = camF;
  } else {
    ar_log_error() << "arGUIXMLParser defaulting to arVRCamera for unknown camera type \""
                     << cameraType << "\"\n";
    camera = new arVRCamera();
  }

  validation &= cameraValidator( cameraNode );
  if (!validation) {
    ar_log_error() << "arGUIXMLParser: invalid attribute or field in camera node.\n";
  }

  if (namedNode)
    delete namedNode;

  camera->setScreen( &screen );
  return camera;
}
//@-node:jimc.20100409112755.251:arGUIXMLParser::_configureCamera
//@+node:jimc.20100409112755.252:arGUIXMLParser::parse

bool arGUIXMLParser::parse( void )
{
  //  Should have already complained about any errors.
//  if ( _doc.Error() ) {
//    ar_log_error() << "arGUIXMLParser: failed to parse at line " << _doc.ErrorRow() << ar_endl;
//    return false;
//  }

  // clear out any previous parsing constructs
  // the graphicsWindow's and drawcallback's are externally owned, but this
  // *will* cause a leak of the windowconfig's (and obviously the
  // arGUIWindowConstruct pointers themselves)
  _parsedWindowConstructs.clear();

  // get a reference to <szg_display>
  TiXmlNode* szgDisplayNode = _doc.FirstChild();

  if ( !szgDisplayNode || !szgDisplayNode->ToElement() ) {
    ar_log_error() << "arGUIXMLParser: malformed <szg_display> node.\n";
    return false;
  }

  arGUIXMLDisplayValidator displayValidator;
  displayValidator( szgDisplayNode );

  // <threaded value="true|false|yes|no" />
  if ( szgDisplayNode->ToElement()->Attribute( "threaded" ) ) {
    _windowingConstruct->setThreaded( _attributeBool( szgDisplayNode->ToElement(), "threaded" ) ? 1 : 0 );
  }

  // <extra_stereo_bufferswap value="true|false|yes|no" />
  if ( szgDisplayNode->ToElement()->Attribute( "extra_stereo_bufferswap" ) ) {
    _windowingConstruct->setUseExtraStereoBufferSwap( _attributeBool( szgDisplayNode->ToElement(), "extra_stereo_bufferswap" ) ? 1 : 0 );
  }

  // <framelock value="wildcat" /> <framelock value="wgl" />
  if ( szgDisplayNode->ToElement()->Attribute( "framelock" ) ) {
    string framelock = szgDisplayNode->ToElement()->Attribute( "framelock" );
    arGUIXMLAttributeValueValidator fLockValidator( "display 'framelock'", "wildcat/wgl/none" );
    fLockValidator( framelock );
    if (framelock == "wildcat") {
      ar_log_warning() << "<szg_display framelock=""wildcat""> is deprecated,\n"
                       << "   please use <szg_display framelock=""wgl""> instead.\n";
    }
    _windowingConstruct->setUseFramelock( ((framelock == "wildcat")||(framelock == "wgl")) ? 1 : 0 );
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
    if ( namedWindowNode ) {
      string namedWindow( windowNode->ToElement()->Attribute( "usenamed" ) );
      ar_log_debug() << "arGUIXML using named window " << namedWindow << ".\n";
      windowNode = namedWindowNode;
    }

    if ( !windowNode->ToElement() ) {
      ar_log_error() << "arGUIXMLParser: skipping invalid window element.\n";
      continue;
    }

    arGUIWindowConfig* windowConfig = new arGUIWindowConfig();

    // <size width="integer" height="integer" />
    if ( (windowElement = windowNode->FirstChild( "size" )) &&
         windowElement->ToElement() ) {
      arGUIXMLVector2Validator winSizeValidator( "window 'size'", "width", "height" );
      winSizeValidator( windowElement );
      int width, height;
      windowElement->ToElement()->Attribute( "width",  &width );
      windowElement->ToElement()->Attribute( "height", &height );
      windowConfig->setSize( width, height );
    }

    // <position x="integer" y="integer" />
    if ( (windowElement = windowNode->FirstChild( "position" )) &&
         windowElement->ToElement() ) {
      arGUIXMLVector2Validator winPosValidator( "window 'position'" );
      winPosValidator( windowElement );
      int x, y;
      windowElement->ToElement()->Attribute( "x", &x );
      windowElement->ToElement()->Attribute( "y", &y );
      windowConfig->setPos( x, y );
    }

    // <fullscreen value="true|false|yes|no" />
    if ( (windowElement = windowNode->FirstChild( "fullscreen" )) ) {
      windowConfig->setFullscreen( _attributeBool( windowElement ) );
    }

    // <decorate value="true|false|yes|no" />
    if ( (windowElement = windowNode->FirstChild( "decorate" )) ) {
      ar_log_debug() << "windowConfig->setDecorate( " << _attributeBool( windowElement )
                     << " ).\n";
      windowConfig->setDecorate( _attributeBool( windowElement ) );
    }

    // <stereo value="true|false|yes|no" />
    if ( (windowElement = windowNode->FirstChild( "stereo" )) ) {
      windowConfig->setStereo( _attributeBool( windowElement ) );
    }

    // <zorder value="normal|top|topmost" />
    if ( (windowElement = windowNode->FirstChild( "zorder" )) &&
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
          /* default, if ( zorder == "top" ) */ AR_ZORDER_TOP;

        windowConfig->setZOrder( arzorder );
      }
    }

    // <bpp value="integer" />
    if ( (windowElement = windowNode->FirstChild( "bpp" )) &&
        windowElement->ToElement() ) {
      arGUIXMLValueValidator bppValidator( "bpp" );
      bppValidator( windowElement );
      int bpp;
      windowElement->ToElement()->Attribute( "value", &bpp );
      windowConfig->setBpp( bpp );
    }

    // <title value="string" />
    if ( (windowElement = windowNode->FirstChild( "title" )) &&
         windowElement->ToElement()) {
      arGUIXMLValueValidator titleValidator( "title" );
      titleValidator( windowElement );
      if (windowElement->ToElement()->Attribute( "value" ) ) {
        windowConfig->setTitle( windowElement->ToElement()->Attribute( "value" ) );
      }
    }

    // <xdisplay value="string" />
    if ( (windowElement = windowNode->FirstChild( "xdisplay" )) &&
        windowElement->ToElement()) {
      arGUIXMLValueValidator xDispValidator( "xdisplay" );
      xDispValidator( windowElement );
      if (windowElement->ToElement()->Attribute( "value" ) ) {
        windowConfig->setXDisplay( windowElement->ToElement()->Attribute( "value" ) );
      }
    }

    // <cursor value="arrow|none|help|wait" />
    if ( (windowElement = windowNode->FirstChild( "cursor")) &&
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
      if ( initialCursor == "none" ) {
        cursor = AR_CURSOR_NONE;
      }
      else if ( initialCursor == "help" ) {
        cursor = AR_CURSOR_HELP;
      }
      else if ( initialCursor == "wait" ) {
        cursor = AR_CURSOR_WAIT;
      }
      else if ( initialCursor == "arrow" ) {
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
    if ( viewportListNode ) {
      // check if this is a pointer to another viewportlist
      namedViewportListNode =
        _getNamedNode( viewportListNode->ToElement()->Attribute( "usenamed" ), "szg_viewport_list" );

      if ( namedViewportListNode ) {
        string namedViewportList( namedViewportListNode->ToElement()->Attribute( "usenamed" ) );
        ar_log_debug() << "arGUIXML using named viewportList " << namedViewportList << ".\n";
        viewportListNode = namedViewportListNode;
      }

      if ( !viewportListNode->ToElement() ) {
        ar_log_error() << "arGUIXMLParser: invalid viewportlist element.\n";
        return false;
      }

      // determine which viewmode was specified, anything other than "custom"
      // doesn't need any viewports to actually be listed
      // <viewmode value="normal|anaglyph|walleyed|crosseyed|overunder|custom" />
      if ( viewportListNode->ToElement()->Attribute( "viewmode" ) ) {
        viewMode = viewportListNode->ToElement()->Attribute( "viewmode" );
        arGUIXMLAttributeValueValidator vModeValValidator( "viewmode", 
            "normal/walleyed/crosseyed/anaglyph/overunder/custom" );
        vModeValValidator( viewMode );

      }
    }

    if ( viewMode == "custom" ) {
      vpListValidator.addChild( "szg_viewport" );

      TiXmlNode* viewportNode = NULL;

      // if the user specified a viewmode of 'custom' but then didn't specify
      // any <szg_viewport>'s - that is an error! (the only way we can get here
      // is if viewportListNode /does/ exist, no need to check it again)
      if ( !(viewportNode = viewportListNode->FirstChild( "szg_viewport" ) ) ) {
        // malformed!, delete currentwindow, print warning, continue with next window tag
        ar_log_error() << "arGUIXMLParser: viewmode is custom, but no <szg_viewport> tags.\n";
        return false;
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
        if ( namedViewportNode ) {
          ar_log_debug() << "arGUIXML using named viewport " <<
            viewportNode->ToElement()->Attribute( "usenamed" ) << ".\n";
          viewportNode = namedViewportNode;
        }

        if ( !viewportNode->ToElement() ) {
          ar_log_error() << "arGUIXML skipping invalid viewport element.\n";
          continue;
        }

        TiXmlNode* viewportElement = NULL;

        // configure the camera and possibly the camera's screen
        arCamera* camera = NULL;
        arGraphicsScreen screen;

        if ( !(camera = _configureCamera( screen, viewportNode->FirstChild( "szg_camera" ) )) ) {
          // should never happen, configureCamera should always return at least /something/
          ar_log_error() << "arGUIXML custom configureCamera failed.\n";
        }

        arViewport viewport;
        viewport.setCamera( camera );
        viewport.setScreen( screen );

        // <coords left="float" bottom="float" width="float" height="float" />
        if ( (viewportElement = viewportNode->FirstChild( "coords" )) ) {
          arVector4 vec = _attributearVector4( viewportElement, "viewport 'coords'", "left", "bottom", "width", "height" );
          viewport.setViewport( vec );
        }

        // <depthclear value="true|false|yes|no" />
        if ( (viewportElement = viewportNode->FirstChild( "depthclear" )) ) {
          viewport.clearDepthBuffer( _attributeBool( viewportElement ) );
        }

        // <colormask R="true|false|yes|no" G="true|false|yes|no" B="true|false|yes|no" A="true|false|yes|no" />
        if ( (viewportElement = viewportNode->FirstChild( "colormask" )) ) {
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
        if ( (viewportElement = viewportNode->FirstChild( "eyesign" )) &&
            viewportElement->ToElement() ) {
          arGUIXMLValueValidator esignValidator( "viewport 'eyesign'" );
          esignValidator( viewportElement );
          float eyesign;
          viewportElement->ToElement()->Attribute( "value", &eyesign );
          viewport.setEyeSign( eyesign );
        }

        // <ogldrawbuf value="GL_NONE|GL_FRONT_LEFT|GL_FRONT_RIGHT|GL_BACK_LEFT|GL_BACK_RIGHT|GL_FRONT|GL_BACK|GL_LEFT|GL_RIGHT|GL_FRONT_AND_BACK" />
        // We support GL_BACK_LEFT, GL_BACK_RIGHT
        if ( (viewportElement = viewportNode->FirstChild( "ogldrawbuf" )) &&
            viewportElement->ToElement() ) {
          arGUIXMLValueValidator oglBufValidator( "viewport 'ogldrawbuf'" );
          oglBufValidator( viewportElement );
          GLenum ogldrawbuf;
          const string buf(viewportElement->ToElement()->Attribute( "value" ));
          arGUIXMLAttributeValueValidator oglValValidator( "viewport 'ogldrawbuf'",
             string("GL_BACK_LEFT/GL_BACK_RIGHT") );
          oglValValidator( buf );
          if ( buf == "GL_BACK_LEFT" )      ogldrawbuf = GL_BACK_LEFT;
          else if ( buf == "GL_BACK_RIGHT" )     ogldrawbuf = GL_BACK_RIGHT;
//          if ( buf == "GL_NONE" )                ogldrawbuf = GL_NONE;
//          else if ( buf == "GL_FRONT_LEFT" )     ogldrawbuf = GL_FRONT_LEFT;
//          else if ( buf == "GL_FRONT_RIGHT" )    ogldrawbuf = GL_FRONT_RIGHT;
//          else if ( buf == "GL_BACK_LEFT" )      ogldrawbuf = GL_BACK_LEFT;
//          else if ( buf == "GL_BACK_RIGHT" )     ogldrawbuf = GL_BACK_RIGHT;
//          else if ( buf == "GL_FRONT" )          ogldrawbuf = GL_FRONT;
//          else if ( buf == "GL_BACK" )           ogldrawbuf = GL_BACK;
//          else if ( buf == "GL_LEFT" )           ogldrawbuf = GL_LEFT;
//          else if ( buf == "GL_RIGHT" )          ogldrawbuf = GL_RIGHT;
//          else if ( buf == "GL_FRONT_AND_BACK" ) ogldrawbuf = GL_FRONT_AND_BACK;
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

        if ( namedViewportNode ) {
          delete namedViewportNode;
        }
        if ( camera ) {
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
        ar_log_error() << "arGUIXML configureCamera failed.\n";
      }

      // viewports added by setViewMode will use this camera and screen
      _parsedWindowConstructs.back()->getGraphicsWindow()->setCamera( camera );
      _parsedWindowConstructs.back()->getGraphicsWindow()->setScreen( screen );

      // set up the appropriate viewports
      if ( !_parsedWindowConstructs.back()->getGraphicsWindow()->setViewMode( viewMode ) ) {
        ar_log_error() << "arGUIXML setViewMode failed.\n";
      }

      if ( camera ) {
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
  return true;
}
//@-node:jimc.20100409112755.252:arGUIXMLParser::parse
//@-others
//@-node:jimc.20100409112755.216:@thin graphics\arGUIXMLParser.cpp
//@-leo
