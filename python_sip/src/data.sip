//
// Important note: argument labels in methods with %MethodCode blocks, e.g. in:
//
//   void fromSequence( SIP_PYOBJECT a0 );
//
// The 'a0' is there for clarity only and is ignored by the SIP parser.
// In particular, you can't change these labels because SIP automatically maps the first
// argument to 'a0', second to 'a1', and so on.
//
// The 'this' pointer is mapped by SIP onto the variable 'sipCpp', but there's a gotcha:
// Confusingly, if what is implemented as an object method is implemented in a
// global operator C++, as in the following case:
//
//    arVector3 __add__( arVector3 a1 );
//
// Which is implemented in C++ as:
//
//    arVector3 operator+( const arVector3&, const arVector3& );
//
// ...then the 'self-as-C++-object' pointer ends up in a0.

//%Module szgmath 0

// Defines mappings from python to C++ types.
%Include typemaps.sip

enum { SZG_VERSION_NUMBER = 2 };

class arLanguage {
%TypeHeaderCode
#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arLanguage.h"
%End
 public:
  arLanguage();
  virtual ~arLanguage();

  arTemplateDictionary* getDictionary();
  arStructuredData* makeDataRecord(int);
  arDataTemplate* find(const char* name);
  arDataTemplate* find(const string& name);
  arDataTemplate* find(int);
};


// arDataType.h

// IDs of data types.

// Because these type codes are encoded in binary records,
// for backwards compatibility only *append* new data types.
// Do not modify these existing values.
enum arDataType {
  AR_GARBAGE=0, AR_CHAR=1, AR_INT=2, AR_LONG=3, AR_FLOAT=4, AR_DOUBLE=5, AR_INT64=6
};

// Get the size and name of a data type from its ID.

int arDataTypeSize(arDataType theType);
const char* arDataTypeName(arDataType theType);
arDataType arDataNameType(const char* theName);




class arDataTemplate {
%TypeHeaderCode
#include "arDataType.h"
#include "arDataUtilities.h"
#include "arDataTemplate.h"
%End
 public:
   arDataTemplate();
   arDataTemplate(const string&, int templateID = -1);
   arDataTemplate(const arDataTemplate&);
//   arDataTemplate& operator=( const arDataTemplate& dataTemplate );
   ~arDataTemplate();

   int add(const string&, arDataType);
   void addAttribute(const string& s, arDataType d); // backwards-compatible
   void setName(const string&);
   string getName() const;
   int getID() const;
   void setID(int ID);
   int getAttributeID(const string&) const;
   arDataType getAttributeType(const string&) const;
   int getNumberAttributes() const;
   void dump() const;
//   int translate(ARchar*, ARchar*, arStreamConfig);

//   arAttribute::iterator attributeBegin();
//   arAttribute::iterator attributeEnd();
//   arAttribute::const_iterator attributeConstBegin() const;
//   arAttribute::const_iterator attributeConstEnd() const;
};

bool ar_addAttributesFromString( arDataTemplate& t,
  const string& nameString, const string& typeString );


class arTemplateDictionary {
%TypeHeaderCode
#include "arDataTemplate.h"
#include "arDataUtilities.h"
#include "arTemplateDictionary.h"
%End
 public:
   arTemplateDictionary();
   // for the common case of adding a single template
   arTemplateDictionary(arDataTemplate*);
   ~arTemplateDictionary();

   int add(arDataTemplate*); // Returns the ID which gets assigned to it.
   arDataTemplate* find(const string&);
   arDataTemplate* find(int);
   void renumber();

   // Read the template list.
   int numberTemplates() const;
//   arTemplateType::iterator begin()
//   arTemplateType::iterator end()

   // Byte-stream representation.
   // AARGH! THESE ARE BROKEN WITH RESPECT TO RENUMBERING!
   // FORTUNATELY, THAT ONLY OCCURS IN arMasterSlaveDataRouter...
   int size() const;
   void pack(char*) const;
//   bool unpack(ARchar*, arStreamConfig);
   bool unpack(char* buf);
   void dump();
};


// Data format which gets passed around the cluster.

class arStructuredData {
%TypeHeaderCode
#include "arDataType.h"
#include "arDataTemplate.h"
#include "arDataUtilities.h"
#include "arLanguage.h" // for error-reporting constructor
#include "arStructuredData.h"
%End
 private:
   arStructuredData();
 public:
//   arStructuredData(arDataTemplate*);
//   arStructuredData(arStructuredData const&);
   arStructuredData(arTemplateDictionary* d, const char* name);
//   arStructuredData const& operator=(arStructuredData const&);
   ~arStructuredData();

   operator bool() const;
   int getID() const;
   int numberDataItems() const;

   // for manipulating data in a fine-grained way
//   void* getDataPtr(int i, arDataType a);
   // Need analogs of this for other data types...
   vector<int>* getDataIntList( int id );
%MethodCode
      int* dataPtr = static_cast<int*>(sipCpp->getDataPtr( a0, AR_INT ));
      if (!dataPtr) {
        PyErr_SetString( PyExc_RuntimeError, "arStructuredData::getDataPtr() failed." );
        sipIsErr = true;
      } else {
        int size = sipCpp->getDataDimension( a0 );
        sipRes = new vector<int>( dataPtr, dataPtr+size );
      }
%End

   const void* getConstDataPtr(int, arDataType) const;
   int getDataDimension(const int) const;      // called by xxxClient
   bool setDataDimension(int, int); // called by xxxServer
   int getStorageDimension(int) const;
   bool setStorageDimension(int, int);
   arDataType getDataType(int fieldIndex) const;

   void* getDataPtr(const string& s, arDataType a);
   const void* getConstDataPtr(const string&, arDataType) const;
   int getDataDimension(const string&) const;
   bool setDataDimension(const string&, int);
   arDataType getDataType(const string& fieldName) const;

   // for manipulating data one chunk at a time
   bool dataInInt( int field, vector<int>* data );
%MethodCode
     int* tmp = new int[a1->size()];
     if (!tmp) {
       PyErr_SetString( PyExc_RuntimeError, "dataInInt() failed to allocate buffer" );
       sipIsErr = 1;
     } else {
       int* it = tmp;
       vector<int>::iterator iter;
       for (iter = a1->begin(); iter != a1->end(); ++iter) {
         *it++ = *iter;
       }
       sipRes = sipCpp->dataIn( a0, (const void*)tmp, AR_INT, a1->size() );
       delete[] tmp;
     }
%End

//   bool dataIn(int field, const void* data, arDataType type, int dim); // called by xxxServer
//   bool dataOut(int, void*, arDataType, int) const; // called by xxxClient

   // some less efficient methods for data manipulation
//   bool dataIn(const string&, const void*,
//               arDataType theType=AR_GARBAGE, int dimension=0);
//   bool dataOut(const string&, void*, arDataType, int) const;

   int getDataFieldIndex( const string& fieldName ) const;
   bool getDataFieldName( const int index, string& fieldName ) const;
   void getFieldNames( vector<string>& names /Out/ ) const;

   // Abbreviations.
   bool dataInString(int id, const string& s);
   bool dataInString(const string& fieldName, const string& s);
   string getDataString(int field);
   string getDataString(const string& fieldName);
   int getDataInt(int id) const;
   int getDataInt(const string& fieldName) const;
   float getDataFloat(int id) const;
   float getDataFloat(const string& fieldName) const;

   // For manipulating big chunks of data with fewer mem copies.
   // Experimenting here with not requiring data type.
   // Called only by xxxClient, I think.
//   bool ptrIn(int, void*, int);

   // byte stream representation
   int size() const;           // # of bytes when packed
   bool pack(char*) const;   // from internal representation to byte stream
   bool unpack(const char*); // from byte stream to internal representation
   bool parse(char*);        // set pointers into char buffer. unowned data.

   // debugging info
   const string& getName() const;
   void dump(bool verbosity = false);

   // XML I/O
//   void print(ostream& s) const;
//   void print(FILE*) const;
   void print() const;
};


// Send data to arDataClient objects.

class arDataServer /*: public arDataPoint*/ {
%TypeHeaderCode
#include "arDataPoint.h"
#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arStructuredDataParser.h"
#include "arDataUtilities.h"
#include "arQueuedData.h"
%End
 // Needs assignment operator and copy constructor, for pointer members.
 private:
   arDataServer();
 public:
   arDataServer(int dataBufferSize);
   ~arDataServer();

   // If incoming packets are processed by a consumption callback,
   // packets consumed in different threads (coming from different sockets)
   // are consumed atomically by default.
   // Call atomicReceive(false) to let consumptions overlap.
   void atomicReceive(bool);

   // set IP:port on which server listens;  default is INADDR_ANY.
   bool setInterface(const string&);
   bool setPort(int);
   bool beginListening(arTemplateDictionary*);

   int getPort() const;
   const string& getInterface() const;

   // delete a connection (whatever that means)
   bool removeConnection(int id);
   // accept a single new connection
   arSocket* acceptConnection();
   // accept a single new connection... but do not add to the sending list yet
   arSocket* acceptConnectionNoSend();
   // add the passive sockets to the send queue
   void activatePassiveSockets();
   void activatePassiveSocket(int);
   // are there any passive sockets?
   bool checkPassiveSockets();
//   list<arSocket*>* getActiveSockets();

   // Send data to all connected clients.
   bool sendData(arStructuredData*);
//   bool sendDataQueue(arQueuedData*);

   // Send data to someone in particular.
   bool sendData(arStructuredData*, arSocket*);
   bool sendDataNoLock(arStructuredData*, arSocket*);
//   bool sendDataQueue(arQueuedData*, arSocket*);

   // Send data to a group of someone's in particular.
//   bool sendDataQueue(arQueuedData*, list<arSocket*>*);

//   void setConsumerFunction
//     (void (*consumerFunction)(arStructuredData*, void*, arSocket*));
   void setConsume( bool onoff );
   virtual void onConsumeData( arStructuredData* data, arSocket* socket );
   void setConsumerObject(void*);

   int getNumberConnected() const;       // passive and active connections
   int getNumberConnectedActive() const; // exclude passive connections

   virtual void onDisconnect( arSocket* theSocket );
//   void setDisconnectFunction
//     (void (*disconnectFunction)(void*, arSocket*));
   void setDisconnectObject(void*);

   // For the database of labels, and choosing a connection based on a given ID
   string dumpConnectionLabels();
   arSocket* getConnectedSocket(int theSocketID);
   arSocket* getConnectedSocketNoLock(int theSocketID);
   void setSocketLabel(arSocket* theSocket, const string& theLabel);
   string getSocketLabel(int theSocketID);
   int getFirstIDWithLabel(const string& theSocketLabel);

   // Accept connections.
   int dialUpFallThrough(const string& s, int port);

   // Restrict connections to certain IPs (see arSocket and arSocketAddress).
   void setAcceptMask(list<string>& mask);
};
