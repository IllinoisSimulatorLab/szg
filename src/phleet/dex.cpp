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
  float t = -1.;
  string sz(ar_getenv("SZG_DEX_LOCALTIMEOUT"));
  if (sz != "NULL" && ar_stringToFloatValid( sz, t ) && t > 0.)
    msecTimeoutLocal = int(t*1000);
  sz = ar_getenv("SZG_DEX_TIMEOUT");
  if (sz != "NULL" && ar_stringToFloatValid( sz, t ) && t > 0.)
    msecTimeoutRemote = int(t*1000);

  // parse and remove the command-line options
  int iVerbose = 1;

  for (i=0; i<argc; i++){

    if (!strcmp(argv[i],"-q")){
      iVerbose = 0;
      // remove the arg from the list
      for (j=i; j<argc-1; j++)
        argv[j] = argv[j+1];
      argc--;
    }

    if (!strcmp(argv[i],"-v")){
      iVerbose = 2;
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
        cerr << "dex error: local timeout must be a number.\n";
        return 1;
      }
      if (t <= 0.){
        cerr << "dex error: local timeout must be a number of seconds > 0.\n";
        return 1;
      }
      msecTimeoutLocal = int(t*1000);
      cout << "dex remark: local timeout = " << msecTimeoutLocal << " milliseconds.\n";
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
        cerr << "dex error: timeout must be a number.\n";
        return 1;
      }
      if (t <= 0.){
        cerr << "dex error: timeout must be a number of seconds > 0.\n";
        return 1;
      }
      msecTimeoutRemote = int(t*1000);
      cout << "dex remark: remote timeout = " << msecTimeoutRemote << " milliseconds.\n";
      for (j=i; j<argc-2; j++)
        argv[j] = argv[j+2];
      argc -= 2;
    }

  }

  if (argc <= 1) {
    cerr << "usage: dex [-vq] [-lt localtimeoutsec] [-t timeoutsec] exe\n"
         << "       dex [-vq] [-lt localtimeoutsec] [-t timeoutsec] host exe [args]\n"
         << "  localtimeoutsec is how long dex waits for a reply.\n"
         << "  timeoutsec is how long szgd waits before aborting the exe's launch.\n";
    // -v is verbose (include context), -q is quiet (exclude ar_log's).
    return 1;
    }

  arSZGClient szgClient;
  // dex forwards rather than parses the "special" Syzygy args, so this works:
  //     dex smoke szgrender -szg networks/graphics=wall
  // But dex DOES parse Syzygy args that relate to user login, so this works:
  //     dex smoke szgrender -szg user=ben -szg server=192.168.0.1:9999
  szgClient.parseSpecialPhleetArgs(false);
  const bool fInit = szgClient.init(argc, argv);
  if (!szgClient)
    return szgClient.failStandalone(fInit);

  getHostsRunningSzgd(szgClient);
  const string localhost(szgClient.getComputerName());
  bool fVirtual = false;
  string hostName;
  string exeName;

  // dex can run in several ways:
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

  if (argc == 2) {
    // Execute on the local machine.
    hostName = localhost;
    exeName = argv[1];
  } else {
    fVirtual = isVirtualComputer(szgClient, argv[1]);
    if (fVirtual || isRunningSzgd(argv[1])) {
      // argv[1] is an actual or virtual computer.
      // Pack the args thus: Run argv[2..] on argv[1].
      hostName = argv[1];
      for (i=2; i<argc; ++i) {
        exeName.append(argv[i]);
        // Don't send an extra space to szgd
        if (i != argc-1) {
          exeName.append(" ");
        }
      }
    } else {
      const string s = szgClient.getVirtualComputers();
      ar_log_critical() << "no virtual computer '" << argv[1] <<
        "', and no szgd on host '" << argv[1] << "'.\n" <<
        (s.empty() ?
	  "  (No virtual computers defined.  Did you run dbatch?)\n" :
	  "  (Known virtual computers are: " + s + ".)\n" );
      return 1;
    }
  }

  string msgContext("NULL");
  if (fVirtual) {
    const string trigger(szgClient.getTrigger(hostName));
    if (trigger == "NULL") {
      ar_log_critical() << "no trigger for virtual computer '" << hostName << "'.\n";
      return 1;
    }
    msgContext = szgClient.createContext(hostName, "default", "trigger", "default", "NULL");
    hostName = trigger;
  }

  // hostName names an actual computer,
  // either from the command line OR the trigger of the virtual computer.
  const int szgdID = szgClient.getProcessID(hostName, "szgd");
  if (szgdID == -1) {
    ar_log_critical() << "no szgd on host " << hostName << ".\n";
    if (fVirtual) {
      ar_log_critical() << "  (That host is the trigger of virtual computer '" << argv[1] << "')\n";
    }
    // Don't reinterpret or retry.  Just fail.
    return 1;
  }

  string msgBody( exeName );
  if (msecTimeoutRemote > -1) {
    // Hack, unpacked by szgd.
    msgBody += "||||" + ar_intToString(msecTimeoutRemote);
  } 
  int match = szgClient.sendMessage("exec", msgBody, msgContext, szgdID, true);
  if (match < 0) {
    cerr << "dex error: failed to send message.\n";
    return 1;
  }

  // Default body, e.g. for timeouts.
  string body("dex error: no response.");

  // We only get a message response with "match" from the one-element list.
  for (;;) {
    const int r = szgClient.getMessageResponse(list<int>(1, match), body, match, msecTimeoutLocal);
    switch (iVerbose) {
    default:
      break;
    case 1: {
	// Remove lines of context (prefixed with "  |") from body.
	vector< string > lines;
	string line;
	istringstream ist;
	ist.str( body );
	body = "";
	while (getline( ist, line, '\n' ))
	  lines.push_back( line );
	vector< string >::const_iterator i;
	for (i = lines.begin(); i != lines.end(); ++i)
	  if (i->find( "  |", 0 ) != 0)
	    body += *i + "\n";
	// fallthrough
      }
    case 2:
      cout << body; // e.g., "cubevars launched.\n"
      break;
    }

    if (r >= 0)
      break;

    if (iVerbose > 1) {
      // Delimit successive bodies.
      cout << "\n";
    }
  }
  return 0;
}
