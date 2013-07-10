//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_STRUCTURED_DATA
#define AR_STRUCTURED_DATA

class arLanguage;
#include "arDataType.h"
#include "arDataTemplate.h"
#include "arDataUtilities.h"
#include "arLanguage.h" // for error-reporting constructor

#include "arLanguageCalling.h"

#include <iostream>
#include <vector>
using namespace std;

// Data format which gets passed around the cluster.

class SZG_CALL arStructuredData {

 private:
   void copy(arStructuredData const& rhs);
   void destroy();
   void _construct(arDataTemplate*);
 public:
   arStructuredData(arDataTemplate*);
   arStructuredData(arStructuredData const&);
   arStructuredData(arTemplateDictionary* d, const char* name);
   arStructuredData const& operator=(arStructuredData const&);
   ~arStructuredData();
   operator bool() const
     { return _fValid; }

   int getID() const
     { return _dataID; }
   int numberDataItems() const
     { return _numberDataItems; }

   // for manipulating data in a fine-grained way
   void* getDataPtr(int i, arDataType a)
     { return (void*)getConstDataPtr(i, a); }
   const void* getConstDataPtr(int, arDataType) const;
   int getDataDimension(const int) const;      // called by xxxClient
   bool setDataDimension(int, int); // called by xxxServer
   int getStorageDimension(int) const;
   bool setStorageDimension(int, int);
   arDataType getDataType(int fieldIndex) const;

   void* getDataPtr(const string& s, arDataType a)
     { return (void*)getConstDataPtr(s, a); }
   const void* getConstDataPtr(const string&, arDataType) const;
   int getDataDimension(const string&) const;
   bool setDataDimension(const string&, int);
   arDataType getDataType(const string& fieldName) const;

   // for manipulating data one chunk at a time
   bool dataIn(int field, const void* data, arDataType type, int dim); // called by xxxServer
   bool dataOut(int, void*, arDataType, int) const; // called by xxxClient

   // some less efficient methods for data manipulation
   bool dataIn(const string&, const void*,
               arDataType theType=AR_GARBAGE, int dimension=0);
   bool dataOut(const string&, void*, arDataType, int) const;

   int getDataFieldIndex( const string& fieldName ) const;
   bool getDataFieldName( const int index, string& fieldName ) const;
   void getFieldNames( std::vector< std::string >& names ) const;

   // Abbreviations.
   bool dataInString(int id, const string& s)
     { return dataIn(id, s.data(), AR_CHAR, s.length()); }
   bool dataInString(const string& fieldName, const string& s)
     { return dataIn(fieldName, s.data(), AR_CHAR, s.length()); }
   bool dataInString(int id, const char* s)
     { return dataIn(id, s, AR_CHAR, strlen(s)); }
   bool dataInString(const string& fieldName, const char* s)
     { return dataIn(fieldName, s, AR_CHAR, strlen(s)); }
   string getDataString(int field);
   string getDataString(const string& fieldName);
   int getDataInt(int id) const
     { int x; return dataOut(id, &x, AR_INT, 1) ? x : -1; }
   int getDataInt(const string& fieldName) const
     { int x; return dataOut(fieldName, &x, AR_INT, 1) ? x : -1; }
   float getDataFloat(int id) const
     { float x; return dataOut(id, &x, AR_FLOAT, 1) ? x : 0.; }
   float getDataFloat(const string& fieldName) const
     { float x; return dataOut(fieldName, &x, AR_FLOAT, 1) ? x : 0.; }

   // For manipulating big chunks of data with fewer mem copies.
   // Experimenting here with not requiring data type.
   // Called only by xxxClient, I think.
   bool ptrIn(int, void*, int);

   // byte stream representation
   int size() const;           // # of bytes when packed
   bool pack(ARchar*) const;   // from internal representation to byte stream
   bool unpack(const ARchar*); // from byte stream to internal representation
   bool parse(ARchar*);        // set pointers into char buffer. unowned data.

   // debugging info
   const string& getName() const
     { return _name; }
   void dump(bool verbosity = false);

   // XML I/O
   void print(ostream& s) const;
   void print(FILE*) const;
   void print() const
     { print(stdout); }

 private:
   bool _fValid;             // got constructed okay
   ARint _dataID;            // the overall ID of this sort of data
   ARint _numberDataItems;   // how many data items are we managing?
   void** _dataPtr;        // an array of pointers to data
   bool*  _owned;          // is the particular chunk of memory owned
                           // by this object or not
   ARint*   _dataDimension;  // dimension associated with each pointer
   ARint*   _storageDimension; // might have extra storage beyond the data
   arDataType* _dataType;  // array: each pointer's data type
   string* _dataName;  // array: each pointer's name
   typedef map< string, int, less<string> > arNameMap;
   arNameMap _dataNameMap;
   string _name; // name of arDataTemplate this was derived from
   static arLock _dumpLock; // Don't interleave multiple threads' output.
};

ostream& operator<<(ostream& s, const arStructuredData& d);

bool ar_getStructuredDataStringField( arStructuredData* dataPtr,
                         const string& name,
                         string& value );

#endif
