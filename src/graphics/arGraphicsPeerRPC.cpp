//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// This must be the first line in every .cpp
#include "arPrecompiled.h"
#include "arGraphicsPeerRPC.h"
#include "arGraphicsUtilities.h"

// The next block of message handlers really just relay messages
// to the appropriate peers.

// messageBody = connect_to_peer_name
string ar_graphicsPeerHandleConnectToPeer(arGraphicsPeer* peer, 
                                          const string& messageBody) {
  const int connectionID = peer->connectToPeer(messageBody);
  return (connectionID < 0) ?
    "szg-rp error: failed to connect remote peer (" + messageBody + ")." :
    "szg-rp success: peers connected (" + ar_intToString(connectionID) + ").";
}

// Format = disconnect_from_peer_name
string ar_graphicsPeerHandleCloseConnection(arGraphicsPeer* peer, 
                                            const string& messageBody) {
  return (peer->closeConnection(messageBody) < 0) ?
    "szg-rp error: failed to disconnect remote peer (" + messageBody + ")." :
    "szg-rp success: peers disconnected";
}

// Format = pull_from_peer_name/[0,1]
string ar_graphicsPeerHandlePullSerial(arGraphicsPeer* peer, 
                                        const string& messageBody) {
  arSlashString parameters(messageBody);
  if (parameters.size() < 6) {
    return "szg-rp error: usage is pull_from_peer_name/remoteRootID/localRootID/send_level/remote_send_on/local_send_on";
  }

  int remoteRootID = -1;
  int localRootID = -1;
  int sendLevel = -1;
  int remoteSendLevel = AR_TRANSIENT_NODE;
  int localSendLevel = AR_TRANSIENT_NODE;
  stringstream value1;
  value1 << parameters[1];
  value1 >> remoteRootID;
  stringstream value2;
  value2 << parameters[2];
  value2 >> localRootID;
  stringstream value3;
  value3 << parameters[3];
  value3 >> sendLevel;
  stringstream value4;
  value4 << parameters[4];
  value4 >> remoteSendLevel;
  stringstream value5;
  value5 << parameters[5];
  value5 >> localSendLevel;
  cout << " (pull serial) peer = " << parameters[0]
       << " remoteRootID = " << remoteRootID
       << " localRootID = " << localRootID
       << " sendLevel = " << sendLevel
       << " remoteSendLevel = " << remoteSendLevel 
       << " localSendLevel = " << localSendLevel;
  if (!peer->pullSerial(parameters[0], remoteRootID, localRootID, 
                        ar_convertToNodeLevel(sendLevel),
                        ar_convertToNodeLevel(remoteSendLevel),
                        ar_convertToNodeLevel(localSendLevel))) {
    return "szg-rp error: failed to pull from remote peer (" + parameters[0] + ").";
  }
  return "szg-rp success: pullSerial succeeded.";
}

// Format = push_to_peer_name/remoteRootID/[0,1]
string ar_graphicsPeerHandlePushSerial(arGraphicsPeer* peer, 
                                       const string& messageBody) {
  arSlashString parameters(messageBody);
  if (parameters.size() < 6) {
    return "szg-rp error: usage is push_to_peer_name/remoteRootID/localRootID/send_level/remote_send_on/local_send_on";
  }

  int remoteRootID = -1;
  int localRootID = -1;
  int sendLevel = -1;
  int remoteSendLevel = AR_TRANSIENT_NODE;
  int localSendLevel = AR_TRANSIENT_NODE;
  stringstream value1;
  value1 << parameters[1];
  value1 >> remoteRootID;
  stringstream value2;
  value2 << parameters[2];
  value2 >> localRootID;
  stringstream value3;
  value3 << parameters[3];
  value3 >> sendLevel;
  stringstream value4;
  value4 << parameters[4];
  value4 >> remoteSendLevel;
  stringstream value5;
  value5 << parameters[5];
  value5 >> localSendLevel;
  cout << "(push serial) peer = " << parameters[0]
       << " remoteRootID = " << remoteRootID
       << " localRootID = " << localRootID
       << " sendLevel = " << sendLevel
       << " remoteSendLevel = " << remoteSendLevel
       << " localSendLevel = " << localSendLevel;
  if (!peer->pushSerial(parameters[0], 
                        remoteRootID, localRootID,
                        ar_convertToNodeLevel(sendLevel),
                        ar_convertToNodeLevel(remoteSendLevel),
                        ar_convertToNodeLevel(localSendLevel))) {
    return "szg-rp error: failed to push to remote peer (" + parameters[0] +
      ", with remote ID = " + ar_intToString(remoteRootID) + ").";
  }
  return "szg-rp success: pushSerial succeeded.";
}

// messageBody is ignored.
string ar_graphicsPeerHandleCloseAllAndReset(arGraphicsPeer* peer, 
                                             const string&) {
  peer->closeAllAndReset();
  return "szg-rp success: reset peer " + peer->getName() + ".";
}

// Ping a remote peer.
// Format = remote_peer_name
string ar_graphicsPeerHandlePingPeer(arGraphicsPeer* peer,
                                     const string& messageBody) {
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 1) {
    return "szg-rp error: body must contain name of remote peer.";
  }

  return peer->pingPeer(bodyList[0]) ?
    "szg-rp success: pingPeer." :
    "szg-rp error: pingPeer.";
}

// Two messages!  
// Lock a node in a remote peer to us.
// Format = remote_peer_name/node_ID
string ar_graphicsPeerHandleRemoteLockNode(arGraphicsPeer* peer, 
                                           const string& messageBody, 
                                           bool lock) {
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2) {
    return "szg-rp error: body format must be remote peer followed by node ID.";
  }

  int ID;
  if (!ar_stringToIntValid(bodyList[1], ID))
    return "szg-rp error: wrong body format.";

  // todo: combine remoteLockNode remoteUnlockNode.  Ditto remoteLockNodeBelow.
  const bool ok = lock ?
    peer->remoteLockNode(bodyList[0], ID) :
    peer->remoteUnlockNode(bodyList[0], ID);
  if (!ok) {
    return "szg-rp error: lock operation failed on node " + ar_intToString(ID) +
           " on peer " + bodyList[0] + ".";
  }

  return lock ?
    "szg-rp success: node locked." :
    "szg-rp success: node unlocked.";
}

// Two messages!
// Lock a node in a remote peer to us.
// Format = remote_peer_name/node_ID
string ar_graphicsPeerHandleRemoteLockNodeBelow(arGraphicsPeer* peer, 
                                                const string& messageBody, 
                                                bool lock) {
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2) {
    return "szg-rp error: body format must be remote peer followed by node ID.";
  }

  int ID;
  if (!ar_stringToIntValid(bodyList[1], ID))
    return "szg-rp error: wrong body format.";

  const bool ok = lock ?
    peer->remoteLockNodeBelow(bodyList[0], ID) :
    peer->remoteUnlockNodeBelow(bodyList[0], ID);
  if (!ok) {
    return "szg-rp error: lock operation failed on node tree " + ar_intToString(ID) +
           " on peer " + bodyList[0] + ".";
  }

  return lock ?
    "szg-rp success: node tree locked." :
    "szg-rp success: node tree unlocked.";
}

// Two messages!
// Lock a local node to a particular peer. Dual of handleLock.
// Format = remote_peer_name
string ar_graphicsPeerHandleLocalLockNode(arGraphicsPeer* peer, 
                                          const string& messageBody, 
                                          bool lock) {
  arSlashString bodyList(messageBody);
  if (bodyList.length() < 2) {
    return "szg-rp error: body format must be remote peer followed by node ID.";
  }

  int ID;
  if (!ar_stringToIntValid(bodyList[1], ID))
    return "szg-rp error: wrong body format.";

  const bool ok = lock ?
    peer->localLockNode(bodyList[0], ID) :
    peer->localUnlockNode(ID);
    // The second parameter is meaningless if !lock.
  if (!ok) {
    return "szg-rp error: lock operation failed on node " + ar_intToString(ID) +
           " on peer " + bodyList[0] + ".";
  }

  return lock ?
    "szg-rp success: node locked." :
    "szg-rp success: node unlocked.";
}

// Format = remote_peer_name/remote_node_ID/filter_value
string ar_graphicsPeerHandleRemoteFilterDataBelow(arGraphicsPeer* peer, 
                                                  const string& messageBody) {
  const arSlashString bodyList(messageBody);
  if (bodyList.length() < 3) {
    return "szg-rp error: body format must be remote peer, local node ID, filter code.";
  }

  int ID;
  if (!ar_stringToIntValid(bodyList[1], ID))
    return "szg-rp error: wrong body format.";

  int value = AR_TRANSIENT_NODE;
  (void)ar_stringToIntValid(bodyList[2], value);
  return peer->remoteFilterDataBelow(bodyList[0], ID, ar_convertToNodeLevel(value)) ?
    "szg-rp success: filter applied." :
    "szg-rp error: failed to apply filter.";
}

// Format = remote_peer_name/remote_node_ID/filter_value
string ar_graphicsPeerHandleLocalFilterDataBelow(arGraphicsPeer* peer, 
                                                 const string& messageBody) {
  const arSlashString bodyList(messageBody);
  if (bodyList.length() < 3) {
    return "szg-rp error: body format must be remote peer, local node ID, filter code.";
  }

  int ID;
  if (!ar_stringToIntValid(bodyList[1], ID))
    return "szg-rp error: wrong body format.";

  int value = AR_TRANSIENT_NODE;
  (void)ar_stringToIntValid(bodyList[2], value);
  return peer->localFilterDataBelow(bodyList[0], ID, ar_convertToNodeLevel(value)) ?
    "szg-rp success: filter applied." :
    "szg-rp error: failed to apply filter.";
}

// Format = node_name
string ar_graphicsPeerHandleGetNodeID(arGraphicsPeer* peer,
                                      const string& messageBody) {
  return "szg-rp success: node ID = (" +
    ar_intToString(peer->getNodeID(messageBody)) + ")";
}

// Format = remote_peer_name/remote_node_name
string ar_graphicsPeerHandleRemoteNodeID(arGraphicsPeer* peer, 
                                         const string& messageBody) {
  const arSlashString bodyList(messageBody);
  if (bodyList.length() < 2) {
    return "szg-rp error: body format must be remote peer followed by node name.";
  }

  return "szg-rp success: node ID = (" +
    ar_intToString(peer->remoteNodeID(bodyList[0], bodyList[1])) + ")";
}

// Two messages!
// Format = file_name
string ar_graphicsPeerHandleReadDatabase(arGraphicsPeer* peer, 
                                         const string& messageBody, 
                                         bool useXML) {
  const bool ok = useXML ?
    peer->readDatabaseXML(messageBody) :
    peer->readDatabase(messageBody);
  return ok ?
    "szg-rp success: database loaded." :
    "szg-rp error: failed to read specified database.";
}

// Two messages!
// Format = file_name
string ar_graphicsPeerHandleWriteDatabase(arGraphicsPeer* peer, 
                                          const string& messageBody, 
                                          bool useXML) {
  const bool ok = useXML ?
    peer->writeDatabaseXML(messageBody) :
    peer->writeDatabase(messageBody);
  return ok ?
    "szg-rp success: database saved." :
    "szg-rp error: failed to write specified database.";
}

// Two messages!
// Format = remote_node_ID/file_name
string ar_graphicsPeerHandleWriteRooted(arGraphicsPeer* peer,
                                        const string& messageBody,
                                        bool useXML) {
  arSlashString parameters(messageBody);
  if (parameters.length() < 2) {
    return "szg-rp error: format must be remote_node_ID/file_name.";
  }

  int ID;
  if (!ar_stringToIntValid(parameters[0], ID))
    return "szg-rp error: wrong format.";

  arDatabaseNode* node = peer->getNode(ID);
  if (!node) {
    return "szg-rp error: no node with ID " + ar_intToString(ID) + ".";
  }

  const bool ok = useXML ?
    peer->writeRootedXML(node, parameters[1]) :
    peer->writeRooted(node, parameters[1]);
  return ok ?
    "szg-rp success: database saved." :
    "szg-rp error: failed to save specified database.";
}

// Two messages!
string ar_graphicsPeerHandleAttach(arGraphicsPeer* peer, 
                                   const string& messageBody, 
                                   bool useXML) {
  arSlashString parameters(messageBody);
  if (parameters.length() < 2) {
    return "szg-rp error: format must be remote_node_ID/file_name.";
  }

  int ID;
  if (!ar_stringToIntValid(parameters[0], ID))
    return "szg-rp error: wrong format.";

  arDatabaseNode* node = peer->getNode(ID);
  if (!node) {
    return "szg-rp error: no node with ID " + ar_intToString(ID) + ".";
  }

  const bool ok = useXML ?
    peer->attachXML(node, parameters[1]) :
    peer->attach(node, parameters[1]);
  return ok ?
    "szg-rp success: database attached." :
    "szg-rp error: failed to attach specified database.";
}

// Two messages!
string ar_graphicsPeerHandleMerge(arGraphicsPeer* peer, 
                                  const string& messageBody, 
                                  bool useXML) {
  arSlashString parameters(messageBody);
  if (parameters.length() < 2) {
    return "szg-rp error: format must be remote_node_ID/file_name.";
  }

  int ID;
  if (!ar_stringToIntValid(parameters[0], ID))
    return "szg-rp error: wrong format.";

  arDatabaseNode* node = peer->getNode(ID);
  if (!node) {
    return "szg-rp error: no node with ID " + ar_intToString(ID) + ".";
  }

  const bool ok = useXML ?
    peer->mergeXML(node, parameters[1]) :
    peer->merge(node, parameters[1]);
  return ok ?
    "szg-rp success: database merged." :
    "szg-rp error: failed to merge specified database.";
}

string ar_graphicsPeerHandlePrintConnections(arGraphicsPeer* peer, const string&) {
  return "szg-rp success:\n" + peer->printConnections();
}

string ar_graphicsPeerHandlePrintPeer(arGraphicsPeer* peer, const string&) {
  return "szg-rp success:\n" + peer->printPeer();
}

string ar_graphicsPeerStripName(string& messageBody) {
  string peerName;
  string actualMessageBody;
  unsigned int position = messageBody.find('/');
  if (position == string::npos) {
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
                                    const string& msgType,
                                    const string& msgBody) {
  return (msgType == "connect_to_peer") ?
      ar_graphicsPeerHandleConnectToPeer(peer, msgBody) :
    (msgType == "close_connection") ?
      ar_graphicsPeerHandleCloseConnection(peer, msgBody) :
    (msgType == "pull_serial") ?
      ar_graphicsPeerHandlePullSerial(peer, msgBody) :
    (msgType == "push_serial") ?
      ar_graphicsPeerHandlePushSerial(peer, msgBody) :
    (msgType == "close_all_and_reset") ?
      ar_graphicsPeerHandleCloseAllAndReset(peer, msgBody) :
    (msgType == "ping_peer") ?
      ar_graphicsPeerHandlePingPeer(peer, msgBody) :
    (msgType =="remote_lock_node") ?
      ar_graphicsPeerHandleRemoteLockNode(peer, msgBody, true) :
    (msgType =="remote_lock_node_below") ?
      ar_graphicsPeerHandleRemoteLockNodeBelow(peer, msgBody, true) :
    (msgType == "remote_unlock_node") ?
      ar_graphicsPeerHandleRemoteLockNode(peer, msgBody, false) :
    (msgType == "remote_unlock_node_below") ?
      ar_graphicsPeerHandleRemoteLockNodeBelow(peer, msgBody, false) :
    (msgType == "local_lock_node") ?
      ar_graphicsPeerHandleLocalLockNode(peer, msgBody, true) :
    (msgType == "local_unlock_node") ?
      ar_graphicsPeerHandleLocalLockNode(peer, msgBody, false) :
    (msgType == "remote_filter_data_below") ?
      ar_graphicsPeerHandleRemoteFilterDataBelow(peer, msgBody) :
    (msgType == "local_filter_data_below") ?
      ar_graphicsPeerHandleLocalFilterDataBelow(peer, msgBody) :
    (msgType == "get_node_id") ?
      ar_graphicsPeerHandleGetNodeID(peer, msgBody) :
    (msgType == "remote_node_id") ?
      ar_graphicsPeerHandleRemoteNodeID(peer, msgBody) :
    (msgType == "read_database") ?
      ar_graphicsPeerHandleReadDatabase(peer, msgBody, false) :
    (msgType == "read_database_xml") ?
      ar_graphicsPeerHandleReadDatabase(peer, msgBody, true) :
    (msgType == "write_database") ?
      ar_graphicsPeerHandleWriteDatabase(peer, msgBody, false) :
    (msgType == "write_database_xml") ?
      ar_graphicsPeerHandleWriteDatabase(peer, msgBody, true) :
    (msgType == "write_rooted") ?
      ar_graphicsPeerHandleWriteRooted(peer, msgBody, false) :
    (msgType == "write_rooted_xml") ?
      ar_graphicsPeerHandleWriteRooted(peer, msgBody, true) :
    (msgType == "attach") ?
      ar_graphicsPeerHandleAttach(peer, msgBody, false) :
    (msgType == "attach_xml") ?
      ar_graphicsPeerHandleAttach(peer, msgBody, true) :
    (msgType == "merge") ?
      ar_graphicsPeerHandleMerge(peer, msgBody, false) :
    (msgType == "merge_xml") ?
      ar_graphicsPeerHandleMerge(peer, msgBody, true) :
    (msgType == "print_connections") ?
      ar_graphicsPeerHandlePrintConnections(peer, msgBody) :
    (msgType == "print_peer") ?
      ar_graphicsPeerHandlePrintPeer(peer, msgBody) :
      "szg-rp error: unknown message type '" + msgType + "'.\n";
}
