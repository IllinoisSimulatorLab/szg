//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arSocket.h"
#include "arUDPSocket.h"
#include "arThread.h"
#include "arSZGClient.h"
#include "arAppLauncher.h"

arSZGClient szgClient;
string vircompName;
string hostName;
arConditionVar executionVar;
arMutex        executeLock;
bool           commandExecutionRequest = false;
int            executionID;

char tagBuffer[1024]; /// \bug buffer overflow possible
char* interfacePacket = NULL;
int packetLength = -1;

const int numDemosMax = 100; // more than fits on the screen of an ipaq
int numDemos = 0;
string buttonName[numDemosMax];
string cmdName[numDemosMax];

int createInterfacePacket(){
  // first, figure out the packet length
  int length = 4; // number of demos
  int i;
  for (i=0; i<numDemos; i++){
    length += 4; // the length of the string
    length += buttonName[i].length();
  }
  if (interfacePacket != NULL){
    delete [] interfacePacket;
  }
  interfacePacket = new char[length];
  // fill in the memory
  *((int*)interfacePacket) = numDemos;
  int where = 4;
  for (i=0; i<numDemos; i++){
    *((int*)(interfacePacket+where)) = buttonName[i].length();
    where += 4;
    for (int j=0; j<buttonName[i].length(); j++){
      interfacePacket[where+j] = buttonName[i].c_str()[j];
    }
    where += buttonName[i].length();
  }
  return length;
}

// initializes the interface by reading a file. we also create a packet
// encapsulating this interface that can be sent to java interfaces
// that request it.
bool initialize(){
  // Read initialization file.
  const string dataPath(szgClient.getAttribute("SZG_DATA","path"));
  FILE* pf = ar_fileOpen("ipaq.txt", dataPath, "r");
  if (!pf) {
    cerr << "ButtonInterface error: failed to open ipaq.txt in SZG_DATA/path "
         << dataPath << ".\n";
    return false;
  }

  // File format is a list of lines 
  // ("button name" "command to execute via szgd").
  char szButton[1024]; // Buffer overflow possible.
  char szCommand[1024];
  int num = EOF;
  for (numDemos = 0; numDemos < numDemosMax; ++numDemos) {
    num = fscanf(pf, "(\"%[^\"]\" \"%[^\"]\") ", szButton, szCommand);
    if (num == EOF)
      break;
    if (num < 2) {
      cerr << "ButtonInterface warning: syntax error in file ipaq.txt.\n" 
           << num << endl;
      break;
      }
    buttonName[numDemos] = szButton;
    cmdName[numDemos] = szCommand;
  }
  if (numDemos == numDemosMax && num != EOF)
    cerr << "ButtonInterface warning: incompletely read file ipaq.txt.\n";
  ar_fileClose(pf);

  // make the interface packet!
  packetLength = createInterfacePacket();
  cout << "PacketSize = " << packetLength << "\n";
  return true;
}

// This actually executes the command, based on a button ID
void executeCommand(int buttonID){
  if (buttonID < 0 || buttonID > numDemos){
    cerr << "ButtonInterface warning: ignoring unknown ID " << buttonID
	 << " (expected 0 through "
	 << numDemos-1 << ").\n";
    return;
  }

  const string& name = cmdName[buttonID];
  cout << "ButtonInterface remark: running " << name << "\n";

  // NOTE: it is important to look up the ID of the szgd running on the
  // control computer every time. Otherwise, ButtonInterface needs to be
  // restarted every time that szgd is restarted

  const int szgdID = szgClient.getProcessID(hostName, "szgd");
  if (szgdID < 0) {
    cerr << "ButtonInterface error: no szgd on host " << hostName << ".\n";
    return;
  }

  // we expect responses from our messages
  string context = szgClient.createContext(vircompName,
                                           "default",
					   "trigger",
					   "default",
					   "NULL");
  int match = szgClient.sendMessage("exec", name, context,
                                    szgdID,true);
if (match < 0){
    cerr << "ButtonInterface warning: failed to send message to szgd on \""
         << vircompName << "/trigger.\n";
  }
  else{
    // get the various responses (and print them out)
    string body;
    int remoteMatch;
    list<int> tags;
    tags.push_back(match);
    while (szgClient.getMessageResponse(tags, body, remoteMatch) < 0){
      cout << body << "\n";
    }
   // output the last (final) response
   cout << body << "\n"; 
   }
}

// commands are processed asynchronously from UDP packets to avoid
// queueing effects
void commandTask(void*){
  while (true){
    ar_mutex_lock(&executeLock);
    while (!commandExecutionRequest){
      executionVar.wait(&executeLock);
    }
    ar_mutex_unlock(&executeLock);
    cout << "ButtonInterface remark: executing command with ID = "
         << "  " << executionID << "\n";
    executeCommand(executionID);
    ar_mutex_lock(&executeLock);
    commandExecutionRequest = false;
    ar_mutex_unlock(&executeLock);
  }
}

// This receives all UDP packets
void receiveTask(void*){
  arSocketAddress incomingAddress;
  arSocketAddress fromAddress;
  const int incomingPort = 4622;
  incomingAddress.setAddress(NULL,incomingPort);
  arUDPSocket socket;
  socket.ar_create();
  if (socket.ar_bind(&incomingAddress) < 0){
    cerr << "ButtonInterface error: failed to bind to "
         << incomingPort << endl;
    return;
  }
  char buffer[1024];

  while (true) {
    socket.ar_read(buffer,1024,&fromAddress);
    // guard against incorrect packets
    buffer[1023] = '\0';
    if (!strcmp("interface",buffer)){
      cout << "ButtonInterface remark: received an interface request.\n"; 
      socket.ar_write(interfacePacket,packetLength,&fromAddress);
    }
    else if (!strcmp("button", buffer)){
      cout << "ButtonInterface remark: received a button press.\n";
      int buttonID = *((int*)(buffer+128));
      cout << "  Button ID = " << buttonID << "\n";
      ar_mutex_lock(&executeLock);
      if (commandExecutionRequest){
        // still processing an old command
	cout << "*********************************************************\n";
	cout << "ButtonInterface remark: discarded command. Still processing "
	     << "  previous.\n";
	cout << "*********************************************************\n";
      }
      else{
        executionID = buttonID;
        commandExecutionRequest = true;
        executionVar.signal();
      }
      ar_mutex_unlock(&executeLock);
    }
    else{
      cout << "ButtonInterface remark: received an illegal packet.\n";
    }
  }
}


int main(int argc, char** argv){
  if (argc != 2){
    cerr << "usage: ButtonInterface virtual_computer\n";
    return 1;
  }

  ar_mutex_init(&executeLock);

  szgClient.init(argc, argv);
  if (!szgClient){
    return 1;
  }
  arAppLauncher launcher("ButtonInterface");
  (void)launcher.setSZGClient(&szgClient);
  vircompName = string(argv[1]);
  if (!launcher.setVircomp(vircompName)){
    cerr << "ButtonInterface error: failed to set virtual computer name.\n";
    return 1;
  }

  if (!launcher.setParameters()){
    cout << "ButtonInterface error: virtual computer incorrectly defined.\n";
    return 1;
  }
  hostName = szgClient.getTrigger(vircompName);
  cout << "Control = " << hostName << "\n";
  if (hostName == "NULL") {
    cerr << "ButtonInterface error: undefined trigger host "
	 << "for virtual computer.\n";
    return 1;
  }

  if (!initialize()){
    return 1;
  }

  arThread dummy1(receiveTask, NULL);
  arThread dummy2(commandTask, NULL);

  string messageType;
  string messageBody;
  while(true){
    szgClient.receiveMessage(&messageType, &messageBody);
    if (messageType == "quit"){
      exit(1);
    } 
  }
}
