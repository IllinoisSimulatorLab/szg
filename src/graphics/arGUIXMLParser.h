#ifndef AR_GUI_XML_PARSER
#define AR_GUI_XML_PARSER

#include "arXMLParser.h"

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

#include <string>
#include <map>

class arGraphicsScreen;
class arCamera;
class arGraphicsWindow;
class arGUIWindowManager;
class arSZGClient;
class arVector3;
class arVector4;

class SZG_CALL arGUIXMLParser {
  public:

    arGUIXMLParser( arGUIWindowManager* wm,
                    std::map<int, arGraphicsWindow* >& windows,
                    arSZGClient& SZGClient,
                    const std::string& config );

    ~arGUIXMLParser( void );

    int parse( void );

    int numberOfWindows( void );

    bool error( void ) { return _doc.Error(); }

  private:

    TiXmlNode* _getNamedNode( const char* name = NULL );

    arVector3 _attributearVector3( TiXmlNode* node,
                                   const std::string& x = "x",
                                   const std::string& y = "y",
                                   const std::string& z = "z" );

    arVector4 _attributearVector4( TiXmlNode* node,
                                   const std::string& x = "x",
                                   const std::string& y = "y",
                                   const std::string& z = "z",
                                   const std::string& w = "w" );

    bool _attributeBool( TiXmlNode* node,
                         const std::string& value = "value" );

    int _configureScreen( arGraphicsScreen& screen,
                          TiXmlNode* screenNode = NULL );

    arCamera* _configureCamera( arGraphicsScreen& screen,
                                TiXmlNode* cameraNode = NULL );

    arGUIWindowManager* _wm;
    arSZGClient* _SZGClient;
    std::map<int, arGraphicsWindow* >* _windows;
    std::string _config, _mininumConfig;

    TiXmlDocument _doc;

};

#endif

