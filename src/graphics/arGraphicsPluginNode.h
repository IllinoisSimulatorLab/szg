//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_PLUGIN_NODE_H
#define AR_GRAPHICS_PLUGIN_NODE_H

#include "arGraphicsNode.h"
#include "arGraphicsPlugin.h"
#include "arSharedLib.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

#include <vector>

class SZG_CALL arGraphicsPluginNode: public arGraphicsNode {
  public:
    arGraphicsPluginNode();
    virtual ~arGraphicsPluginNode();

    void draw(arGraphicsContext*);

    arStructuredData* dumpData();
    bool receiveData(arStructuredData*);

    /// Data access functions specific to arGraphicsPluginNode.
    static void setSharedLibSearchPath( const std::string& searchPath );


  protected:
    arStructuredData* _dumpData( const string& fileName,
                                 std::vector<int>& intData,
                                 std::vector<long>& longData,
                                 std::vector<float>& floatData,
                                 std::vector<double>& doubleData,
                                 std::vector< std::string >& stringData,
                                 bool owned );
    arGraphicsPlugin* _makeObject();
    static arSharedLib* _getSharedLib( const std::string& fileName );
    static std::map< std::string, arSharedLib* > __sharedLibMap;
    static std::string __sharedLibSearchPath;
    arGraphicsPlugin* _object;
    bool _triedToLoad;
    std::string _fileName;
    std::vector<int> _intData;
    std::vector<long> _longData;
    std::vector<float> _floatData;
    std::vector<double> _doubleData;
    std::vector< std::string > _stringData;
};

#endif
