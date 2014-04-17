#include "arPrecompiled.h"
#include "arExperimentUtilities.h"
#include <sstream>

using std::string;
using std::vector;

bool ar_getStringField( arStructuredData* dataPtr,
                        const string name,
                        string& value ) {
  char* charPtr;
  if ((charPtr = (char*)dataPtr->getDataPtr(name,AR_CHAR))==NULL) {
    cerr << "ar_getStringField error: failed to get pointer to field " << name << endl;
    return false;
  }
  int dataSize = dataPtr->getDataDimension(name);
  if (dataSize==0) {
    cerr << "ar_getStringField error: empty field " << name << endl;
    return false; // we'll take an empty field as a failure here
  }
  char *stringPtr = new char[dataSize+1];
  if (stringPtr==0) {
    cerr << "ar_getStringField error: memory panic\n";
    return false;
  }
  memcpy( stringPtr, charPtr, dataSize );
  stringPtr[dataSize] = '\0';
  value = string( stringPtr );
  delete[] stringPtr;
  return true;
}

bool ar_extractTokenList( arStructuredData* dataPtr,
                          const string name,
                          vector<string>& outList,
                          const char delim ) {
  string tempString;
  if (!ar_getStringField( dataPtr, name, tempString )) {
    cerr << "ar_extractTokenList error: unable to extract " << name
         << " list from file header.\n";
    return false;
  }
  if (!ar_getTokenList( tempString, outList, delim )) {
    cerr << "ar_extractTokenList error: unable to tokenize " << name << " list.\n";
    return false;
  }
//  vector<string>::const_iterator iter;
//  for (iter = outList.begin(); iter != outList.end(); iter++)
//    cerr << *iter << endl;
  return true;
}
