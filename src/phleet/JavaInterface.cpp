//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arSocket.h"
#include "arUDPSocket.h"
#include "arThread.h"
#include "arSZGClient.h"

arSZGClient* SZGClient = NULL;
bool connected = false;
arSocket listeningSocket(AR_LISTENING_SOCKET);
arSocket* dataSocket = NULL;
char tagBuffer[1024];

enum {
  JAVA_REQ_PROCESS_TABLE=1,
  JAVA_RECV_PROCESS_TABLE,
  JAVA_KILL,
  JAVA_REQ_EXEC_TABLE,
  JAVA_RECV_EXEC_TABLE,
  JAVA_EXEC 
  };

void connectionTask(void*){
  arSocketAddress incomingAddress;
  arSocketAddress fromAddress;
  const int incomingPort = 4622;
  incomingAddress.setAddress(NULL,incomingPort);
  arUDPSocket socket;
  socket.ar_create();
  if (socket.ar_bind(&incomingAddress) < 0){
    cerr << "JavaInterface error: failed to bind to INADDR_ANY:"
         << incomingPort << endl;
    return;
  }
  char buffer[256];

  while (true) {
    if (connected) {
      ar_usleep(100000);
      continue;
    }

    if (dataSocket) {
      dataSocket->ar_close();
      delete dataSocket;
    }
    dataSocket = new arSocket(AR_STANDARD_SOCKET);
    dataSocket->ar_create();
    // first, receive discovery packet
    do {
      cout << "About to get discovery packet\n";
      socket.ar_read(buffer,256,&fromAddress);
      cout << "Service requested = " << buffer << " " << buffer+128 << "\n";
    }
    while (strcmp(tagBuffer,buffer+128));
    // tags match.
    // next, send the packet telling the client where to connect
    memset(buffer, 0, 16);
    sprintf(buffer,"%i",9999);
    socket.ar_write(buffer,16,&fromAddress);

    // next, open the TCP connection
    listeningSocket.ar_accept(dataSocket);
    //ar_usleep(1000000);
    connected = true;
    cout << "JavaInterface: got a connection\n";
  }
}

void messageTask(void*){
  string messageType, messageBody;
  while (1){
    SZGClient->receiveMessage(&messageType, &messageBody);
    if (messageType=="quit"){
      exit(0);
    }
  }
}

void swapBytes(char* buf){
  char buf2[4];
  buf2[0] = buf[0]; buf2[1] = buf[1]; buf2[2] = buf[2]; buf2[3] = buf[3];
  buf[0] = buf2[3]; buf[1] = buf2[2]; buf[2] = buf2[1]; buf[3] = buf2[0];
}

string createExecString(const string& computer, const string& stuff){
    string result;
    unsigned place = 0;
    char token[256];
    int tokenPlace = 0;
    while (place < stuff.length()){
	while (stuff[place] != '/' && place < stuff.length()){
	    token[tokenPlace++] = stuff[place++];
	}
	place++;
        token[tokenPlace] = '\0';
        result += computer+'/'+token+"/0:";
        tokenPlace = 0;
    }
    return result; 
}

string createExecTable(){
    string execTable;
    string processTable = SZGClient->getProcessList();
    cout << processTable << "\n";
    string activeComputer[50];
    int whichComputer = 0;
    
    // process the list to determine who's running an szgd
    unsigned place = 0;
    char computer[256];
    int computerPlace = 0;
    char process[256];
    int processPlace = 0;
    while (place<processTable.length()){
	// find a computer name
        while (processTable[place] != '/'){
	    computer[computerPlace++] = processTable[place++];
	}
	place++;
        // find the process name
        while (processTable[place] != '/'){
	    process[processPlace++] = processTable[place++];
	}
        // skip to the next beginning
        while (processTable[place] != ':' && place<processTable.length()){
	    place++;
	}
        place++;
        computer[computerPlace] = '\0';
        process[processPlace] = '\0';
	if (!strcmp("szgd",process)){
	    bool flag = false;
            for (int i=0; i<whichComputer; i++){
		// make sure no duplicate szgd
                if (activeComputer[i]==computer){
		    flag = true;
		    break;
		}
	    }
	    if (!flag){
		// OK to add
                activeComputer[whichComputer] = computer;
                string shortcut(SZGClient->getAttribute(
		  activeComputer[whichComputer], "SZG_EXEC", "shortcut", ""));
                cout << "JavaInterface: found szgd on "
		     << activeComputer[whichComputer] << "\n\t"
		     << shortcut << "\n";
                execTable += createExecString(activeComputer[whichComputer],
                                              shortcut);
                whichComputer++;
	    }
	}
        computerPlace = 0;
        processPlace = 0;
    }
    return execTable;
}

int main(int argc, char** argv){
  if (argc != 2){
    cerr << "usage: JavaInterface service_tag\n"
         << "\t(service_tag = SZG_INPUT, SZG_TRACKER, SZG_JOYSTICK, etc.)\n";
    return 1;
  }
  SZGClient = new arSZGClient;
  SZGClient->init(argc, argv);
  if (!*SZGClient)
    return 1;

  arThread dummy1(messageTask);
  listeningSocket.ar_create();
  const int listeningPort = 9999;
  if (listeningSocket.ar_bind(NULL,listeningPort) < 0){
    cerr << "JavaInterface error: failed to bind to INADDR_ANY on port " 
         << listeningPort << ".\n";
    return 1;
  }
  listeningSocket.ar_listen(5);

  dataSocket = NULL;
  strcpy(tagBuffer,argv[1]);
  arThread dummy2(connectionTask);

  const int bufferSize = 10000;
  char buffer[bufferSize];

  // now, start the actual communications
  while (true) {
    if (!connected){
      ar_usleep(100000);
      continue;
    }

    cout << "JavaInterface trying to read data.\n";
    bool ok = dataSocket->ar_read(buffer,4);
    if (!ok){
      cout << "JavaInterface error: read failed.\n";
    }
    else{
      cout << "JavaInterface read 4 bytes.\n";
      //swapBytes(buffer);
      int code = *(int*)buffer;
      
      cout << "JavaInterface:  code = " << code << "\n";
      // needed for the message send/ receives.
      int match;
      switch (code)
	{
      case JAVA_REQ_PROCESS_TABLE:
	{
	cout << "JavaInterface got process list request.\n";
	string send = SZGClient->getProcessList();
	*(int*)buffer = JAVA_RECV_PROCESS_TABLE;
	swapBytes(buffer);
	*(int*)(buffer+4) = send.length();
	swapBytes(buffer+4);
	strncpy(buffer+8,send.data(),send.length());
	ok = dataSocket->ar_write(buffer,send.length()+8);
	break;
	}
      case JAVA_KILL:
	{
	cout << "JavaInterface got kill request.\n";
	ok = dataSocket->ar_read(buffer,4);
	if (ok){
	  const int killID = *(int*)buffer;
	  cout << "JavaInterface going to kill " << killID << "\n";
	  match = SZGClient->sendMessage("quit","0",killID);
	  cout << "JavaInterface sent kill message.\n";
	}
	else{
	  cout << "JavaInterface failed to read ID.\n";
	}
	break;
	}
      case JAVA_REQ_EXEC_TABLE:
	{
	cout << "JavaInterface got exec table request.\n";
	string send = createExecTable();
	*(int*)buffer = JAVA_RECV_EXEC_TABLE;
	swapBytes(buffer);
	*(int*)(buffer+4) = send.length();
	swapBytes(buffer+4);
	strncpy(buffer+8,send.data(),send.length());
	ok = dataSocket->ar_write(buffer,send.length()+8);
	break;
	}
      case JAVA_EXEC:
	{
	cout << "JavaInterface got exec request.\n";
	ok = dataSocket->ar_read(buffer,4);
	if (ok){
	  const int execLength = *(int*)buffer;
	  ok = dataSocket->ar_read(buffer,execLength);
	  if (ok){
	    buffer[execLength] = '\0';
	    int loc=0;
	    while (buffer[loc] != '/')
	      ++loc;
	    const string computer(buffer, loc);
	    const string process(buffer, loc+1, execLength);
	    cout << computer << " " << process << "**\n";
	    const int execDaemonID = SZGClient->getProcessID(computer, "szgd");
	    // remember that we expect to get a response here
	    match = SZGClient->sendMessage("exec",process,execDaemonID,true);
	    if (match < 0){
              cerr << "JavaInterface warning: failed to send exec message.\n";
	    }
            else{
              // get the various responses (and print them out)
              string body;
	      list<int> tags;
	      tags.push_back(match);
              while (SZGClient->getMessageResponse(tags, body, match) < 0){
		// this is not the final response
                cout << body << "\n";
              }
              // output the last (final) response
            cout << body << "\n"; 
	    }
	  }
	}
	break;
	}
      default:
	cout << "JavaInterface got unknown ID " << code << ".\n";
	break;
      }
      if (!ok)
	connected = false;
    }
    if (!ok) {
      cerr << "JavaInterface warning: disconnected.\n";
      connected = false;
    }
  }
}
