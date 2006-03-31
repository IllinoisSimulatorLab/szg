//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arSZGClient.h"
#include <stdio.h>

const int chost = 60;
const int cchMax = 30;
static char hosts[chost * cchMax];

void getHostsRunningSzgd(arSZGClient& szgClient){
  
  const string& lines = szgClient.getProcessList();

  /// \todo copypasted from dtop.cpp
  for (unsigned iline=0; iline < lines.length(); ++iline) {
    // Stuff buf[] with the next line.
    int ich = 0;
    char buf[256];
    while (lines[iline] != ':' && iline < lines.length())
      buf[ich++] = lines[iline++];
    buf[ich] = '\0';

    // Parse the components of buf[].
    char host[256] = " ";
    char task[256];
    char id[256];
    sscanf(buf, "%[^/]/%[^/]/%s", host+1, task, id);

    // strip domain name from host
    /// \todo include both stripped and unstripped ones in list?
    char* pch = strchr(host, '.');
    if (pch)
      *pch = '\0';

    if (!strcmp(host, "NULL"))
      continue;

    if (!strcmp(task, "szgd")) {
      strcat(host, " ");
      // host has the form " hostname "
      strcat(hosts, host);
    }
  }
}

bool isRunningSzgd(const char* szHost){
  if (!szHost || !*szHost)
    return false;
  char buf[256];
  sprintf(buf, " %s ", szHost);
  return strstr(hosts, szHost) != NULL;
}

/// \todo make this a member of szgClient, and use it wherever SZG_CONF/virtual is mentioned.
bool isVirtualComputer(arSZGClient& szgClient, const char* host){
  return szgClient.getAttribute(host, "SZG_CONF", "virtual", "") == "true";
}

int main(int argc, char** argv){
  int i=0, j=0;
  // Default is to have no timeout.
  int localtimeout = -1;
  int remoteTimeout = -1;
  float tempFloat;
  bool stat;
  bool verbosity = false;
  // parse and remove the command-line options
  for (i=0; i<argc; i++){
    if (!strcmp(argv[i],"-v")){
      verbosity = true;
      // remove the arg from the list
      for (j=i; j<argc-1; j++){
        argv[j] = argv[j+1];
      }
      argc--;
    }
    if (!strcmp(argv[i],"-lt")){
      // The next arg must be a timeout, in seconds.
      if (argc <= i+1){
	cerr << "dex error: a timeout in seconds must follow the -lt option.\n";
	return 1;
      }
      stat = ar_stringToFloatValid( string(argv[i+1]), tempFloat );
      if (!stat) {
	cerr << "dex error: the local timeout must be a number.\n";
        return 1;
      }
      if (tempFloat <= 0.){
	cerr << "dex error: the local timeout must be a number of seconds > 0.\n";
	return 1;
      } else {
        // convert to int, milliseconds.
        localtimeout = (int)(tempFloat*1000);
        cout << "dex remark: local timeout = " << localtimeout << " milliseconds.\n";
      }
      for (j=i; j<argc-2; j++){
        argv[j] = argv[j+2];
      }
      argc = argc - 2;
    }
    if (!strcmp(argv[i],"-t")){
      // The next arg must be a remote timeout, in seconds.
      if (argc <= i+1){
	cerr << "dex error: a timeout in seconds must follow the -t option.\n";
	return 1;
      }
      stat = ar_stringToFloatValid( string(argv[i+1]), tempFloat );
      if (!stat) {
	cerr << "dex error: the timeout must be a number.\n";
        return 1;
      }
      if (tempFloat <= 0.){
	cerr << "dex error: the timeout must be a number of seconds > 0.\n";
	return 1;
      } else {
        // convert to int, milliseconds.
        remoteTimeout = (int)(tempFloat*1000);
        cout << "dex remark: remote timeout = " << remoteTimeout << " milliseconds.\n";
      }
      for (j=i; j<argc-2; j++){
        argv[j] = argv[j+2];
      }
      argc = argc - 2;
    }
  }

  if (argc <= 1) {
    // don't even try connecting to szgserver
    cerr << "usage: " << argv[0] << " [-v] [-lt localtimeoutsec] [-t timeoutsec] executable_name\n"
         << "       " << argv[0] << " [-v] [-lt localtimeoutsec] [-t timeoutsec] hostname executable_name [args]\n"
         << "       localtimeoutsec is the time for dex to wait for a reply.\n"
         << "       timeoutsec is the timeout for the actual launch of the application by szgd.\n"
         << "       If the app hasn't launched in timeoutsec, it will abort.\n";
    return 1;
    }

  arSZGClient szgClient;
  // It is very important that dex DOES NOT parse the "specical" phleet args,
  // but instead passes them along to the remotely executed program
  // because commands like:
  // dex smoke szgrender -szg networks/graphics=wall
  // should work.
  // NOTE: however, the phleet args that relate to user login MUST be
  // parsed. This allows commands like:
  // dex smoke szgrender -szg user=ben -szg server=192.168.0.1:9999
  // to work. 
  szgClient.parseSpecialPhleetArgs(false);
  szgClient.init(argc, argv);
  if (!szgClient)
    return 1;
  // This does a "dps" and finds the hosts that are running szgd.
  getHostsRunningSzgd(szgClient);

  string hostName;
  string exeName;

  const string localhost(szgClient.getComputerName());
  bool runningOnVirtual = false;

  // There is a somewhat complex dance to dex because it can be used it
  // several ways:
  //  a. dex virtual_computer arg0 arg1 arg2
  //  b. dex actual_computer arg0 arg1 arg2
  //  c. dex arg0 arg1 arg2
  // In the final case, we are executing via the szgd on the local computer.
  // This creates some complications. For instance, if an executable has the
  // same name as a computer or virtual computer then it cannot be executed
  // in this way. However, this is a desirable shorthand for a common case
  // and *does* work in the most cases that arise in practice.
  // NOTE: the fault tree is essentially:
  //  a. Is argv[1] a virtual computer name? If so, interpret command that
  //     way.
  //  b. Is argv[1] a computer running szgd? If so, interpret command that
  //     way.
  //  c. Otherwise, interpret command as (c) above.

  // The next block of code determines the manner in which we are executing.
  // It also packs the args into a string to be sent to szgd.
  if (argc == 2) {
    // In this case, we MUST be attempting to execute on the local machine.
    hostName = localhost;
    exeName = argv[1];
  }
  else {
    // The over-riding consideration is:
    //  "Are we running on a virtual computer?"
    runningOnVirtual = isVirtualComputer(szgClient, argv[1]);
    // If argv[1] refers to an actual or virtual computer,
    // pack the args like so... (same either way)
    if (runningOnVirtual || isRunningSzgd(argv[1])) {
      // Run argv[2..] on argv[1].
      hostName = argv[1];
      for (i=2; i<argc; ++i) {
	exeName.append(argv[i]);
	// Important we do not send an extra space to the szgd
        if (i != argc-1){
	  exeName.append(" ");
	}
      }
    }
    else {
      // Otherwise, pack the args like so...
      if (isRunningSzgd(localhost.c_str())) {
	cout << argv[0] << " remark: interpreting " << argv[1]
	     << " as executable name.\n";
        // Run argv[1..] on localhost.
	hostName = localhost;
	for (i=1; i<argc; ++i) {
	  exeName.append(argv[i]);
	  // Don't send an extra space to szgd
	  if (i != argc-1){
	    exeName.append(" ");
	  }
	}
      }
      else {
	cerr << argv[0] << " error: no virtual computer '" << argv[1]
	     << "', and no szgd on host '" << argv[1]
	     << "' or on local host '" << localhost << "'.\n";
	return 1;
      }
    }
  }

  // getTrigger(...) returns the trigger of the virtual computer
  // passed-in as an arg, if such is fact a virtual computer,
  // and otherwise returns "NULL". THIS IS ONLY MEANINGFUL IF WE ARE
  // TRYING TO EXECUTE ON A VIRTUAL COMPUTER.
  string messageContext("NULL");
  if (runningOnVirtual){
    string virtualComputerTrigger(szgClient.getTrigger(hostName));
    if (virtualComputerTrigger != "NULL"){
      messageContext = szgClient.createContext(hostName,"default","trigger",
                                               "default","NULL");
      hostName = virtualComputerTrigger;
    }
    else{
      cerr << argv[0] << " error: virtual computer '" << hostName << "' has no trigger.\n";
      // Fatal error.
      return 1;
    }
  }

  // By this point, hostName is the name of an actual computer, either from
  // the command line OR the trigger of the virtual computer.
  const int szgdID = szgClient.getProcessID(hostName, "szgd");
  if (szgdID == -1) {
    cerr << argv[0] << " error: found no szgd on computer=" 
         << hostName << ".\n";
    if (runningOnVirtual){
      cerr << "  (which is the trigger of virtual computer " 
           << argv[1] << ")\n";
    }
    // Don't reinterpret or retry.  Just fail.
    return 1;
  }

  string messageBody( exeName );
  if (remoteTimeout > -1) {
    ostringstream tempStream;
    tempStream << remoteTimeout;
    // Oy vey. Hack. This will get unpacked by szgd.
    messageBody += "||||"+tempStream.str();
  } 
  int match = szgClient.sendMessage("exec", 
                                    messageBody, 
                                    messageContext, 
                                    szgdID, true);
  if (match < 0){
    cout << "dex error: an error has occured in sending the message.\n";
    return 1;
  }
  list<int> tags;
  tags.push_back(match);
  // We need a reasonable default (what if a timeout occurs, for instance).
  string body("dex error: no response was received.");
  // We only get a message response with "match" from the tags list.
  // The variable match is filled-in with the "match" we received, which
  // is redundant in this case since there's just one thing in the list.
  while (szgClient.getMessageResponse(tags,body,match,localtimeout) < 0){
    if (verbosity){
      cout << body << "\n";
    } else {
      // Find lines beginning with "szg:" and print those.
      std::vector< std::string > lines;
      std::string line;
      istringstream ist;
      ist.str( body );
      while (getline( ist, line, '\n' )) {
        lines.push_back( line );
      }
      std::vector< std::string >::const_iterator iter;
      for (iter = lines.begin(); iter != lines.end(); ++iter) {
        if (iter->find( "szg:", 0 ) == 0) {
          cout << *iter << endl;
        }
      }
    }
  }
  // output the last (final) response
  cout << body << "\n";
  return 0;
}
