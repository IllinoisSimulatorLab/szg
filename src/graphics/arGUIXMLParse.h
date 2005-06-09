#ifndef AR_GUI_XML_PARSE
#define AR_GUI_XML_PARSE

#include "arMath.h"
#include "arXMLParser.h"
#include "arSZGClient.h"
#include "arGUIWindow.h"
#include "arGUIWindowManager.h"
#include "arGraphicsWindow.h"
#include "arGraphicsScreen.h"
#include "arCamera.h"
#include <string>
#include <map>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

SZG_CALL TiXmlNode* getNamedNode( arSZGClient& SZGClient, const char* name = NULL );

SZG_CALL arVector3 AttributearVector3( TiXmlNode* node,
                                       const std::string& x = "x",
                                       const std::string& y = "y",
                                       const std::string& z = "z" );

SZG_CALL arVector4 AttributearVector4( TiXmlNode* node,
                                       const std::string& x = "x",
                                       const std::string& y = "y",
                                       const std::string& z = "z",
                                       const std::string& w = "w" );

SZG_CALL bool AttributeBool( TiXmlNode* node,
                             const std::string& value = "value" );

SZG_CALL int configureScreen( arSZGClient& SZGClient,
                              arGraphicsScreen& screen,
                              TiXmlNode* screenNode = NULL );

SZG_CALL arCamera* configureCamera( arSZGClient& SZGClient,
                                    arGraphicsScreen& screen,
                                    TiXmlNode* cameraNode = NULL );

SZG_CALL int parseGUIXML( arGUIWindowManager* wm,
                          std::map<int, arGraphicsWindow* >& windows,
                          arSZGClient& SZGClient,
                          const std::string& config );

#endif

