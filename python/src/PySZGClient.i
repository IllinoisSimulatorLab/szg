//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation (http://www.gnu.org/copyleft/gpl.html).

// The arSZGClient is the basic communications object. In a normal,
// framework-based application, the only method youll probably need
// is getAttribute() for reading parameters from the Syzygy database.
//
// Example: to get the position of screen 0 
//
// Example: youre running a master/slave application and you need to send a
// message to the master from a slave. You do that with the arAppLauncher
// and the arSZGClient, as shown here (fw is an arMasterSlaveFramework).
//
//  cl = fw.getSZGClient()
//  al = fw.getAppLauncher()
//  mn = al.getMasterPipeNumber()
//  rp = al.getRenderProgram( mn )
//  if rp == 'NULL':
//    return
//  rpl = rp.split('/')
//  id = cl.getProcessID(rpl[0],rpl[1])
//  m = cl.sendMessage( 'user', 'connected', id, False )

// ******************** based on arSZGClient.h ********************

class arSZGClient{
 public:
  arSZGClient();
  ~arSZGClient();

  void simpleHandshaking(bool state);
  void parseSpecialPhleetArgs(bool state);
  bool init(int&, char** argv, const string& forcedName = string("NULL"));
  stringstream& initResponse(){ return _initResponseStream; }
  bool sendInitResponse(bool state);
  stringstream& startResponse(){ return _startResponseStream; }
  bool sendStartResponse(bool state);
  
  int  getServiceComponentID(const string& serviceName);

  // functions that aid operation on a virtual computer
  arSlashString getNetworks(const string& channel);
  arSlashString getAddresses(const string& channel);
  const string& getVirtualComputer();
  const string& getMode(const string& channel);
  string getTrigger(const string& virtualComputer);

  // Here are the functions for dealing with the parameter database
  // An abbreviation for the common case computerName=="NULL".
  bool setAttribute(const string& groupName, 
                    const string& parameterName,
                    const string& parameterValue);
%extend{
  bool setAttributeComputer(const string& computerName,
                    const string& groupName, 
                    const string& parameterName,
                    const string& parameterValue) {
      return self->setAttribute( computerName, groupName, parameterName, parameterValue );
      }
  bool setAttributeUser(const string& userName,
                    const string& computerName,
                    const string& groupName, 
                    const string& parameterName,
                    const string& parameterValue)  {
      return self->setAttribute( userName, computerName, groupName, parameterName, parameterValue );
      }
}
  string testSetAttribute(const string& computerName,
              const string& groupName,
              const string& parameterName,
              const string& parameterValue);
%extend{
  string testSetAttributeUser(const string& userName,
                          const string& computerName,
              const string& groupName,
              const string& parameterName,
              const string& parameterValue) {
    return self->testSetAttribute( userName, computerName, groupName, parameterName, parameterValue );
    }
}
  // An abbreviation for the common case computerName=="NULL".
  string getAttribute(const string& groupName, 
                      const string& parameterName,
              const string& validValues = "");
%extend{
  string getAttributeUser(const string& userName,
              const string& computerName,
              const string& groupName,
              const string& parameterName,
              const string& validValues) {
    return self->getAttribute( userName, computerName, groupName, parameterName, validValues );
    }
  string getAttributeComputer(const string& computerName,
                      const string& groupName, 
                      const string& parameterName,
              const string& validValues /* no default */) {
    return self->getAttribute( computerName, groupName, parameterName, validValues );
    }
  arVector3 getAttributeVector( const string& groupName,
                                const string& parameterName ) {
    arVector3 result;
    if (!self->getAttributeVector3( groupName, parameterName, result )) {
        PyErr_SetString(PyExc_RuntimeError,"arSZGClient error: getAttributeVector3() failed.\n");
        return false;
    }
    return result;
    }
}
  // More abbreviations.
  int getAttributeInt(const string& groupName, 
                      const string& parameterName);
//  int getAttributeInt(const string&, const string&, const string&,
//              const string&);
  bool getAttributeFloats(const string& groupName,
                  const string& parameterName,
                          float* values,
                          int numvalues = 1);
  bool getAttributeInts(const string& groupName,
                const string& parameterName,
                        int* values,
                        int numvalues = 1);
  bool getAttributeLongs(const string& groupName,
                 const string& parameterName,
                         long* values,
                         int numvalues = 1);
  string getAllAttributes(const string& substring);

  // A way to get parameters in from a file (as in dbatch, for instance)
  bool parseParameterFile(const string& fileName);

  const string& getLabel() const;

  const string& getComputerName() const;

  const string& getUserName() const;

  // general administration functions
  string getProcessList();
  bool killProcessID(const string& computer, const string& processLabel);
  string getProcessLabel(int processID);
  int getProcessID(const string& computer, const string& processLabel);
  int getProcessID(void); // get my own process ID
  int sendMessage(const string& type, const string& body, int destination,
                   bool responseRequested = false);

};  

