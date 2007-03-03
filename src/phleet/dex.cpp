//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arSZGClient.h"

#include <stdio.h>

const int chost = 60;
const int cchMax = 30;
static char hosts[chost * cchMax + 2];

void getHostsRunningSzgd(arSZGClient& szgClient){
  
  const string& lines = szgClient.getProcessList();
  strcpy(hosts, " ");

  // todo: copypasted from dtop.cpp
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
    // todo: include both stripped and unstripped ones in list?
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
  return strstr(hosts, buf) != NULL;
}

// todo: make this a member of szgClient, and use it wherever SZG_CONF/virtual is mentioned.
bool isVirtualComputer(arSZGClient& szgClient, const char* host){
  return szgClient.getAttribute(host, "SZG_CONF", "virtual", "") == "true";
}

int main(int argc, char** argv){
  int i=0, j=0;
  // Default is no timeouts.
  int msecTimeoutLocal = -1;
  int msecTimeoutRemote = -1;

  float t;
  string sz = ar_getenv("SZG_DEX_LOCALTIMEOUT");
  if (sz != "NULL" && ar_stringToFloatValid( sz, t ) && t>0.)
    msecTimeoutLocal = int(t*1000);
  sz = ar_getenv("SZG_DEX_TIMEOUT");
  if (sz != "NULL" && ar_stringToFloatValid( sz, t ) && t>0.)
    msecTimeoutRemote = int(t*1000);

  // parse and remove the command-line options
  bool verbosity = false;

  for (i=0; i<argc; i++){
    if (!strcmp(argv[i],"-v")){
      verbosity = true;
      // remove the arg from the list
      for (j=i; j<argc-1; j++)
        argv[j] = argv[j+1];
      argc--;
    }

    // factor out copypaste from these two blocks.

    if (!strcmp(argv[i],"-lt")){
      // The next arg must be a timeout, in seconds.
      if (argc <= i+1){
        cerr << "dex error: a timeout in seconds must follow the -lt option.\n";
        return 1;
      }
      if (!ar_stringToFloatValid( string(argv[i+1]), t )) {
        cerr << "dex error: the local timeout must be a number.\n";
        return 1;
      }
      if (t <= 0.){
        cerr << "dex error: the local timeout must be a number of seconds > 0.\n";
        return 1;
      } else {
        msecTimeoutLocal = int(t*1000);
        cout << "dex remark: local timeout = " << msecTimeoutLocal << " milliseconds.\n";
      }
      for (j=i; j<argc-2; j++)
        argv[j] = argv[j+2];
      argc -= 2;
    }

    if (!strcmp(argv[i],"-t")){
      // The next arg must be a remote timeout, in seconds.
      if (argc <= i+1){
        cerr << "dex error: a timeout in seconds must follow the -t option.\n";
        return 1;
      }
      if (!ar_stringToFloatValid( string(argv[i+1]), t )) {
        cerr << "dex error: the timeout must be a number.\n";
        return 1;
      }
      if (t <= 0.){
        cerr << "dex error: the timeout must be a number of seconds > 0.\n";
        return 1;
      } else {
        msecTimeoutRemote = int(t*1000);
        cout << "dex remark: remote timeout = " << msecTimeoutRemote << " milliseconds.\n";
      }
      for (j=i; j<argc-2; j++)
        argv[j] = argv[j+2];
      argc -= 2;
    }

  }

  if (argc <= 1) {
    cerr << "usage: dex [-v] [-lt localtimeoutsec] [-t timeoutsec] executable_name\n"
         << "  dex [-v] [-lt localtimeoutsec] [-t timeoutsec] hostname executable_name [args]\n"
         << "  localtimeoutsec is how long dex waits for a reply.\n"
         << "  timeoutsec is how long szgd gives the app to launch before aborting it.\n";
    return 1;
    }

  arSZGClient szgClient;
  // dex forwards rather than parses the "special" phleet args, so this works:
  //     dex smoke szgrender -szg networks/graphics=wall
  // But dex DOES parse phleet args that relate to user login, so this works:
  //     dex smoke szgrender -szg user=ben -szg server=192.168.0.1:9999
  szgClient.parseSpecialPhleetArgs(false);
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  getHostsRunningSzgd(szgClient);
  const string localhost(szgClient.getComputerName());
  bool runningOnVirtual = false;
  string hostName;
  string exeName;

  // dex is complicated because it can be used in several ways:
  //  a. dex virtual_computer arg0 arg1 arg2
  //  b. dex actual_computer arg0 arg1 arg2
  //  c. dex arg0 arg1 arg2
  // Case (c) executing via the szgd on the local computer.
  // This creates some complications. For instance, if an executable has the
  // same name as a computer or virtual computer then it cannot be executed
  // in this way. However, this is a desirable shorthand for a common case
  // and *does* work in the most cases that arise in practice.
  // NOTE: the fault tree is essentially:
  //  a. Is argv[1] a virtual computer name? If so, interpret command that way.
  //  b. Is argv[1] a computer running szgd? If so, interpret command that way.
  //  c. Otherwise, interpret command as (c) above.

  // This block of code determines which of the 3 cases,
  // and packs the args into a string to be sent to szgd.
  if (argc == 2) {
    // Execute on the local machine.
    hostName = localhost;
    exeName = argv[1];
  } else {
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
        // Don't send an extra space to szgd
        if (i != argc-1) {
          exeName.append(" ");
        }
      }
    } else {
      cerr << argv[0] << " error: no virtual computer '" << argv[1]
           << "', and no szgd on host '" << argv[1] << "'.\n";
      const string s = szgClient.getVirtualComputers();
      if (s.empty())
        cout << "  (No virtual computers defined.  Did you run dbatch?)\n";
      else
        cout << "  (Known virtual computers are: " << s << ".)\n";
      return 1;
    }
  }

  string messageContext("NULL");
  if (runningOnVirtual) {
    const string trigger(szgClient.getTrigger(hostName));
    if (trigger == "NULL") {
      cerr << argv[0] << " error: no trigger for virtual computer '" << hostName << "'.\n";
      return 1;
    }
    messageContext = szgClient.createContext(
      hostName, "default", "trigger", "default", "NULL");
    hostName = trigger;
  }

  // hostName names an actual computer,
  // either from the command line OR the trigger of the virtual computer.
  const int szgdID = szgClient.getProcessID(hostName, "szgd");
  if (szgdID == -1) {
    cerr << argv[0] << " error: no szgd on host " << hostName << ".\n";
    if (runningOnVirtual){
      cerr << "  (which is the trigger of virtual computer '" << argv[1] << "')\n";
    }
    // Don't reinterpret or retry.  Just fail.
    return 1;
  }

  string messageBody( exeName );
  if (msecTimeoutRemote > -1) {
    ostringstream tempStream;
    tempStream << msecTimeoutRemote;
    // Hack. This will get unpacked by szgd.
    messageBody += "||||"+tempStream.str();
  } 
  int match = szgClient.sendMessage("exec", messageBody, messageContext, szgdID, true);
  if (match < 0) {
    cerr << "dex error: failed to send message.\n";
    return 1;
  }
  list<int> tags;
  tags.push_back(match);

  // Default body, e.g. for timeouts.
  string body("dex error: got no response.");

  // We only get a message response with "match" from the tags list.
  // The variable match is filled-in with the "match" we received, which
  // is redundant in this case since there's just one thing in the list.
  while (szgClient.getMessageResponse(tags,body,match,msecTimeoutLocal) < 0) {
    if (verbosity){
      cout << body << "\n";
    } else {
      // Print lines beginning with "szg:".
      vector< string > lines;
      string line;
      istringstream ist;
      ist.str( body );
      while (getline( ist, line, '\n' )) {
        lines.push_back( line );
      }
      vector< string >::const_iterator iter;
      for (iter = lines.begin(); iter != lines.end(); ++iter) {
        if (iter->find( "szg:", 0 ) == 0) {
          cout << *iter << endl;
        }
      }
    }
  }
  // Print the final response.
  cout << body << "\n";
  return 0;
}
