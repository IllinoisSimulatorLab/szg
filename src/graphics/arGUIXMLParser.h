#ifndef AR_GUI_XML_PARSER
#define AR_GUI_XML_PARSER

#include "arXMLParser.h"

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

#include <string>
#include <map>
#include <vector>

class arGraphicsScreen;
class arCamera;
class arGraphicsWindow;
class arGUIWindowManager;
class arGUIWindowConfig;
class arSZGClient;
class arVector3;
class arVector4;
class arWindowInitCallback;
class arRenderCallback;
class arGUIRenderCallback;
class arHead;

class SZG_CALL arGUIXMLWindowConstruct {
  public:

    arGUIXMLWindowConstruct( int windowID = -1,
                             arGUIWindowConfig* windowConfig = NULL,
                             arGraphicsWindow* graphicsWindow = NULL );
    ~arGUIXMLWindowConstruct( void );

    void setWindowID( int windowID ) { _windowID = windowID; }
    void setWindowConfig( arGUIWindowConfig* windowConfig ) { _windowConfig = windowConfig; }
    void setGraphicsWindow( arGraphicsWindow* graphicsWindow ) { _graphicsWindow = graphicsWindow; }

    int getWindowID( void ) const { return _windowID; }
    arGUIWindowConfig* getWindowConfig( void ) const { return _windowConfig; }
    arGraphicsWindow* getGraphicsWindow( void ) const { return _graphicsWindow; }

  private:

    int _windowID;                        ///< -1 if the window has not yet been created
    arGUIWindowConfig* _windowConfig;
    arGraphicsWindow* _graphicsWindow;

};

class SZG_CALL arGUIXMLParser {
  public:

    // all the functions off the SZGClient that the arGUIXMLParser needs to
    // call /should/ be made const, but they arent right now so this has
    // to be passed in non-const
    arGUIXMLParser( arGUIWindowManager* wm = NULL,
                    arSZGClient* SZGClient = NULL,
                    const std::string& config = "" );

    ~arGUIXMLParser( void );

    void setConfig( const std::string& config );
    std::string getConfig( void ) const { return _config; }

    void setSZGClient( arSZGClient* SZGClient ) { _SZGClient = SZGClient; }
    void setWindowManager( arGUIWindowManager* wm ) { _wm = wm; }

    int parse( void );

    // NOTE: since the same callback pointer is used for each {arGUI|arGraphics}
    // window in the _windowsConstructs, this means that those windows do *not*
    // own their callbacks and should *not* ever call delete on them
    int createWindows( arWindowInitCallback* initCB = NULL,
                       arRenderCallback* graphicsDrawCB = NULL,
                       arGUIRenderCallback* guiDrawCB = NULL,
                       arHead* head = NULL );

    int numberOfWindows( void );

    bool error( void ) const { return _doc.Error(); }

    const std::vector< arGUIXMLWindowConstruct* >* getWindowConstructs( void ) const { return &_windowConstructs; }

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

    std::vector< arGUIXMLWindowConstruct* > _windowConstructs;

    std::string _config, _mininumConfig;

    TiXmlDocument _doc;

};

#endif

