//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// This must be the first line in every .cpp
#include "arPrecompiled.h"
#include "arGraphicsPeerRPC.h"

// The next block of message handlers really just relay messages
// to the appropriate peers.

// messageBody = connect_to_peer_name
string ar_graphicsPeerHandleConnectToPeer(arGraphicsPeer* peer, 
                                          const string& messageBody){
  stringstream result;
  int connectionID = peer->connectToPeer(messageBody);
  if (connectionID < 0){
    result << "szg-rp error: failed to connect to remote peer"
	   << " (" << messageBody << ").";
    return result.str();
  }
  result << "szg-rp success: peers connected (" << connectionID << ").";
  return result.str();
}

// Format = disconnect_from_peer_name
string ar_graphicsPeerHandleCloseConnection(arGraphicsPeer* peer, 
                                            const string& messageBody){
  stringstream result;
  int socketID = peer->closeConnection(messageBody);
  if (socketID < 0){
    cerr << "szg-rp error: failed to close connection to remote peer"
	 << " (" << messageBody << ").";
    return result.str();
  }
  result << "szg-rp success: peers disconnected.";
  return result.str();
}

// Format = pull_from_peer_name/[0,1]
string ar_graphicsPeerHandlePullSerial(arGraphicsPeer* peer, 
                                        const string& messageBody){
  stringstream result;
  arSlashString parameters(messageBody);
  if (parameters.size() < 6){
    result << "szg-rp error: usage is pull_from_peer_name/remoteRootID/"
	   << "localRootID/send_level/remote_send_on/local_send_on";
    return result.str();
  }
  int remoteRootID = -1;
  int localRootID = -1;
  int sendLevel = -1;
  stringstream value1;
  value1 << parameters[1];
  value1 >> remoteRootID;
  stringstream value2;
  value2 << parameters[2];
  value2 >> localRootID;
  stringstream value3;
  value3 << parameters[3];
  value3 >> sendLevel;
  cout << " (pull serial) peer = " << parameters[0]
       << " remoteRootID = " << remoteRootID
       << " localRootID = " << localRootID
       << " sendLevel = " << sendLevel
       << " remoteSendOn = " << parameters[4] 
       << " localSendOn = " << parameters[5];
  if (!peer->pullSerial(parameters[0], remoteRootID, localRootID, sendLevel,
                        parameters[4] == "1",
                        parameters[5] == "1")){
    result << "szg-rp error: failed to pull from remote peer"
	   << " (" <<  parameters[0] << ").";
    return result.str();
  }
  result << "szg-rp success: pullSerial succeeded.";
  return result.str();
}

// Format = push_to_peer_name/remoteRootID/[0,1]
string ar_graphicsPeerHandlePushSerial(arGraphicsPeer* peer, 
                                       const string& messageBody){
  stringstream result;
  arSlashString parameters(messageBody);
  if (parameters.size() < 6){
    result << "szg-rp error: usage is push_to_peer_name/remoteRootID/"
	   << "localRootID/send_level/remote_send_on/local_send_on";
    return result.str();
  }
  int remoteRootID = -1;
  int localRootID = -1;
  int sendLevel = -1;
  stringstream value1;
  value1 << parameters[1];
  value1 >> remoteRootID;
  stringstream value2;
  value2 << parameters[2];
  value2 >> localRootID;
  stringstream value3;
  value3 << parameters[3];
  value3 >> sendLevel;
  cout << "AARGH! peer = " << parameters[0]
       << " remoteRootID = " << remoteRootID
       << " localRootID = " << localRootID
       << " sendLevel = " << sendLevel
       << " remoteSendOn = " << parameters[4]
       << " localSendOn = " << parameters[5];
  if (!peer->pushSerial(parameters[0], 
                        remoteRootID, localRootID, sendLevel,
                        parameters[4] == "1",
                        parameters[5] == "1")){
    result << "szg-rp error: failed to push to remote peer"
	   << " (" <<  parameters[0] << ", with remote ID = "
	   << remoteRootID << ").";
    return result.str();
  }
  result << "szg-rp success: pushSerial succeeded.";
  return result.str();
}

// messageBody is ignored.
string ar_graphicsPeerHandleCloseAllAndReset(arGraphicsPeer* peer, 
                                             const string&){
  stringstream result;
  peer->closeAllAndReset();
  result << "szg-rp success: reset peer " << peer->getName() << ".";
  return result.str();
}

// Two messages!
// Lets us lock a node in a remote peer to us!
// Format = remote_peer_name/node_ID
string ar_graphicsPeerHandleRemoteLockNode(arGraphicsPeer* peer, 
                                           const string& messageBody, 
                                           bool lock){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node ID.";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[1];
  IDStream >> ID;
  bool state;
  if (lock){
    state = peer->remoteLockNode(bodyList[0], ID);
  }
  else{
    state = peer->remoteUnlockNode(bodyList[0], ID);
  }
  if (!state){
    result << "szg-rp error: lock operation failed on node " << ID 
	   << " on peer " << bodyList[0] << ".";
    return result.str();
  }
  if (lock){
    result << "szg-rp success: node locked.";
  }
  else{
    result << "szg-rp success: node unlocked.";
  }
  return result.str();
}

// Two messages!
// Lets us lock a node in a remote peer to us!
// Format = remote_peer_name/node_ID
string ar_graphicsPeerHandleRemoteLockNodeBelow(arGraphicsPeer* peer, 
                                                const string& messageBody, 
                                                bool lock){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node ID.";
    return result.str();
  }
  stringstream IDStream;
  int ID;
  IDStream << bodyList[1];
  IDStream >> ID;
  bool state;
  if (lock){
    state = peer->remoteLockNodeBelow(bodyList[0], ID);
  }
  else{
    state = peer->remoteUnlockNodeBelow(bodyList[0], ID);
  }
  if (!state){
    result << "szg-rp error: lock operation failed on node tree " << ID 
	   << " on peer " << bodyList[0] << ".";
    return result.str();
  }
  if (lock){
    result << "szg-rp success: node tree locked.";
  }
  else{
    result << "szg-rp success: node tree unlocked.";
  }
  return result.str();
}

// Two messages!
// It lets us lock a local node to a particular peer. Dual of handleLock.
// Format = remote_peer_name
string ar_graphicsPeerHandleLocalLockNode(arGraphicsPeer* peer, 
                                          const string& messageBody, 
                                          bool lock){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node ID.";
    return result.str();
  }
  stringstream IDStream;
  int ID = -1;
  IDStream << bodyList[1];
  IDStream >> ID;
  const bool state = lock ?
    peer->localLockNode(bodyList[0], ID) :
    peer->localUnlockNode(ID);
    // The second parameter is meaningless if !lock.
  if (!state){
    result << "szg-rp error: lock operation failed on node " << ID 
	   << " on peer " << bodyList[0] << ".";
    return result.str();
  }
  result << lock ? "szg-rp success: node locked." :
                   "szg-rp success: node unlocked.";
  return result.str();
}

// Format = remote_peer_name/remote_node_ID/filter_value
string ar_graphicsPeerHandleRemoteFilterDataBelow(arGraphicsPeer* peer, 
                                                  const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 3){
    result << "szg-rp error: body format must be remote peer,"
	   << "local node ID, filter code.";
    return result.str();
  }
  stringstream IDStream;
  int ID = -1;
  IDStream << bodyList[1];
  IDStream >> ID;
  stringstream valueStream;
  int value = -1;
  valueStream << bodyList[2];
  valueStream >> value;
  if (peer->remoteFilterDataBelow(bodyList[0], ID, value)){
    result << "szg-rp success: filter applied.";
  }
  else{
    result << "szg-rp error: could not apply filter.";
  }
  return result.str();
}

// Format = remote_peer_name/remote_node_ID/filter_value
string ar_graphicsPeerHandleLocalFilterDataBelow(arGraphicsPeer* peer, 
                                                 const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 3){
    result << "szg-rp error: body format must be remote peer,"
	   << "local node ID, filter code.";
    return result.str();
  }
  stringstream IDStream;
  int ID = -1;
  IDStream << bodyList[1];
  IDStream >> ID;
  stringstream valueStream;
  int value = -1;
  valueStream << bodyList[2];
  valueStream >> value;
  if (peer->localFilterDataBelow(bodyList[0], ID, value)){
    result << "szg-rp success: filter applied.";
  }
  else{
    result << "szg-rp error: could not apply filter.";
  }
  return result.str();
}

// Format = node_name
string ar_graphicsPeerHandleGetNodeID(arGraphicsPeer* peer,
				      const string& messageBody){
  stringstream result;
  result << "szg-rp success: node ID = ("
	 << peer->getNodeID(messageBody) << ")";
  return result.str(); 
}

// Format = remote_peer_name/remote_node_name
string ar_graphicsPeerHandleRemoteNodeID(arGraphicsPeer* peer, 
                                         const string& messageBody){
  stringstream result;
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2){
    result << "szg-rp error: body format must be remote peer followed by "
	   << "node name.";
    return result.str();
  }
  result << "szg-rp success: node ID = ("
	 << peer->remoteNodeID(bodyList[0], bodyList[1]) << ")";
  return result.str();
}

// Two messages!
// Format = file_name
string ar_graphicsPeerHandleReadDatabase(arGraphicsPeer* peer, 
                                         const string& messageBody, 
                                         bool useXML){
  const bool ok = useXML ?
    peer->readDatabaseXML(messageBody) :
    peer->readDatabase(messageBody);
  stringstream result;
  if (!ok){
    result << "szg-rp error: failed to read specified database.";
    return result.str();
  }
  result << "szg-rp success: database loaded.";
  return result.str();
}

// Two messages!
// Format = file_name
string ar_graphicsPeerHandleWriteDatabase(arGraphicsPeer* peer, 
                                          const string& messageBody, 
                                          bool useXML){
  const bool ok = useXML ?
    peer->writeDatabaseXML(messageBody) :
    peer->writeDatabase(messageBody);
  stringstream result;
  if (!ok){
    result << "szg-rp error: failed to write specified database.";
    return result.str();
  }
  result << "szg-rp success: database saved.";
  return result.str();
}

// Two messages!
// Format = remote_node_ID/file_name
string ar_graphicsPeerHandleWriteRooted(arGraphicsPeer* peer,
					const string& messageBody,
					bool useXML){
  stringstream result;
  arSlashString parameters(messageBody);
  if (parameters.length() < 2){
    result << "szg-rp error: format must be remote_node_ID/file_name.";
    return result.str();
  }
  stringstream value;
  value << parameters[0];
  int ID = -1;
  value >> ID;
  // Is there a node with this ID?
  arDatabaseNode* node = peer->getNode(ID);
  if (!node){
    result << "szg-rp error: no node with ID=" << ID << ".";
    return result.str();
  }
  const bool ok = useXML ?
    peer->writeRootedXML(node, parameters[1]) :
    peer->writeRooted(node, parameters[1]);
  if (!ok){
    result << "szg-rp error: failed to save specified database.";
    return result.str();
  }
  result << "szg-rp success: database saved.";
  return result.str();
}

// Two messages!
// Format = remote_node_ID/file_name
string ar_graphicsPeerHandleAttach(arGraphicsPeer* peer, 
                                   const string& messageBody, 
                                   bool useXML){
  stringstream result;
  arSlashString parameters(messageBody);
  if (parameters.length() < 2){
    result << "szg-rp error: format must be remote_node_ID/file_name.";
    return result.str();
  }
  stringstream value;
  value << parameters[0];
  int ID = -1;
  value >> ID;
  // Is there a node with this ID?
  arDatabaseNode* node = peer->getNode(ID);
  if (!node){
    result << "szg-rp error: no node with ID=" << ID << ".";
    return result.str();
  }
  const bool ok = useXML ?
    peer->attachXML(node, parameters[1]) :
    peer->attach(node, parameters[1]);
  if (!ok){
    result << "szg-rp error: failed to attach specified database.";
    return result.str();
  }
  result << "szg-rp success: database attached.";
  return result.str();
}

// Two messages!
// Format = remote_node_ID/file_name
string ar_graphicsPeerHandleMerge(arGraphicsPeer* peer, 
                                  const string& messageBody, 
                                  bool useXML){
  stringstream result;
  arSlashString parameters(messageBody);
  if (parameters.length() < 2){
    result << "szg-rp error: format must be remote_node_ID/file_name.";
    return result.str();
  }
  stringstream value;
  value << parameters[0];
  int ID = -1;
  value >> ID;
  arDatabaseNode* node = peer->getNode(ID);
  if (!node){
    result << "szg-rp error: no node with ID=" << ID << ".";
    return result.str();
  }
  const bool ok = useXML ?
    peer->mergeXML(node, parameters[1]) :
    peer->merge(node, parameters[1]);
  if (!ok){
    result << "szg-rp error: failed to merge specified database.";
    return result.str();
  }
  result << "szg-rp success: database merged.";
  return result.str();
}

// messageBody is unused.
string ar_graphicsPeerHandlePrintConnections(arGraphicsPeer* peer,
					     const string&){
  stringstream result;
  result << "szg-rp success:\n" << peer->printConnections();
  return result.str();
}

// messageBody is unused.
string ar_graphicsPeerHandlePrintPeer(arGraphicsPeer* peer, 
                                  const string&){
  stringstream result;
  result << "szg-rp success:\n" << peer->printPeer();
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
    actualMessageBody = messageBody.length() > position+1 ?
	messageBody.substr(position+1, messageBody.length()-position-1) : "";
  }
  messageBody = actualMessageBody;
  return peerName;
}

string ar_graphicsPeerHandleMessage(arGraphicsPeer* peer,
                                    const string& messageType,
				    const string& messageBody){
  string responseBody;
  if (messageType == "connect_to_peer"){
    responseBody = ar_graphicsPeerHandleConnectToPeer(peer,messageBody);
  }
  else if (messageType == "close_connection"){
    responseBody = ar_graphicsPeerHandleCloseConnection(peer, messageBody);
  }
  else if (messageType == "pull_serial"){
    responseBody = ar_graphicsPeerHandlePullSerial(peer, messageBody);
  }
  else if (messageType == "push_serial"){
    responseBody = ar_graphicsPeerHandlePushSerial(peer, messageBody);
  }
  else if (messageType == "close_all_and_reset"){
    responseBody = ar_graphicsPeerHandleCloseAllAndReset(peer, messageBody);
  }
  else if (messageType =="remote_lock_node"){
    responseBody = ar_graphicsPeerHandleRemoteLockNode(peer, 
                                                       messageBody, 
                                                       true);
  }
  else if (messageType =="remote_lock_node_below"){
    responseBody = ar_graphicsPeerHandleRemoteLockNodeBelow(peer, 
                                                            messageBody, 
                                                            true);
  }
  else if (messageType == "remote_unlock_node"){
    responseBody = ar_graphicsPeerHandleRemoteLockNode(peer,  
                                                       messageBody, 
                                                       false);
  } 
  else if (messageType == "remote_unlock_node_below"){
    responseBody = ar_graphicsPeerHandleRemoteLockNodeBelow(peer,  
                                                            messageBody, 
                                                            false);
  } 
  else if (messageType == "local_lock_node"){
    responseBody = ar_graphicsPeerHandleLocalLockNode(peer, 
                                                     messageBody, 
                                                     true);
  }
  else if (messageType == "local_unlock_node"){
    responseBody = ar_graphicsPeerHandleLocalLockNode(peer, 
                                                      messageBody, 
                                                      false);
  }
  else if (messageType == "remote_filter_data_below"){
    responseBody 
      = ar_graphicsPeerHandleRemoteFilterDataBelow(peer, messageBody);
  }
  else if (messageType == "local_filter_data_below"){
    responseBody
      = ar_graphicsPeerHandleLocalFilterDataBelow(peer, messageBody);
  }
  else if (messageType == "get_node_id"){
    responseBody = ar_graphicsPeerHandleGetNodeID(peer, messageBody);
  }
  else if (messageType == "remote_node_id"){
    responseBody = ar_graphicsPeerHandleRemoteNodeID(peer, messageBody);
  }
  else if (messageType == "read_database"){
    responseBody 
      = ar_graphicsPeerHandleReadDatabase(peer, messageBody, false);
  }
  else if (messageType == "read_database_xml"){
    responseBody = ar_graphicsPeerHandleReadDatabase(peer, messageBody, true);
  }
  else if (messageType == "write_database"){
    responseBody 
      = ar_graphicsPeerHandleWriteDatabase(peer, messageBody, false);
  }
  else if (messageType == "write_database_xml"){
    responseBody = ar_graphicsPeerHandleWriteDatabase(peer, 
                                                      messageBody, 
                                                      true);
  }
  else if (messageType == "write_rooted"){
    responseBody = ar_graphicsPeerHandleWriteRooted(peer,
						    messageBody,
						    false);
  }
  else if (messageType == "write_rooted_xml"){
    responseBody = ar_graphicsPeerHandleWriteRooted(peer,
						    messageBody,
						    true);
  }
  else if (messageType == "attach"){
    responseBody = ar_graphicsPeerHandleAttach(peer, messageBody, false);
  }
  else if (messageType == "attach_xml"){
    responseBody = ar_graphicsPeerHandleAttach(peer, messageBody, true);
  }
  else if (messageType == "merge"){
    responseBody = ar_graphicsPeerHandleMerge(peer, messageBody, false);
  }
  else if (messageType == "merge_xml"){
    responseBody = ar_graphicsPeerHandleMerge(peer, messageBody, true);
  }
  else if (messageType == "print_connections"){
    responseBody = ar_graphicsPeerHandlePrintConnections(peer, messageBody);
  }
  else if (messageType == "print_peer"){
    responseBody = ar_graphicsPeerHandlePrintPeer(peer, messageBody);
  }
  else{
    responseBody = string("szg-rp error: unknown message type.\n");
  }
  return responseBody;
}
