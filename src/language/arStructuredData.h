//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_STRUCTURED_DATA
#define AR_STRUCTURED_DATA

class arLanguage;
#include "arDataType.h"
#include "arDataTemplate.h"
#include "arDataUtilities.h"
#include "arLanguage.h" // for error-reporting constructor

//*******************************************************************
// AAAAAAARRRRRRRGGGHHHHHHH
#include "arThread.h"
//*******************************************************************

#include <iostream>
#include <vector>
using namespace std;

/// Data format which gets passed around the cluster.

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
   
   /// useful for manipulating data in a fine-grained way
   void* getDataPtr(int,arDataType);
   int getDataDimension(int);      ///< called by xxxClient
   bool setDataDimension(int,int); ///< called by xxxServer
   int getStorageDimension(int);
   bool setStorageDimension(int,int);
   arDataType getDataType(int fieldIndex) const;

   void* getDataPtr(const string&, arDataType);
   int getDataDimension(const string&);
   bool setDataDimension(const string&, int); 
   arDataType getDataType(const string& fieldName);
   
   /// useful for manipulating data one chunk at a time
   bool dataIn(int,const void*,arDataType,int); ///< called by xxxServer
   bool dataOut(int,void*,arDataType,int);      ///< called by xxxClient

   /// some less efficient methods for data manipulation
   bool dataIn(const string&, const void*, 
               arDataType theType=AR_GARBAGE, int dimension=0);
   bool dataOut(const string&, void*, arDataType, int);
   
   int getDataFieldIndex( const string& fieldName );
   bool getDataFieldName( int index, string& fieldName );
   void getFieldNames( std::vector< std::string >& names ) const;

   /// common abbreviations
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
   int getDataInt(int id)
     { int x; return dataOut(id, &x, AR_INT, 1) ? x : -1; }
   float getDataFloat(int id)
     { float x; return dataOut(id, &x, AR_FLOAT, 1) ? x : 0.; }

   /// Useful for manipulating big chunks of data with fewer mem copies.
   /// Experimenting here with not requiring data type.
   /// Called only by xxxClient, I think.
   bool ptrIn(int,void*,int);
   
   // byte stream representation
   int size() const;     ///< how many bytes will this data be when packed
   void pack(ARchar*);   ///< from internal representation to byte stream
   bool unpack(ARchar*); ///< from byte stream to internal representation
   bool parse(ARchar*);  ///< set pointers into char buffer... do not own data

   // debugging info
   const string& getName() const
     { return _name; }
   void dump(bool verbosity = false);

   // XML I/O
   void print();
   void print(ostream& s);
   void print(FILE*);

 private:
   bool _fValid;             // got constructed okay
   ARint _dataID;            // the overall ID of this sort of data
   ARint _numberDataItems;   // how many data items are we managing?
   void** _dataPtr;        // an array of pointers to data
   bool*  _owned;          // is the particular chunk of memory owned
                           // by this object or not
   ARint*   _dataDimension;  // dimension associated with each pointer
   ARint*   _storageDimension; // might have extra storage beyond the data
   arDataType* _dataType;  // the data type associated with each pointer
   string* _dataName;  // the name associated with each pointer
   typedef map< string,int,less<string> > arNameMap;
   arNameMap _dataNameMap;
   string _name; // name of arDataTemplate this was derived from
};

ostream& operator<<(ostream& s, const arStructuredData& d);

bool ar_getStructuredDataStringField( arStructuredData* dataPtr,
                         const string& name,
                         string& value );

#endif
