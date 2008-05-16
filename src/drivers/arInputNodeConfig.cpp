//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arXMLUtilities.h"
#include "arStringTextStream.h"
#include "arInputNodeConfig.h"

using std::string;

// The input node configuration looks like this:
// <szg_device>
//   <input_sources>
//     ... list (possibly empty) of input source object names ...
//   </input_sources>
//   <input_sinks>
//     ... list (possibly empty) of input sink object names ...
//   </input_sinks>
//   <input_filters>
//     ... list (possibly empty) of input filters
//   </input_filters>
//   <pforth>
//     ... either empty (all whitespace) or a pforth program ...
//   </pforth>
// </szg_device>

bool arInputNodeConfig::_parseTokenList(arStringTextStream& tokenStream,
                                        const string& tagType,
                                        vector<string>& tokenList) {
  // Get input_sources.
  string tagText( ar_getTagText( &tokenStream ) );
  if (tagText != tagType) {
    ar_log_error() << "arInputNodeConfig parser expected " << tagType << " tag.\n";
    return false;
  }
  arBuffer<char> buffer(128);
  if (!ar_getTextBeforeTag( &tokenStream, &buffer )) {
    ar_log_error() << "arInputNodeConfig failed to parse " << tagType << " field.\n";
    return false;
  }
  stringstream tokens( buffer.data );
  string token;
  do {
    tokens >> token;
    if (!tokens.fail()) {
      if (find(tokenList.begin(), tokenList.end(), token) != tokenList.end()) {
	ar_log_error() << tagType << " arInputNodeConfig ignoring duplicate token '" << token << "'\n";
	continue;
      }
      tokenList.push_back( token );
      ar_log_debug() << tagType << " arInputNodeConfig token '" << token << "'\n";
    }
  } while ( !tokens.eof() );

  // Closing input_sources tag.
  tagText = ar_getTagText( &tokenStream );
  if (tagText != "/"+tagType) {
    ar_log_error() << "arInputNodeConfig parser: expected end tag </" << tagType
      << ">, not <" << tagText << ">.\n";
    return false;
  }
  return true;
}

bool arInputNodeConfig::parseXMLRecord( const string& nodeConfig ) {
  arStringTextStream configStream( nodeConfig );
  arBuffer<char> buffer(128); 
  valid = false;

  string tagText( ar_getTagText( &configStream ) );
  if (tagText != "szg_device") {
    ar_log_error() << "arInputNodeConfig parser expected starting tag <szg_device>, not <"
                     << tagText << ">.\n";
    return false;
  }
  if (!_parseTokenList( configStream, "input_sources", inputSources )) {
    return false;
  }
  if (!_parseTokenList( configStream, "input_sinks", inputSinks )) {
    return false;
  }
  if (!_parseTokenList( configStream, "input_filters", inputFilters )) {
    return false;
  }
  
  tagText = ar_getTagText( &configStream );
  if (tagText != "pforth") {
    ar_log_error() << "arInputNodeConfig parser expected <pforth> tag, not <"
                   << tagText << ">.\n";
    return false;
  }
  if (! ar_getTextBeforeTag( &configStream, &buffer )) {
    ar_log_error() << "arInputNodeConfig parser failed in <pforth> field.\n";
    return false;
  }
  pforthProgram = string( buffer.data );
  tagText = ar_getTagText( &configStream );
  if (tagText != "/pforth") {
    ar_log_error() << "arInputNodeConfig parser expected </pforth> tag, not <"
                   << tagText << ">.\n";
    return false;
  }
  tagText = ar_getTagText( &configStream );
  if (tagText != "/szg_device") {
    ar_log_error() << "arInputNodeConfig parser expected </szg_device> tag, not <"
                   << tagText << ">.\n";
    return false;
  }
  valid = true;
  return true;
}


