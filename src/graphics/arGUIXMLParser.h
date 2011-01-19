//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// For classes arGUIXMLWindowConstruct, arGUIWindowingConstruct, and arGUIXMLParser.

#ifndef AR_GUI_XML_PARSER
#define AR_GUI_XML_PARSER

#include "arXMLParser.h"
#include "arGraphicsCalling.h"

#include <string>
#include <map>
#include <vector>

// forward declarations
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

/**
 * Window configuration object populated with state from parsed XML.
 * Holds the associated arGraphicsWindow and a possible draw callback.
 *
 * @see arGUIWindowingConstruct::getWindowConstructs
 * @see arGUIXMLParser::parse
 */
class SZG_CALL arGUIXMLWindowConstruct {
  public:
    arGUIXMLWindowConstruct( arGUIWindowConfig* windowConfig = NULL,
                             arGraphicsWindow* graphicsWindow = NULL,
                             arGUIRenderCallback* guiDrawCallback = NULL );

    ~arGUIXMLWindowConstruct( void );

    // Accessors.
    void setWindowConfig( arGUIWindowConfig* windowConfig ) { _windowConfig = windowConfig; }
    void setGraphicsWindow( arGraphicsWindow* graphicsWindow ) { _graphicsWindow = graphicsWindow; }
    void setGUIDrawCallback( arGUIRenderCallback* guiDrawCallback ) { _guiDrawCallback = guiDrawCallback; }

    // Both of these pointers should probably be returned const,
    // but there's places they're used where it's not currently possible to do so
    arGUIWindowConfig* getWindowConfig( void ) const { return _windowConfig; }
    arGraphicsWindow* getGraphicsWindow( void ) const { return _graphicsWindow; }

    arGUIRenderCallback* getGUIDrawCallback( void ) const { return _guiDrawCallback; }

  private:
    arGUIWindowConfig* _windowConfig;       // A window configuration object.
    arGraphicsWindow* _graphicsWindow;      // An associated arGraphicsWindow.
    arGUIRenderCallback* _guiDrawCallback;  // A arGUIWindow draw callback.

};

/**
 * Configuration object, passed to an arGUIWindowManager to create
 * a set of arGUIWindows.
 *
 * @see arGUIWindowManager::createWindows
 * @see arGUIXMLParser::getWindowingConstruct
 */
class SZG_CALL arGUIWindowingConstruct {
  public:

    /**
     * Constructor.
     * @param threaded         Should the arGUIWindowManager be threaded.
     * @param useFramelock     Should the arGUIWindowManager use framelocking.
     * @param windowConstructs windowConstructs to be created.
     */
    arGUIWindowingConstruct( int threaded = -1, int useFramelock = -1,
                             int useExtraBufSwap = -1,
                             std::vector< arGUIXMLWindowConstruct* >* windowConstructs = NULL );

    ~arGUIWindowingConstruct( void );

    // Accessors.
    void setThreaded( int threaded ) { _threaded = threaded; }
    int getThreaded( void ) const { return _threaded; }

    void setUseFramelock( int useFramelock ) { _useFramelock = useFramelock; }
    int getUseFramelock( void ) const { return _useFramelock; }

    void setUseExtraStereoBufferSwap( int useExtraBufSwap ) { _useExtraStereoBufferSwap = useExtraBufSwap; }
    int getUseExtraStereoBufferSwap( void ) const { return _useExtraStereoBufferSwap; }

    void setWindowConstructs( std::vector< arGUIXMLWindowConstruct* >* windowConstructs ) { _windowConstructs = windowConstructs; }
    // conceptually, should probably be returned const however there are places
    // it's used where this is not possible
    std::vector< arGUIXMLWindowConstruct* >* getWindowConstructs( void ) const { return _windowConstructs; }

  private:
    int _threaded;        // Whether or not to use threading, -1 if not explicitly set from the XML.
    int _useFramelock;    // Whether or not to use framelocking, -1 if not explicitly set from the XML.
    int _useExtraStereoBufferSwap; // Workaround for nVidia active stereo swap bug;
            // buffer swap required between right and left eye buffer rendering.

    std::vector< arGUIXMLWindowConstruct* >* _windowConstructs;   // A collection of windows to be created.
};

/**
 * A utility class that takes an XML configuration string and parses it to
 * create a set of usable window configuration objects.
 */
class SZG_CALL arGUIXMLParser {
  public:

    /**
     * Constructor.
     *
     * @note The functions used off the arSZGClient all <em>should</em> be
     *        const, but they aren't right now so the reference is passed
     *        in as non-const.
     *
     * @param SZGClient A reference to the arSZGClient from which to get
     *                  additional XML strings.
     * @param config    An initial XML configuration string.
     */
    arGUIXMLParser( arSZGClient* SZGClient = NULL, const std::string& displayName = "" );

    ~arGUIXMLParser( void );

    /**
     * Parse the XML configuration string.
     * Creates some arGUIXMLWindowConstructs.
     *
     * @note If any configuration objects existed from a previous call to
     *       parse, they will be cleared out each successive call to parse.
     *
     * @return <ul>
     *           <li>0 on success.
     *           <li>-1 if xml configuration string could not be parsed.
     *         </ul>
     */
    bool parse( void );

    // Accessors.
    void setDisplayName( const std::string& displayName );
    std::string getDisplayName( void ) const { return _displayName; }
    void setSZGClient( arSZGClient* SZGClient ) { _SZGClient = SZGClient; }
    bool error( void ) const { return _doc.Error(); }
    arGUIWindowingConstruct* getWindowingConstruct( void ) const { return _windowingConstruct; }

  private:

    /**
     * Get another XML configuration string from the arSZGClient.
     *
     * @note The caller is responsible for cleaning up the returned pointer.
     *
     * @return <ul>
     *           <li>A pointer to the node on success.
     *           <li>NULL if the node could not be found.
     *         </ul>
     *
     * @see parse
     * @see _configureScreen
     * @see _configureCamera
     */
    TiXmlNode* _getNamedNode( const char* name = NULL, const std::string& nodeType="NULL" );

    /**
     * Forward diagnostics to ar_log_error().
     * @see _getNamedNode
     */
    void _reportParseError( TiXmlDocument* nodeDoc, const std::string& nodeDesc );
    
    /**
     * Construct an arVector3 from an XML element.
     *
     * @param node A pointer to the XML element.
     * @param x    The name of the first attribute.
     * @param y    The name of the second attribute.
     * @param z    The name of the third attribute.
     *
     * @return An arVector3 (with default values if the node could not be found).
     */
    arVector3 _attributearVector3( TiXmlNode* node,
                                   const std::string& name,
                                   const std::string& x = "x",
                                   const std::string& y = "y",
                                   const std::string& z = "z" );

    /**
     * Construct an arVector4 from an XML element.
     *
     * @param node A pointer to the XML element.
     * @param x    The name of the first attribute.
     * @param y    The name of the second attribute.
     * @param z    The name of the third attribute.
     * @param w    The name of the third attribute.
     *
     * @return An arVector4 (with default values if the node could not be found).
     */
    arVector4 _attributearVector4( TiXmlNode* node,
                                   const std::string& name,
                                   const std::string& x = "x",
                                   const std::string& y = "y",
                                   const std::string& z = "z",
                                   const std::string& w = "w" );

    /**
     * Convert an XML element to a Boolean.
     *
     * @param node A pointer to the XML element.
     * @param value The name of the attribute to be converted.
     *
     * @return The element as a boolean.
     */
    bool _attributeBool( TiXmlNode* node,
                         const std::string& value = "value" );

    /**
     * Construct an arGraphicsScreen from an XML node.
     *
     * @param screen     A reference to the arGraphicsScreen whose state should
     *                   be set according to the XML node.
     * @param screenNode A pointer to the XML node.
     *
     * @return 0 (unused)
     *
     * @see _configureCamera
     */
    int _configureScreen( arGraphicsScreen& screen,
                          TiXmlNode* screenNode = NULL );

    /**
     * Construct an arCamera from an XML node.
     *
     * @param screen     A reference to the arGraphicsScreen who should be set
     *                   on the new camera.
     * @param cameraNode A pointer to the XML node.
     *
     * @return A pointer to a new arCamera (an aVRCamera by default).
     *
     * @see _configureScreen
     */
    arCamera* _configureCamera( arGraphicsScreen& screen,
                                TiXmlNode* cameraNode = NULL );

    arSZGClient* _SZGClient;
    arGUIWindowingConstruct* _windowingConstruct;     // Set up by parse()
    std::vector< arGUIXMLWindowConstruct* > _parsedWindowConstructs;
    std::string _displayName, _mininumConfig;              // XML configuration strings.
    TiXmlDocument _doc;

};

#endif
