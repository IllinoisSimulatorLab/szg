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

    arGUIXMLWindowConstruct( arGUIWindowConfig* windowConfig = NULL,
                             arGraphicsWindow* graphicsWindow = NULL,
                             arGUIRenderCallback* guiDrawCallback = NULL );
    ~arGUIXMLWindowConstruct( void );

    void setWindowConfig( arGUIWindowConfig* windowConfig ) { _windowConfig = windowConfig; }
    void setGraphicsWindow( arGraphicsWindow* graphicsWindow ) { _graphicsWindow = graphicsWindow; }
    void setGUIDrawCallback( arGUIRenderCallback* guiDrawCallback ) { _guiDrawCallback = guiDrawCallback; }

    // conceptually both of these pointers should /probably/ be returned const,
    // but there's places they're used where it's not currently possible to do so
    arGUIWindowConfig* getWindowConfig( void ) const { return _windowConfig; }
    arGraphicsWindow* getGraphicsWindow( void ) const { return _graphicsWindow; }

    arGUIRenderCallback* getGUIDrawCallback( void ) const { return _guiDrawCallback; }

  private:

    arGUIWindowConfig* _windowConfig;
    arGraphicsWindow* _graphicsWindow;

    arGUIRenderCallback* _guiDrawCallback;

};

class SZG_CALL arGUIWindowingConstruct {
  public:
    arGUIWindowingConstruct( int threaded = -1, int useFramelock = -1,
                             std::vector< arGUIXMLWindowConstruct* >* windowConstructs = NULL );
    ~arGUIWindowingConstruct( void );

    void setThreaded( int threaded ) { _threaded = threaded; }
    void setUseFramelock( int useFramelock ) { _useFramelock = useFramelock; }
    void setWindowConstructs( std::vector< arGUIXMLWindowConstruct* >* windowConstructs ) { _windowConstructs = windowConstructs; }

    int getThreaded( void ) const { return _threaded; }
    int getUseFramelock( void ) const { return _useFramelock; }
    std::vector< arGUIXMLWindowConstruct* >* getWindowConstructs( void ) const { return _windowConstructs; }

  private:

    // both of these are set to -1 if they have not been *explicitly* set from
    // the xml
    int _threaded;
    int _useFramelock;

    std::vector< arGUIXMLWindowConstruct* >* _windowConstructs;
};

class SZG_CALL arGUIXMLParser {
  public:

    // all the functions off the SZGClient that the arGUIXMLParser needs to
    // call /should/ be made const, but they arent right now so this has
    // to be passed in non-const
    arGUIXMLParser( /* arGUIWindowManager* wm = NULL, */
                    arSZGClient* SZGClient = NULL,
                    const std::string& config = "" );

    ~arGUIXMLParser( void );

    void setConfig( const std::string& config );
    std::string getConfig( void ) const { return _config; }

    void setSZGClient( arSZGClient* SZGClient ) { _SZGClient = SZGClient; }
    // void setWindowManager( arGUIWindowManager* wm ) { _wm = wm; }

    int parse( void );

    bool error( void ) const { return _doc.Error(); }

    arGUIWindowingConstruct* getWindowingConstruct( void ) const { return _windowingConstruct; }

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

    // arGUIWindowManager* _wm;
    arSZGClient* _SZGClient;

    arGUIWindowingConstruct* _windowingConstruct;

    std::vector< arGUIXMLWindowConstruct* > _parsedWindowConstructs;
    // std::vector< arGUIXMLWindowConstruct* > _createdWindowConstructs;

    std::string _config, _mininumConfig;

    TiXmlDocument _doc;

};

#endif

