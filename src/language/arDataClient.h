//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATA_CLIENT
#define AR_DATA_CLIENT

#include "arDataPoint.h"
#include "arStructuredData.h"
#include "arTemplateDictionary.h"
#include "arLanguageCalling.h"

// Get data from an arDataServer.

class SZG_CALL arDataClient : public arDataPoint {
 // Needs assignment operator and copy constructor, for pointer members.
 public:
   arDataClient(const string& exeName="");
   ~arDataClient();

   bool dialUp(const char*, int);   // connect to IP address / port
   bool dialUpFallThrough(const char*, int);
   bool dialUp(const string& s, int i)
     { return dialUp(s.c_str(), i); }
   bool dialUpFallThrough(const string& s, int i)
     { return dialUpFallThrough(s.c_str(), i); }
   void closeConnection();
   bool getData(ARchar*&, int&); // 1st arg will be grown to fit, if needed.
   bool getDataQueue(ARchar*&, int&); // 1st arg grown to fit, if needed.
   arTemplateDictionary* getDictionary();

   bool sendData(arStructuredData*);

   arStreamConfig getRemoteStreamConfig() const
     { return _remoteStreamConfig; }
   arSocket* getSocket() const
     { return _socket; }
   int getSocketIDRemote() const
     { return _socketIDRemote; }
   const string& getLabel() const
     { return _exeName; }
   void setLabel(const string& exeName);

 private:
   arTemplateDictionary* _theDictionary;
   arSocket* _socket;
   string _exeName; // for diagnostics

   int _socketIDRemote;
   arStreamConfig _remoteStreamConfig;

   bool _activeConnection;
   arLock _lockSend;

   bool _dialUpInit(const char*, int);
   bool _dialUpConnect(const char*, int);
   bool _dialUpActivate();
   bool _translateID(ARchar* buf, ARchar* dest, int& size);
};

#endif
