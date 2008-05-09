//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INPUT_NODE_CONFIG_H
#define AR_INPUT_NODE_CONFIG_H

#include "arSZGClient.h"
#include "arInputSource.h"
#include "arInputSink.h"
#include "arIOFilter.h"
#include "arStringTextStream.h"
#include "arDriversCalling.h"

#include <string>
#include <map>
#include <vector>

class SZG_CALL arInputNodeConfig {
  public:
    arInputNodeConfig(): pforthProgram(""), valid(false) {}
    virtual ~arInputNodeConfig() {}

    void addInputSource( const string& source ) { inputSources.push_back( source ); }
    bool parseXMLRecord( const string& nodeConfig );

    vector<string> inputSources;
    vector<string> inputSinks;
    vector<string> inputFilters; 
    string       pforthProgram;
    bool valid;

  private:
    bool _parseTokenList(arStringTextStream& tokenStream,
                          const string& tagType,
                          vector<string>& tokenList);
};

#endif

