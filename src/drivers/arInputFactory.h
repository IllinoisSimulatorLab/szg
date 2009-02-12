//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_FACTORY_H
#define AR_INPUT_FACTORY_H

#include "arSZGClient.h"
#include "arInputSource.h"
#include "arInputSink.h"
#include "arIOFilter.h"
#include "arStringTextStream.h"
#include "arInputNodeConfig.h"
#include "arDriversCalling.h"

#include <string>
#include <map>
#include <vector>

class SZG_CALL arInputFactory {
  public:
    arInputFactory() : _inputConfig() {}
    arInputFactory( const arInputNodeConfig& inputConfig ) :
       _inputConfig( inputConfig ) {}
    virtual ~arInputFactory();
    bool configure( arSZGClient& szgClient );
    void setInputNodeConfig( const arInputNodeConfig& inputConfig ) {
      _inputConfig = inputConfig;
    }
    arInputSource* getInputSource( const string& driverName );
    arInputSink* getInputSink( const string& sinkName );
    arIOFilter* getFilter( const string& filterName );
    // slotNumber is a bit ugly. You pass in the value of the _output_
    // slot. It may be incremented and/or used as the number for one
    // or more _input_ slots.
    bool loadInputSources( arInputNode& inputNode, unsigned& slotNumber, bool fNetInput=false );
    bool loadInputSinks( arInputNode& inputNode );
    bool loadFilters( arInputNode& inputNode, const string& namedPForthProgram="" );
    arInputSource* findInputSource( const string& driverName );
  private:
    void _printDriverNames( arLogStream& os );
    void _deleteSources();
    arInputNodeConfig _inputConfig;
    arSZGClient* _szgClientPtr;
    map< string, arInputSource* > _sourceNameMap;
#if defined( AR_USE_MINGW ) || defined( AR_LINKING_DYNAMIC) || !defined( AR_USE_WIN_32 )
    string _execPath;
#endif
};

#endif
