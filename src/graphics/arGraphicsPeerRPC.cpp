//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// This must be the first line in every .cpp
#include "arPrecompiled.h"
#include "arGraphicsPeerRPC.h"

// The next block of message handlers really just relay messages
// to the appropriate peers.

// messageBody is ignored.
string ar_graphicsPeerHandleReset(arGraphicsPeer* peer, 
                                  const string&){
  stringstream result;
  peer->closeAllAndReset();
  result << "szg-rp success: reset peer " << peer->getName() << ".\n";
  return result.str();
}

// messageBody = connect_to_peer_name
string ar_graphicsPeerHandleConnect(arGraphicsPeer* peer, 
                                    const string& messageBody){
  stringstream result;
  int connectionID = peer->connectToPeer(messageBody);
  if (connectionID < 0){
    result << "szg-rp error: failed to connect to remote peer.\n"
	   << "  (" << messageBody << ")\n";
    return result.str();
  }
  result << "szg-rp success: peers connected (" << connectionID << ").\n";
  return result.str();
}

// BUG BUG BUG BUG BUG BUG BUG BUG! This is really dealing with *two* messages,
// serialize and afterwards suck new stuff in... and serialize and afterwards
// do not.
// Format = connect_to_peer_name 
string ar_graphicsPeerHandleConnectDump(arGraphicsPeer* peer, 
                                        const string& messageBody, 
                                        bool relayOn){
  stringstream result;
  int connectionID = peer->connectToPeer(messageBody);
  if (connectionID < 0){
    result << "szg-rp error: failed to connect to remote peer.\n"
	   << "  (" <<  messageBody << ")\n";
    return result.str();
  }
  peer->pullSerial(messageBody, relayOn);
  result << "szg-rp success: connect-dump succeeded (" << connectionID 
         << ").\n";
  return result.str();
}

// Again, really two messages.
// Format = push_to_peer_name
string ar_graphicsPeerHandlePushSerial(arGraphicsPeer* peer, 
                                       const string& messageBody, bool sendOn){
  stringstream result;
  if (!peer->pushSerial(messageBody, 0, sendOn)){
    result << "szg-rp error: failed to send serialization to named peer.\n";
  }
  else{
    result << "szg-rp success: push serial succeeded.\n";
  }
  return result.str();
}

// Format = disconnect_from_peer_name
string ar_graphicsPeerHandleDisconnect(arGraphicsPeer* peer, 
                                       const string& messageBody){
  stringstream result;
  int socketID = peer->closeConnection(messageBody);
  if (socketID < 0){
    cerr << "szg-rp error: failed to close connection to remote peer.\n"
	 << "  (" << messageBody << ")\n";
    return result.str();
  }
  result << "szg-rp success: peers disconnected.\n";
  return result.str();
}

// Again, really two messages.
// Format = receive_from_peer_name
string ar_graphicsPeerHandleRelay(arGraphicsPeer* peer, 
                                  const string& messageBody, bool state){
  stringstream result;
  if (!peer->receiving(messageBody, state)){
    result << "szg-rp error: remote peer not connected.\n"
	   << "  (" << messageBody << ")\n";
    return result.str();
  }
  if (state){
    result << "szg-rp success: relay is on.\n";
  }
  else{
    result << "szg-rp success: relay is off.\n";
  }
  return result.str();
}

// Again, two messages.
// Format = send_to_peer_name
string ar_graphicsPeerHandleSend(arGraphicsPeer* peer, 
                                 const string& messageBody, bool state){
  stringstream result;
  if (!peer->sending(messageBody, state)){
    result << "szg-rp error: remote peer not connected.\n"
	   << "  (" << messageBody << ")\n";
    return result.str();
  }
  if (state){
    result << "szg-rp success: send is on.\n";
  }
  else{
    result << "szg-rp success: send is off.\n";
  }
  return result.str();
}

// messageBody is unused.
string ar_graphicsPeerHandlePrint(arGraphicsPeer* peer, 
                                  const string&){
  stringstream result;
  result << "szg-rp success:\n";
  result << peer->print();
  return result.str();
}

// Two messages!
// Format = file_name
string ar_graphicsPeerHandleLoad(arGraphicsPeer* peer, 
                                 const string& messageBody, bool useXML){
  stringstream result;
  bool state = false;
  if (useXML){
    state = peer->readDatabaseXML(messageBody);
  }
  else{
    state = peer->readDatabase(messageBody);
  }
  if (!state){
    result << "szg-rp error: failed to load specified database.\n";
    return result.str();
  }
  result << "szg-rp success: database loaded.\n";
  return result.str();
}

// Two messages!
// Format = file_name
string ar_graphicsPeerHandleSave(arGraphicsPeer* peer, 
                                 const string& messageBody, bool useXML){
  stringstream result;
  bool state = false;
  if (useXML){
    state = peer->writeDatabaseXML(messageBody);
  }
  else{
    state = peer->writeDatabase(messageBody);
  }
  if (!state){
    result << "szg-rp error: failed to save specified database.\n";
    return result.str();
  }
  result << "szg-rp success: database saved.\n";
  return result.str();
}

// Two messages!
// Lets us lock a node in a remote peer to us!
// Format = remote_peer_name/node_ID
string ar_graphicsPeerHandleLock(arGraphicsPeer* peer, 
                                 const string& messageBody, bool lock){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node ID.\n";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[2];
  IDStream >> ID;
  bool state;
  if (lock){
    state = peer->lockRemoteNode(bodyList[1], ID);
  }
  else{
    state = peer->unlockRemoteNode(bodyList[1], ID);
  }
  if (!state){
    result << "szg-rp error: lock operation failed on node " << ID 
	   << " on peer " << bodyList[1] << "\n";
    return result.str();
  }
  if (lock){
    result << "szg-rp success: node locked.\n";
  }
  else{
    result << "szg-rp success: node unlocked.\n";
  }
  return result.str();
}

// Two messages!
// It lets us lock a local node to a particular peer. Dual of handleLock.
// Format = remote_peer_name
string ar_graphicsPeerHandleLockLocal(arGraphicsPeer* peer, 
                                      const string& messageBody, bool lock){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node ID.\n";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[2];
  IDStream >> ID;
  bool state;
  if (lock){
    state = peer->lockLocalNode(bodyList[1], ID);
  }
  else{
    // NOTE: the second parameter is meaningless in this case.
    state = peer->unlockLocalNode(ID);
  }
  if (!state){
    result << "szg-rp error: lock operation failed on node " << ID 
	   << " on peer " << bodyList[1] << "\n";
    return result.str();
  }
  if (lock){
    result << "szg-rp success: node locked.\n";
  }
  else{
    result << "szg-rp success: node unlocked.\n";
  }
  return result.str();
}

// Format = remote_peer_name/remote_node_ID/filter_value
string ar_graphicsPeerHandleDataFilter(arGraphicsPeer* peer, 
                                       const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 3){
    result << "szg-rp error: body format must be remote peer,"
	   << "local node ID, filter code.\n";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[2];
  IDStream >> ID;
  stringstream valueStream;
  int value;
  valueStream << bodyList[3];
  valueStream >> value;
  peer->filterDataBelowRemote(bodyList[1], ID, value);
  result << "szg-rp success: filter applied.\n";
  return result.str();
}

// Format = remote_peer_name/remote_node_name
string ar_graphicsPeerHandleNodeID(arGraphicsPeer* peer, 
                                   const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node name.\n";
    return result.str();
  }
  result << "szg-rp success: node ID =\n  "
	 << peer->getNodeIDRemote(bodyList[1], bodyList[2]);
  return result.str();
}

string ar_graphicsPeerStripName(string& messageBody){
  string peerName;
  string actualMessageBody;
  unsigned int position = messageBody.find('/');
  if (position == string::npos){
    // Just the peer name.
    peerName = messageBody;
    actualMessageBody = "";
  }
  else{
    peerName = messageBody.substr(0, position);
    if (messageBody.length() > position+1){
      actualMessageBody 
        = messageBody.substr(position+1, messageBody.length()-position-1);
    }
    else{
      actualMessageBody = "";
    }
  }
  messageBody = actualMessageBody;
  return peerName;
}

string ar_graphicsPeerHandleMessage(arGraphicsPeer* peer,
                                    const string& messageType,
				    const string& messageBody){
  string responseBody;

  if (messageType == "connect"){
    responseBody = ar_graphicsPeerHandleConnect(peer,messageBody);
  }
  else if (messageType == "connect-dump"){
    responseBody = ar_graphicsPeerHandleConnectDump(peer, messageBody, false);
  }
  else if (messageType == "connect-dump-relay"){
    responseBody = ar_graphicsPeerHandleConnectDump(peer, messageBody, true);
  }
  else if (messageType == "push-serial"){
    responseBody = ar_graphicsPeerHandlePushSerial(peer, messageBody, false);
  }
  else if (messageType == "push-serial-send"){
    responseBody = ar_graphicsPeerHandlePushSerial(peer, messageBody, true);
  }
  else if (messageType == "disconnect"){
    responseBody = ar_graphicsPeerHandleDisconnect(peer, messageBody);
  }
  else if (messageType == "relay-on"){
    responseBody = ar_graphicsPeerHandleRelay(peer, messageBody, true);
  }
  else if (messageType == "relay-off"){
    responseBody = ar_graphicsPeerHandleRelay(peer, messageBody, false);
  }
  else if (messageType == "send-on"){
    responseBody = ar_graphicsPeerHandleSend(peer, messageBody, true);
  }
  else if (messageType == "send-off"){
    responseBody = ar_graphicsPeerHandleSend(peer, messageBody, false);
  }
  else if (messageType == "reset"){
    responseBody = ar_graphicsPeerHandleReset(peer, messageBody);
  }
  else if (messageType == "print"){
    responseBody = ar_graphicsPeerHandlePrint(peer, messageBody);
  }
  else if (messageType == "load"){
    responseBody = ar_graphicsPeerHandleLoad(peer, messageBody, false);
  }
  else if (messageType == "load-xml"){
    responseBody = ar_graphicsPeerHandleLoad(peer, messageBody, true);
  }
  else if (messageType == "save"){
    responseBody = ar_graphicsPeerHandleSave(peer, messageBody, false);
  }
  else if (messageType == "save-xml"){
    responseBody = ar_graphicsPeerHandleSave(peer, messageBody, true);
  }
  else if (messageType =="lock"){
    responseBody = ar_graphicsPeerHandleLock(peer, messageBody, true);
  }
  else if (messageType == "unlock"){
    responseBody = ar_graphicsPeerHandleLock(peer, messageBody, false);
  } 
  else if (messageType == "lock-local"){
    responseBody = ar_graphicsPeerHandleLockLocal(peer, messageBody, true);
  }
  else if (messageType == "unlock-local"){
    responseBody = ar_graphicsPeerHandleLockLocal(peer, messageBody, false);
  }
  else if (messageType == "filter_data"){
    responseBody = ar_graphicsPeerHandleDataFilter(peer, messageBody);
  }
  else if (messageType == "node-ID"){
    responseBody = ar_graphicsPeerHandleNodeID(peer, messageBody);
  }
  else{
    responseBody = string("szg-rp error: unknown message type.\n");
  }
  return responseBody;
}
