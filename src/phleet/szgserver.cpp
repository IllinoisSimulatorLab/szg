//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#define SZG_DO_NOT_EXPORT

#include "arDataServer.h"
#include "arLogStream.h"
#include "arPhleetConfig.h"
#include "arPhleetConnectionBroker.h"
#include "arPhleetOSLanguage.h"
#include "arUDPSocket.h"

#include <stdio.h>
using namespace std;

// a parser that can manage storage for that dictionary
arPhleetOSLanguage       lang;
arStructuredDataParser*  dataParser = NULL;
arDataServer*            dataServer = NULL;
arPhleetConnectionBroker connectionBroker;

// TCP-wrappers style filtering on the incoming IPs.
list<string> serverAcceptMask;

// Addresses of this host's NICs, for "discovery" (dhunt).
arSlashString computerAddresses;
arSlashString computerMasks;

string serverName;
string serverIP;
int    serverPort = -1;

//*******************************************
// Global data storage.  "DB" means "Database".
//*******************************************

typedef map<string,string,less<string> > SZGparamDB;
typedef SZGparamDB::iterator iterParam;
typedef SZGparamDB::const_iterator const_iterParam;

// Contains database values particular to szgserver, i.e. the pseudoDNS.
SZGparamDB rootContainer;

// One parameter database per dlogin'd user.
typedef map<string,SZGparamDB*,less<string> > SZGuserDB;
SZGuserDB userDB;

// The current parameter database
SZGparamDB* valueContainer = NULL;

// message IDs start at 1
int nextMessageID = 1;

class arPhleetMsg{
 public:
  arPhleetMsg() :
    id(-1),
    idOwner(-1),
    idDestination(-1),
    idMatch(-1),
    idTradingMatch(-1)
    {};
  arPhleetMsg(int a, int b, int c, int d) :
    id(a),
    idOwner(b),
    idDestination(c),
    idMatch(d),
    idTradingMatch(-1)
    {};

  int id;
  int idOwner; // owner is the component to which msg was originally directed,
               // or possibly another if there was a message trade) 
  int idDestination; // response's destination
  int idMatch;       // "match" for the response
  int idTradingMatch;  // "match" for a trade in progress
};

// DOH!!! The next section of STL shows VERY BAD DATA STRUCTURE DESIGN!!

// Map message ID to message's info.
typedef map<int,arPhleetMsg,less<int> > SZGmsgOwnershipDB;
SZGmsgOwnershipDB messageOwnershipDB;

// Map message key to message's info.
typedef map<string,arPhleetMsg,less<string> > SZGmsgTradingDB;
SZGmsgTradingDB messageTradingDB;

// when a connection goes away, we need to be able to respond to all messages
// owned by that particular component (but to which a response has not yet
// been posted)... i.e. by sending an "error message"
typedef map<int,list<int>,less<int> > SZGcomponentMessageOwnershipDB;
SZGcomponentMessageOwnershipDB componentMessageOwnershipDB;

// When a connection goes away, remove all the "message trades"
// initiated by it, and send error responses to the original senders.
typedef map<int,list<string>,less<int> > SZGcomponentTradingOwnershipDB;
SZGcomponentTradingOwnershipDB componentTradingOwnershipDB;

// Map named lock to lock's holder's component-ID.
typedef map<string,int,less<string> > SZGlockOwnershipDB;
SZGlockOwnershipDB lockOwnershipDB;

// When a connection goes away, release all its locks.
typedef map<int,list<string>,less<int> > SZGcomponentLockOwnershipDB;
SZGcomponentLockOwnershipDB componentLockOwnershipDB;

// When a lock is released, notify other components.
typedef map<string,list<arPhleetNotification>,less<string> > SZGlockNotificationDB;
SZGlockNotificationDB lockNotificationDB;

// Lock-release notifications owned by a particular component.
typedef map<int,list<string>,less<int> > SZGlockNotificationOwnershipDB;
SZGlockNotificationOwnershipDB lockNotificationOwnershipDB;

// When component goes away, notify other components.
typedef map<int,list<arPhleetNotification>,less<int> > SZGkillNotificationDB;
SZGkillNotificationDB killNotificationDB;

// Kill notifications owned by a particular component.
// The list<int> is IDs of component we're observing to see if they vanish.
typedef map<int,list<int>,less<int> > SZGkillNotificationOwnershipDB;
SZGkillNotificationOwnershipDB killNotificationOwnershipDB;

//**************************************************
// end of global data storage
//**************************************************


//**************************************************
// utility functions
//**************************************************

void _transferMatchFromTo(arStructuredData* from, arStructuredData* to) {
  const int match = from->getDataInt(lang.AR_PHLEET_MATCH);
  to->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
}

void SZGactivateUser(const string& userName) {
  // normal user
  SZGuserDB::const_iterator i = userDB.find(userName);
  if (i != userDB.end()) {
    valueContainer = i->second;
  }
  else {
    // add a new parameter database for this user
    SZGparamDB* newDB = new SZGparamDB;
    userDB.insert(SZGuserDB::value_type(userName,newDB));
    valueContainer = newDB;
  }
}

//********************************************************************
// functions manipulating the message databases
//********************************************************************

// A helper function for the larger message database manipulators.
// This inserts a given message ID into the list maintained for a given
// component (which is the list of message IDs to which the component is
// expected to respond).
void SZGinsertMessageIDIntoComponentsList(int componentID, int messageID) {
  SZGcomponentMessageOwnershipDB::iterator
    i(componentMessageOwnershipDB.find(componentID));
  if (i == componentMessageOwnershipDB.end()) {
    // the component owns no messages currently
    list<int> messageList;
    messageList.push_back(messageID);
    componentMessageOwnershipDB.insert
      (SZGcomponentMessageOwnershipDB::value_type(componentID,
							messageList));
  }
  else {
    i->second.push_back(messageID);
  }
}

// Helper for the larger message database manipulators.
// Removes the given message ID from the list maintained for the
// given component (which is the list of message IDs to which the component
// is expected to respond).
void SZGremoveMessageIDFromComponentsList(int componentID, int messageID) {
  SZGcomponentMessageOwnershipDB::iterator
    i(componentMessageOwnershipDB.find(componentID));
  if (i == componentMessageOwnershipDB.end()) {
    ar_log_error() << "internal error: no message list for component "
         << componentID << ".\n";
  }
  else {
    i->second.remove(messageID);
  }
}

// A helper function for the larger message database manipulators.
// Adds the given key to the list maintained for the given component
// (which is the list of keys on which the component has initiated trades)
void SZGinsertKeyIntoComponentsList(int componentID, const string& key) {
  SZGcomponentTradingOwnershipDB::iterator
    i(componentTradingOwnershipDB.find(componentID));
  if (i == componentTradingOwnershipDB.end()) {
    // the component has posted no trades as of yet
    list<string> tradeList;
    tradeList.push_back(key);
    componentTradingOwnershipDB.insert
      (SZGcomponentTradingOwnershipDB::value_type(componentID,
							tradeList));
  }
  else {
    i->second.push_back(key);
  }
}

// Helper function for the message database manipulators.
// @param componentID A component
// @param key Key to be removed from the list of keys on which the component has initiated trades
void SZGremoveKeyFromComponentsList(int componentID, const string& key) {
  SZGcomponentTradingOwnershipDB::iterator
    i(componentTradingOwnershipDB.find(componentID));
  if (i == componentTradingOwnershipDB.end()) {
    ar_log_error() << "internal error: no trading key in component's list.\n";
  }
  else {
    i->second.remove(key);
  }
}

// Add a message to the internal database storage.
// @param messageID ID of the message
// @param componentOwnerID ID of the component which has the right
// to respond to this message
// @param componentOriginatorID ID of the component which sent the
// message
void SZGaddMessageToDB(const int messageID, 
                       const int componentOwnerID,
                       const int componentOriginatorID,
                       const int match) {
  arPhleetMsg message(messageID, componentOwnerID, componentOriginatorID, match);
  messageOwnershipDB.insert(SZGmsgOwnershipDB::value_type
				  (messageID, message));
  // Add to the component's list of owned messages.
  SZGinsertMessageIDIntoComponentsList(componentOwnerID, messageID);
}

// Return the ID of the component that owns this message
int SZGgetMessageOwnerID(const int messageID) {
  SZGmsgOwnershipDB::const_iterator i(messageOwnershipDB.find(messageID));
  return (i == messageOwnershipDB.end()) ? -1 : i->second.idOwner;
}

// Return the match corresponding to the original message. Helpful when
// we are going to send a response back to the originator. It that case,
// we need to fill in the original match so that the async stuff on the
// arSZGClient side can route the messages correctly.
int SZGgetMessageMatch(const int messageID) {
  SZGmsgOwnershipDB::const_iterator i(messageOwnershipDB.find(messageID));
  return (i == messageOwnershipDB.end()) ? -1 : i->second.idMatch;
} 

// Return the ID of the component that originated this message
// (and to which the response needs to be sent).
int SZGgetMessageOriginatorID(const int messageID) {
  SZGmsgOwnershipDB::const_iterator i(messageOwnershipDB.find(messageID));
  return (i == messageOwnershipDB.end()) ? -1 : i->second.idDestination;
}

// Remove the message with the given ID from the database.
// Since a message is entered in the database only when the sender requests
// a response, this function is used when
// a response to a message is received at the szgserver.
// @param messageID ID of the message
bool SZGremoveMessageFromDB(const int messageID) {
  SZGmsgOwnershipDB::iterator i(messageOwnershipDB.find(messageID));
  if (i == messageOwnershipDB.end()) {
    ar_log_error() << "ignoring request to remove missing message.\n";
    return false;
  }

  const int idOwner = i->second.idOwner;
  messageOwnershipDB.erase(i);
  SZGremoveMessageIDFromComponentsList(idOwner, messageID);
  return true;
}

// Initiate an "ownership trade",
// when szgd wants its launchee to respond to the dex command that launched it.
//
// @param key Value by which the trade is indexed
// @param messageID ID of the message
// @param requestingComponentID ID of the component requesting that
// the trade be posted
// @param tradingMatch this is used to respond to the component that
// initiated the trade when said trade has been completed.
bool SZGaddMessageTradeToDB(const string& key,
                            const int messageID,
                            const int requestingComponentID,
			    const int tradingMatch) {
  SZGmsgOwnershipDB::iterator i(messageOwnershipDB.find(messageID));
  if (i == messageOwnershipDB.end()) {
    ar_log_error() << "ignoring trade on messageless ID.\n";
    return false;
  }
  const int responseOwner = i->second.idOwner;
  if (responseOwner != requestingComponentID) {
    ar_log_error() << "ignoring trade asked by non-owning component.\n";
    return false;
  }
  SZGmsgTradingDB::iterator j(messageTradingDB.find(key));
  if (j != messageTradingDB.end()) {
    // a trade has already been posted with this key... failure
    ar_log_error() << "ignoring duplicate trade on key " << key << ".\n";
    return false;
  }

  // Start the trade.
  i->second.idTradingMatch = tradingMatch;
  messageTradingDB.insert(SZGmsgTradingDB::value_type(key, i->second));

  // Remove the corresponding data from the message ownership database.
  messageOwnershipDB.erase(i);

  // Remove the corresponding data from the component's list.
  SZGremoveMessageIDFromComponentsList(responseOwner, messageID);

  // Add the trading key to the component's list.
  SZGinsertKeyIntoComponentsList(responseOwner, key);

  return true;
}

// Complete a message ownership trade.  Return false on error.
// @param key Value on which the trade was posted
// @param newOwnerID ID of the component which will henceforth own
// the right to respond to the message
// @param message Gives the various attributes of the phleet message
bool SZGmessageRequest(const string& key, int newOwnerID,
                       arPhleetMsg& message) {
  SZGmsgTradingDB::iterator j(messageTradingDB.find(key));
  if (j == messageTradingDB.end()) {
    // No trade has been posted on this key.
    ar_log_error() << "ignoring trade on missing key " << key << ".\n";
    return false;
  }

  // stuff messageData
  message = j->second;
  // get the old owner before overwriting
  int oldOwner = message.idOwner;
  // the change is that someone else owns the message now.
  message.idOwner = newOwnerID;
  // remove from the trading database
  messageTradingDB.erase(j);
  // make the component requesting the trade the new message owner
  messageOwnershipDB.insert(SZGmsgOwnershipDB::value_type
    (message.id,message));

  // remove the key from the list associated with the trading component
  // message.idOwner used to be where oldOwner is now
  SZGremoveKeyFromComponentsList(oldOwner, key);

  // add the messageID to the list associated with the new owner
  SZGinsertMessageIDIntoComponentsList(newOwnerID, message.id);
  return true;
}

// Revoke a message trade that has yet to be completed, e.g.
// when szgd fails to launch an executable, in which
// case szgd wants to respond directly to the "dex" command.
// @param key Value on which the message trade was posted
// @param revokerID ID of the component that is requesting the trade revocation
bool SZGrevokeMessageTrade(const string& key, int revokerID) {
  SZGmsgTradingDB::iterator j(messageTradingDB.find(key));
  // is there a message with this key?
  if (j == messageTradingDB.end()) {
    ar_log_error() << "failed to revoke message trade: no key '" << key <<"'.\n";
    return false;
  }

  arPhleetMsg message = j->second;
  if (message.idOwner != revokerID) {
    ar_log_error() << "component unauthorized to revoke message trade.\n";
    return false;
  }

  // remove the trade from the database
  messageTradingDB.erase(j);
  // restore the original message owenership
  messageOwnershipDB.insert(
    SZGmsgOwnershipDB::value_type(message.id, message));
  // enter the message ID back into the ownership list of the component
  SZGinsertMessageIDIntoComponentsList(message.idOwner, message.id);
  // remove the key from the trading list associated with this component
  SZGremoveKeyFromComponentsList(message.idOwner, key);
  return true;
}

// Get message info from the trading database.
bool SZGgetMessageTradeInfo(const string& key, arPhleetMsg& message) {
  SZGmsgTradingDB::const_iterator j(messageTradingDB.find(key));
  if (j == messageTradingDB.end()) {
    return false;
  }

  // Found a message with this key. Return that key's trading info.
  message = j->second;
  return true;
}

//********************************************************************
// funtions dealing with the locks
//********************************************************************

// Request a named lock.
// Returns true iff no errors occur (the lock is granted).
// @param lockName Name of the lock.
// @param id ID of component requesting the lock
// @param ownerID Set to -1 if the lock was not previously held,
// otherwise set to the ID of the holding component.
bool SZGgetLock(const string& lockName, int id, int& ownerID) {
  SZGlockOwnershipDB::const_iterator i(lockOwnershipDB.find(lockName));
  if (i != lockOwnershipDB.end()) {
    ownerID = i->second;
    // Don't complain, because this is common.
    return false;
  }

  // Nobody holds the lock. Insert it in the global list.
  lockOwnershipDB.insert(SZGlockOwnershipDB::value_type(lockName, id));
  // Insert the name in the list associated with this component.
  SZGcomponentLockOwnershipDB::iterator j(componentLockOwnershipDB.find(id));
  if (j == componentLockOwnershipDB.end()) {
    // The component has never owned a lock.
    list<string> lockList;
    lockList.push_back(lockName);
    componentLockOwnershipDB.insert
      (SZGcomponentLockOwnershipDB::value_type(id, lockList));
  }
  else {
    // The component either owns, or has owned, a lock.
    j->second.push_back(lockName);
  }
  ownerID = -1;
  return true;
}

// Return true iff the lock is held.
bool SZGcheckLock(const string& lockName) {
  return lockOwnershipDB.find(lockName) != lockOwnershipDB.end();
}

// Enters a request for notification when a given lock, currently held,
// is released.
// BUG BUG BUG BUG BUG BUG BUG: If a given component asks for multiple
//  notifications on the same lock name, then it will receive only the
//  first!
void SZGrequestLockNotification(int id, const string& lockName, int match) {
  arPhleetNotification notification(id, match);
  // Add to list keyed by lock name.
  SZGlockNotificationDB::iterator i(lockNotificationDB.find(lockName));
  if (i == lockNotificationDB.end()) {
    // no notifications, yet, for this lock's release
    list<arPhleetNotification> temp;
    temp.push_back(notification);
    lockNotificationDB.insert(SZGlockNotificationDB::value_type
				    (lockName, temp));
  }
  else {
    // there are already notifications requested for this lock
    i->second.push_back(notification);
  }
  // we must also enter it into the list organized by component ID
  SZGlockNotificationOwnershipDB::iterator j
    = lockNotificationOwnershipDB.find(id);
  if (j == lockNotificationOwnershipDB.end()) {
    // no notifications directed at this component
    list<string> tempS;
    tempS.push_back(lockName);
    lockNotificationOwnershipDB.insert
      (SZGlockNotificationOwnershipDB::value_type(id, tempS));
  }
  else {
    // there are already notifications directed towards this component
    j->second.push_back(lockName);
  }
}

// Sends the lock release notifications, if any. NOTE: this is ALWAYS called
// upon lock release. Because of some technicalities about the way we use
// call, we need to be able to use the data server's regular methods 
// (in the case of serverLock = false) or data server's "no lock" methods
// (in the case of serverLock = true).
void SZGsendLockNotification(string lockName, bool serverLock) {
  SZGlockNotificationDB::iterator i
    = lockNotificationDB.find(lockName);
  if (i != lockNotificationDB.end()) {
    // there are actually some notifications
    arStructuredData* data 
      = dataParser->getStorage(lang.AR_SZG_LOCK_NOTIFICATION);
    data->dataInString(lang.AR_SZG_LOCK_NOTIFICATION_NAME, lockName);
    for (list<arPhleetNotification>::iterator j = i->second.begin();
	 j != i->second.end(); j++) {
      // Must set the match.
      data->dataIn(lang.AR_PHLEET_MATCH, &j->match, AR_INT, 1);
      arSocket* theSocket = NULL;
      // we use too different calls, sendData and sendDataNoLock,
      // depending upon the context in which we were called.
      if (serverLock) {
        theSocket = dataServer->getConnectedSocketNoLock(j->componentID);
      }
      else {
        theSocket = dataServer->getConnectedSocket(j->componentID);
      }
      if (!theSocket) {
	ar_log_error() << "can't send lock notification for missing component.\n";
      }
      else {
	// we use too different calls, sendData and sendDataNoLock,
	// depending upon the context in which we were called.
	if (serverLock) {
          if (!dataServer->sendDataNoLock(data, theSocket)) {
	    ar_log_error() << "failed to send no-lock notification.\n";
	  }
	}
	else {
          if (!dataServer->sendData(data, theSocket)) {
	    ar_log_error() << "failed to send lock notification.\n";
	  }
	}
      }
      // now, we need to do a little clean-up...
      // THIS IS VERY, VERY INEFFICIENT. TODO TODO TODO TODO TODO TODO TODO
      SZGlockNotificationOwnershipDB::iterator k
	= lockNotificationOwnershipDB.find(j->componentID);
      if (k != lockNotificationOwnershipDB.end()) {
        k->second.remove(lockName);
	// if the list is empty, better remove it.
        if (k->second.empty()) {
          lockNotificationOwnershipDB.erase(k);
	}
      }
      else {
	ar_log_error() << "found no expected lock notification owner.\n";
      }
    }
    // Must remember to recycle the storage!
    dataParser->recycle(data);
    // finally, the lock has gone away, so remove its entry
    lockNotificationDB.erase(i);
  }
}

// A component is going away. Remove any outstanding lock notification
// requests from internal storage.
void SZGremoveComponentLockNotifications(int componentID) {
  SZGlockNotificationOwnershipDB::iterator i
    = lockNotificationOwnershipDB.find(componentID);
  if (i != lockNotificationOwnershipDB.end()) {
    // this component has some notifications
    for (list<string>::iterator j = i->second.begin();
	 j != i->second.end(); j++) {
      // WOEFULLY INEFFICIENT... TODO TODO TODO TODO TODO TODO
      SZGlockNotificationDB::iterator k
	= lockNotificationDB.find(*j);
      if (k != lockNotificationDB.end()) {
	// AARGH! Binding the component ID and the match together makes
	// this step more inefficient!
        list<arPhleetNotification>::iterator l = k->second.begin();
	while (l != k->second.end()) {
          if (l->componentID == componentID) {
            l = k->second.erase(l);
	  }
	  else {
	    l++;
	  }
	}
	// if the list is empty, better remove it!
	if (k->second.empty()) {
	  lockNotificationDB.erase(k);
	}
      }
      else {
	ar_log_error() << "found no lock notification, needed by component list.\n";
      }
    }
    // finally, the component has gone away... so remove its info
    lockNotificationOwnershipDB.erase(i);
  }
}

// Release a named lock.
// Returns false on error (component doesn't own the lock).
// NOTE: this is only called when a component explcitly requests
// a lock be released. Consequently, we call a version of
// SZGsendLockNotification(...) that uses the normal methods of arDataServer
// (i.e. NOT the "no lock" methods).
// @param lockName Name of the lock.
// @param id ID of component releasing the lock
bool SZGreleaseLock(const string& lockName, int id) {
  SZGlockOwnershipDB::iterator
    i(lockOwnershipDB.find(lockName));
  if (i == lockOwnershipDB.end()) {
    // Lock is not held.
    ar_log_error() << "already released lock "
	 << lockName << " as requested by component" << id
	 << " (" << dataServer->getSocketLabel(id) << ").\n";
    return false;
  }

  if (i->second != id) {
    // Lock is held by another component
    // component requesting the release doesn't own the lock
    ar_log_error() << "failed to release lock "
	 << lockName << ": not held by component" << id
	 << " (" << dataServer->getSocketLabel(id) << ").\n";
    return false;
  }

  // Remove the lock from the database.
  lockOwnershipDB.erase(i);

  // Remove the lock from the component's database.
  SZGcomponentLockOwnershipDB::iterator
    j(componentLockOwnershipDB.find(id));
  if (j == componentLockOwnershipDB.end()) {
    ar_log_error() << "internal error: lock list missing on release.\n";
    return false;
  }

  j->second.remove(lockName);
  // if we've gotten this far, the lock has indeed been released.
  SZGsendLockNotification(lockName, false);
  return true;
}

// Release all locks held by one component. NOTE: this method is ONLY
// called when a component is removed from the database. Hence, we
// call a version of SZGsendLockNotification(...) that uses the data
// server's "no lock" methods.
// @param id ID of component releasing the locks
void SZGreleaseLocksOwnedByComponent(int id) {
  // (Too late to call dataServer->getSocketLabel(id).)
  // Get the lockList.
  SZGcomponentLockOwnershipDB::iterator
    i(componentLockOwnershipDB.find(id));
  if (i == componentLockOwnershipDB.end()) {
    return;
  }
  for (list<string>::iterator k=i->second.begin(); k != i->second.end(); k++) {
    const string lockName(*k);
    // erase it from the global storage
    SZGlockOwnershipDB::iterator j=lockOwnershipDB.find(lockName);
    if (j == lockOwnershipDB.end()) {
      ar_log_error() << "internal error: no lock name to remove on component shutdown.\n";
    }
    else {
      SZGsendLockNotification(lockName, true);
      lockOwnershipDB.erase(j);
    }
  }
  // erase the component's lock list
  componentLockOwnershipDB.erase(i);
}

//********************************************************************
// Functions dealing with kill notifications.
// NOTE: THESE FUNCTIONS ARE ESSENTIALLY THE SAME AS THE LOCK RELEASE
// NOTIFICATIONS!
//********************************************************************

// Enters a request for notification when a particular component goes
// away. This is useful for efficient notification of a kill's success.
void SZGrequestKillNotification(int requestingComponentID, 
                                int observedComponentID,
                                int match) {
  arPhleetNotification notification(requestingComponentID, match);
  // Add to list keyed by ID, i.e. the component for whose demise we are waiting.
  SZGkillNotificationDB::iterator i(
    killNotificationDB.find(observedComponentID));
  if (i == killNotificationDB.end()) {
    // no notifications, yet, for this component's demise
    list<arPhleetNotification> temp;
    temp.push_back(notification);
    killNotificationDB.insert(SZGkillNotificationDB::value_type
				    (observedComponentID, temp));
  }
  else {
    // there are already notifications requested for this component's demise
    i->second.push_back(notification);
  }
  // we must also enter it into the list organized by the REQUESTING
  // component's ID.
  SZGkillNotificationOwnershipDB::iterator j
    = killNotificationOwnershipDB.find(requestingComponentID);
  if (j == killNotificationOwnershipDB.end()) {
    // no kill notifications directed at this component yet
    list<int> tempI;
    // We need to know how to remove the notification request from the
    // OBSERVED component's database if the REQUESTING component goes away
    // before the observed component does.
    tempI.push_back(observedComponentID);
    killNotificationOwnershipDB.insert
      (SZGkillNotificationOwnershipDB::value_type
        (requestingComponentID, tempI));
  }
  else {
    // there are already notifications directed towards this REQUESTING 
    // component. REMEMBER: we want to be able to refer back to the 
    // OBSERVED component (which is index by which we store the notification
    // requests.
    j->second.push_back(observedComponentID);
  }
}

// Sends the kill release notifications, if any. This is ALWAYS called when
// the component exits. Because of some technicalities about the way we use
// call, we need to be able to use the data server's regular methods 
// (in the case of serverLock = false) or data server's "no lock" methods
// (in the case of serverLock = true).
void SZGsendKillNotification(int observedComponentID, bool serverLock) {
  SZGkillNotificationDB::iterator i
    = killNotificationDB.find(observedComponentID);
  if (i != killNotificationDB.end()) {
    // there are actually some notifications
    arStructuredData* data 
      = dataParser->getStorage(lang.AR_SZG_KILL_NOTIFICATION);
    data->dataIn(lang.AR_SZG_KILL_NOTIFICATION_ID, &observedComponentID,
		 AR_INT, 1);
    // This component has a list of other components that wish to be
    // notified when it exits.
    for (list<arPhleetNotification>::iterator j = i->second.begin();
	 j != i->second.end(); j++) {
      // Must set the match.
      data->dataIn(lang.AR_PHLEET_MATCH, &j->match, AR_INT, 1);
      arSocket* theSocket = NULL;
      // we use too different calls, sendData and sendDataNoLock,
      // depending upon the context in which we were called.
      // NOTE: the componentID held by the arPhleetNotification is the
      // ID of the REQUESTING component.
      if (serverLock) {
        theSocket = dataServer->getConnectedSocketNoLock(j->componentID);
      }
      else {
        theSocket = dataServer->getConnectedSocket(j->componentID);
      }
      if (!theSocket) {
	ar_log_error() << "can't send kill notification to missing component.\n";
      }
      else {
	// we use too different calls, sendData and sendDataNoLock,
	// depending upon the context in which we were called.
	if (serverLock) {
          if (!dataServer->sendDataNoLock(data, theSocket)) {
	    ar_log_error() << "failed to send no-lock kill notification.\n";
	  }
	}
	else {
          if (!dataServer->sendData(data, theSocket)) {
	    ar_log_error() << "failed to send kill notification.\n";
	  }
	}
      }
      // now, we need to do a little clean-up...
      // THIS IS VERY, VERY INEFFICIENT. TODO TODO TODO TODO TODO TODO TODO
      // NOTE: this notification database is organized via REQUESTING
      // component ID (and j->componentID is REQUESTING COMPONENT ID)
      SZGkillNotificationOwnershipDB::iterator k
	= killNotificationOwnershipDB.find(j->componentID);
      if (k != killNotificationOwnershipDB.end()) {
	// The lists in the killNotificationOwnershipDB are
	// of OBSERVED component IDs (i.e. we want to be notified when this
	// goes away)
        k->second.remove(observedComponentID);
	// If this component has no more (other) components whose potential
	// demise it is observing, remove the list.
        if (k->second.empty()) {
          killNotificationOwnershipDB.erase(k);
	}
      }
      else {
	ar_log_error() << "no expected kill notification owner.\n";
      }
    }
    dataParser->recycle(data);
    // finally, we've sent all the kill notifications, so remove the entry.
    killNotificationDB.erase(i);
  }
}

// A component is going away. Remove from internal storage any outstanding kill notification
// requests that it owns.
void SZGremoveComponentKillNotifications(int requestingComponentID) {
  SZGkillNotificationOwnershipDB::iterator i =
    killNotificationOwnershipDB.find(requestingComponentID);
  if (i != killNotificationOwnershipDB.end()) {
    // this component has some kill notifications.
    // traverse its list (which contains the IDs of the components it is OBSERVING).
    for (list<int>::iterator j = i->second.begin();
	 j != i->second.end(); j++) {
      // WOEFULLY INEFFICIENT... TODO TODO TODO TODO TODO TODO
      // Find the observed component and look at its list of
      // registered kill notification requests.
      SZGkillNotificationDB::iterator k = killNotificationDB.find(*j);
      if (k != killNotificationDB.end()) {
	// Remove every notification from the list which is related to the
	// requestingComponentID.
        list<arPhleetNotification>::iterator l = k->second.begin();
	while (l != k->second.end()) {
	  // arPhleetNotification's componentID is the 
	  // ID of the component REQUESTING the notification!
          if (l->componentID == requestingComponentID) {
            l = k->second.erase(l);
	  }
	  else {
	    ++l;
	  }
	}
	if (k->second.empty()) {
	  // Remove the empty list.
	  killNotificationDB.erase(k);
	}
      }
      else {
	ar_log_error() << "no kill notification, needed by component list.\n";
      }
    }
    // finally, the component has gone away... so remove its info
    killNotificationOwnershipDB.erase(i);
  }
}


//********************************************************************
// functions dealing with component management
//********************************************************************

// The connection broker uses this callback to send the notifications of
// service release to clients that have requested such. NOTE: this is OK
// ONLY because it is called (indirectly) from SZGremoveComponentFromDB
// (THIS IS RELATED TO THE CONNECTION BROKER... AND EXISTS IN CALLBACK
//  FORM BECAUSE THE CONNECTION BROKER CANNOT ITSELF DO EVERYTHING THAT IS
//  NECESSARY TO RELEASE A COMPONENT WHEN IT GOES AWAY)
void SZGreleaseNotificationCallback(int componentID,
				    int match,
                                    const string& serviceName) {
  // since this is called from within the data server's lock, we have
  // to use the no-lock methods
  arSocket* destinationSocket 
    = dataServer->getConnectedSocketNoLock(componentID);
  if (destinationSocket) {
    arStructuredData* data 
      = dataParser->getStorage(lang.AR_SZG_SERVICE_RELEASE);
    // Must propogate the match!
    data->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
    data->dataInString(lang.AR_SZG_SERVICE_RELEASE_NAME, serviceName);
    if (!dataServer->sendDataNoLock(data, destinationSocket)) {
      ar_log_error() << "failed to send to specified socket in release notifcation.\n";
    }
  }
}

// Clean up when a socket is removed from the database.  If a component dies
// before replying to a message, szgserver itself must reply (with
// SZG_FAILURE).  Also clean up message trades, locks, and services offered.
// @param componentID ID of component which should have sent replies
void SZGremoveComponentFromDB(const int componentID) {
  // Don't sendDataNoLock, because we're called from 
  // the automatic remove socket from data server call, already in the mutex.

  // Construct the failure message.
  int messageID = -1;
  int responseDest = -1;
  arStructuredData* messageAdminData = dataParser->getStorage(lang.AR_SZG_MESSAGE_ADMIN);
  (void)messageAdminData->dataIn(lang.AR_SZG_MESSAGE_ADMIN_ID, &messageID, AR_INT, 1);
  (void)messageAdminData->dataInString(lang.AR_SZG_MESSAGE_ADMIN_STATUS, "SZG_FAILURE");
  (void)messageAdminData->dataInString(lang.AR_SZG_MESSAGE_ADMIN_TYPE, "SZG Response");
  (void)messageAdminData->dataInString(lang.AR_SZG_MESSAGE_ADMIN_BODY, "");

  // send "failure" messages to all components expecting
  // a message response from this component
  const SZGcomponentMessageOwnershipDB::iterator i =
    componentMessageOwnershipDB.find(componentID);
  // The componentMessageOwnershipDB gives the messages owned by
  // a particular component.
  if (i != componentMessageOwnershipDB.end()) {
    for (list<int>::iterator k=i->second.begin(); k!=i->second.end(); k++) {
      // go through the list, sending failure messages.
      messageID = *k;
      // find the component that originated the message
      const SZGmsgOwnershipDB::iterator j =
        messageOwnershipDB.find(messageID);
      if (j == messageOwnershipDB.end()) {
        ar_log_error() << "found no message ID info in socket clean-up.\n";
      }
      else {
        messageAdminData->dataIn(lang.AR_SZG_MESSAGE_ADMIN_ID, &messageID, 
                                 AR_INT, 1);
	// Must propogate the match!
        messageAdminData->dataIn(lang.AR_PHLEET_MATCH, &j->second.idMatch,
                                 AR_INT, 1);
	// the first element of the pair is the owner of the response
	// the second element of the pair is the destination of our message
        responseDest = j->second.idDestination;
        messageOwnershipDB.erase(j);
        // The response destination may have vanished.
        arSocket* destinationSocket =
          dataServer->getConnectedSocketNoLock(responseDest);
        if (!destinationSocket) {
	  ar_log_error() << "destination vanished for message ID "
	       << messageID << ".\n";
	}
	else {
          dataServer->sendDataNoLock(messageAdminData, destinationSocket);
	}
      }
    }
    componentMessageOwnershipDB.erase(i);
  }

  // Remove all the message trades owned by this component,
  // again sending failure messages to the originating components.
  const SZGcomponentTradingOwnershipDB::iterator l =
    componentTradingOwnershipDB.find(componentID);
  if (l != componentTradingOwnershipDB.end()) {
    for (list<string>::iterator n=l->second.begin(); n!=l->second.end(); n++) {
      const string messageKey = *n;
      // we now need to find the component that sent the message upon
      // which the trade is posted
      const SZGmsgTradingDB::iterator m =
        messageTradingDB.find(messageKey);
      if (m == messageTradingDB.end()) {
        ar_log_error() << "no key info in socket cleanup.\n";
      }
      else {
        messageID = m->second.id;
        responseDest = m->second.idDestination;
	// Must make sure that the match is propogated.
	messageAdminData->dataIn(lang.AR_PHLEET_MATCH,
				 &m->second.idMatch, AR_INT, 1);
        messageAdminData->dataIn(lang.AR_SZG_MESSAGE_ADMIN_ID, &messageID, 
                                 AR_INT, 1);
        messageTradingDB.erase(m);
        // NOTE: we cannot assume that the response destination still exists!
        arSocket* originatingSocket =
          dataServer->getConnectedSocketNoLock(responseDest);
        if (!originatingSocket) {
	  ar_log_error() << "on socket cleanup, destination vanished for message ID "
	    << messageID << ".\n";
	}
	else {
          dataServer->sendDataNoLock(messageAdminData, originatingSocket);
	}
      }
    }
    componentTradingOwnershipDB.erase(l);
  }
  dataParser->recycle(messageAdminData);
  // nuke the connection brokering
  connectionBroker.removeComponent(componentID);
  // Deal with the lock-related stuff, both removing the locks
  // this component owns (and, from in there, sending messages to the
  // components that desire notification on lock release) and in 
  SZGreleaseLocksOwnedByComponent(componentID);
  SZGremoveComponentLockNotifications(componentID);
  // Remove any kill notifications owned by this component.
  SZGremoveComponentKillNotifications(componentID);
  // Finally, send any kill notifcations (regarding this component) that
  // other components have requested. NOTE: we use the NO_LOCK version
  // since we are inside the arDataServer's lock (this is called from
  // the disconnect callback of the arDataServer).
  SZGsendKillNotification(componentID, true);
}

//********************************************************************
// Misc functions
//********************************************************************

/*
This thread listens on port 4620 for discovery requests (as via dhunt
or dconnect) and responds with a broadcast "I am here" packet, also on
port 4620. An infinite loop is avoided with a flag indicating
whether this is a discovery request or a response.

Discovery packet (200 bytes):
  bytes 0-3: Version number, to reject incompatible packets.
  byte 4: 0 for discovery, 1 for response.
  bytes 4-131: The requested server name, NULL-terminated string.
  bytes 132-199: All 0's

Response packet (200 bytes):
  bytes 0-3: Version number, to reject incompatible packets.
  byte 4: 0 for discovery, 1 for response.
  bytes 5-131: Our name, NULL-terminated string.
  bytes 132-163: The interface upon which the remote whatnot should
    connect, NULL-terminated string.
  bytes 164-199: The port upon which the remote whatnot should connect,
    NULL-terminated string. (yes, this is way more space than necessary).

  If *pv is set to false, szgserver aborts.
*/

void serverDiscoveryFunction(void* pv) {
  char buffer[200];
  ar_stringToBuffer(serverIP, buffer, sizeof(buffer));
  arSocketAddress incomingAddress;
  const int port = 4620;
  incomingAddress.setAddress(NULL, port);
  arUDPSocket _socket;
  _socket.ar_create();
  _socket.setBroadcast(true);
  // Allows multiple szgservers to exist on a single box!
  _socket.reuseAddress(true);
  if (_socket.ar_bind(&incomingAddress) < 0) {
    ar_log_critical() << "failed to bind to " << "INADDR_ANY:" << port
	 << ".\n\t(is another szgserver already running?)\n";
    *(bool *)pv = true; // abort
    return;
  }

  arSocketAddress fromAddress;
  while (true) {
    _socket.ar_read(buffer,200,&fromAddress);
    if (buffer[4] == 1) {
      // Discard this response packet.
      continue;
    }

    // It's not a response packet.  It might be a discovery packet.
    if (!fromAddress.checkMask(serverAcceptMask)){
      ar_log_error() << "rejected packet from non-whitelisted "
                       << fromAddress.getRepresentation() << ".\n";
      continue;
    }

    if (buffer[0] != 0 || buffer[1] != 0 || buffer[2] != 0) {
      ar_log_error() << "ignored misformatted packet (pre-2003 Syzygy?) from "
                       << fromAddress.getRepresentation() << ".\n";
      continue;
    }

    // It's a discovery packet.
    if (buffer[3] != SZG_VERSION_NUMBER) {
      ar_log_error() << "ignored discovery packet with SZG_VERSION_NUMBER = " <<
        int(buffer[3]) << " , from " << fromAddress.getRepresentation() << ".\n";
      continue;
    }

    const string remoteServerName(buffer+5);
    if (remoteServerName != serverName && remoteServerName != "*")
      continue;

    // Respond.
    memset(buffer, 0, sizeof(buffer));
    buffer[3] = SZG_VERSION_NUMBER;
    buffer[4] = 1;

    // Stuff the szgserver's name and IP:port.
    ar_stringToBuffer(serverName, buffer+5, 127);
    ar_stringToBuffer(serverIP, buffer+132, 32);
    sprintf(buffer+164,"%i",serverPort);

    // Broadcast to any NIC whose broadcast address matches fromAddress's.
    bool ok = false;
    for (int i=0; i<computerAddresses.size(); ++i) {
      arSocketAddress tmpAddress;
      if (!tmpAddress.setAddress(computerAddresses[i].c_str(), 0)) {
	ar_log_remark() << "szg.conf has bad address '"
	     << computerAddresses[i] << "'.\n";
	continue;
      }
      const string broadcastAddress =
	tmpAddress.broadcastAddress(computerMasks[i].c_str());
      if (broadcastAddress == "NULL") {
	ar_log_remark() << "szg.conf has bad mask '"
	     << computerMasks[i] << "' for address '"
	     << computerAddresses[i] << "'.\n";
	continue;
      }
      if (broadcastAddress ==
	  fromAddress.broadcastAddress(computerMasks[i].c_str())) {
	fromAddress.setAddress(broadcastAddress.c_str(), port);
	_socket.ar_write(buffer,200,&fromAddress);
	ok = true;
	break;
      }
    }
    if (!ok) {
      ar_log_error() << "szg.conf has no broadcast address for response.\n";
    }
  }
}

// Callback for accessing a database parameter
// (or the process table, or a collection of database parameters)
// @param dataRequest Data record from the client
// @param dataSocket Socket upon which the request travels
void attributeGetRequestCallback(arStructuredData* dataRequest,
                                 arSocket* dataSocket) {

  // Choose the user database.
  SZGactivateUser(dataRequest->getDataString(lang.AR_PHLEET_USER));

  // The attribute name.
  string attribute(dataRequest->getDataString(lang.AR_ATTR_GET_REQ_ATTR));
  string value;

  // Fill in the data from storage.
  arStructuredData* dataResponse = dataParser->getStorage(lang.AR_ATTR_GET_RES);

  _transferMatchFromTo(dataRequest, dataResponse);
  const string type(dataRequest->getDataString(lang.AR_ATTR_GET_REQ_TYPE));

  if (type=="NULL") {
    // Send the process table.
    (void)dataResponse->dataInString(
      lang.AR_ATTR_GET_RES_ATTR, attribute);
    (void)dataResponse->dataInString(
      lang.AR_ATTR_GET_RES_VAL, dataServer->dumpConnectionLabels());
  }

  else if (type=="ALL") {
    // Concatenate all members of valueContainer into a return value.
    (void)dataResponse->dataInString(lang.AR_ATTR_GET_RES_ATTR, attribute);
    // todo: generate a 1.0-style szg dbatch script.
    for (const_iterParam i = valueContainer->begin();
      i != valueContainer->end(); ++i) {
      // Output in dbatch format.
      string first = i->first;
      unsigned slash = first.find("/");
      if (slash == string::npos) {
	// Global attr.
        value += (first + "  " + i->second + "\n");
      }
      else {
	// Local attr.  It has slashes.
        // Replace both slashes with spaces, to make it compatible with the syntax of dbatch.
        first.replace(slash, 1, " ");
        slash = first.find("/");
        first.replace(slash, 1, " ");
        const string& s = first + "   " + i->second + "\n";
        value += s;
      }
    }
    (void)dataResponse->dataInString(lang.AR_ATTR_GET_RES_VAL, value);
  }

  else if (type=="substring") {
    // Send an attribute, or a list of attributes, passing a substring test.
    value = string("(List):\n");
    for (const_iterParam i = valueContainer->begin();
      i != valueContainer->end(); ++i) {
      const string s(i->first + "  =  " + i->second + "\n");
      if (s.find(attribute) != string::npos) {
	value += s;
      }
    }
    (void)dataResponse->dataInString(lang.AR_ATTR_GET_RES_ATTR, attribute);
    (void)dataResponse->dataInString(lang.AR_ATTR_GET_RES_VAL, value);
  }

  else if (type=="value") {
    const_iterParam i(valueContainer->find(attribute));
    value = (i == valueContainer->end()) ? string("NULL") : i->second;
    (void)dataResponse->dataInString(lang.AR_ATTR_GET_RES_ATTR, attribute);
    (void)dataResponse->dataInString(lang.AR_ATTR_GET_RES_VAL, value);
  }

  else {
    ar_log_error() << "internal error: unrecognized type for attribute get request.\n";
  }

  // Send the record.
  dataServer->sendData(dataResponse, dataSocket);
  dataParser->recycle(dataResponse);
}

// Callback for setting a parameter in the database.
// @param pd Record containing the client request
// @param dataSocket Socket upon which the communication occurred
void attributeSetCallback(arStructuredData* pd, arSocket* dataSocket) {
  // Print user data.
  SZGactivateUser(pd->getDataString(lang.AR_PHLEET_USER));
  const string attribute(pd->getDataString(lang.AR_ATTR_SET_ATTR));
  const string value(pd->getDataString(lang.AR_ATTR_SET_VAL));
  const ARint requestType = pd->getDataInt(lang.AR_ATTR_SET_TYPE);

  // Insert the data into the table.
  iterParam i(valueContainer->find(attribute));
  if (requestType == 0) {
    // Remove from the table any data already with this key.
    if (i != valueContainer->end())
      valueContainer->erase(i);
    // Don't insert NULL values.
    if (value != "NULL")
      valueContainer->insert(SZGparamDB::value_type(attribute, value));

    // Ack by filling in the match.
    arStructuredData* connectionAckData = dataParser->getStorage(lang.AR_CONNECTION_ACK);
    _transferMatchFromTo(pd, connectionAckData);
    if (!dataServer->sendData(connectionAckData,dataSocket)) {
      ar_log_error() << "AR_ATTR_SET send failed.\n";
    }
    dataParser->recycle(connectionAckData);
    return;
  }

  // Test-and-set.
  string returnString("NULL");
  if (i != valueContainer->end()) {
    if (i->second == "NULL") {
      // Set the attr's value.
      valueContainer->erase(i);
      // Don't insert NULL values.
      if (value != "NULL") {
        valueContainer->insert(SZGparamDB::value_type(attribute, value));
      }
      returnString = value;
    }
  } else {
    // Don't insert NULL values.
    if (value != "NULL") {
      valueContainer->insert(SZGparamDB::value_type(attribute, value));
    }
    returnString = value;
  }

  // Return the info, first getting some space to put it in.
  arStructuredData* attrGetResponseData = dataParser->getStorage(lang.AR_ATTR_GET_RES);
  _transferMatchFromTo(pd, attrGetResponseData);
  if (!attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_ATTR, attribute) ||
      !attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_VAL, returnString) ||
      !dataServer->sendData(attrGetResponseData, dataSocket)) {
    ar_log_error() << "AR_ATTR_GET_RES send failed.\n";
  }
  dataParser->recycle(attrGetResponseData);
}

inline const char* szgSuccess(bool ok)
{
  return ok ? "SZG_SUCCESS" : "SZG_FAILURE";
}

// THIS FUNCTION IS OBNOXIOUSLY SIMPLE. IT MAKES READABILITY SUFFER.
// DO NOT INSERT THIS SORT OF THING IN THE FUTURE.
bool SZGack(arStructuredData* messageAckData, bool ok) {
  return messageAckData->dataInString(lang.AR_SZG_MESSAGE_ACK_STATUS,
    szgSuccess(ok));
}

void processInfoCallback(arStructuredData* pd, arSocket* dataSocket) {
 
  const string requestType = pd->getDataString(lang.AR_PROCESS_INFO_TYPE);
  int theID = -1;
  string theLabel;
  if (requestType == "self") {
    theID = dataSocket->getID();
    theLabel = dataServer->getSocketLabel(theID);
  }
  else if (requestType == "ID") {
    theLabel = pd->getDataString(lang.AR_PROCESS_INFO_LABEL);
    theID = dataServer->getFirstIDWithLabel(theLabel);
  }
  else if (requestType == "label") {
    theID = pd->getDataInt(lang.AR_PROCESS_INFO_ID);
    theLabel = dataServer->getSocketLabel(theID);
  }
  else {
    ar_log_error() << "got unknown type on process info request.\n";
  }
  pd->dataInString(lang.AR_PROCESS_INFO_LABEL, theLabel);
  pd->dataIn(lang.AR_PROCESS_INFO_ID, &theID, AR_INT, 1);
  if (!dataServer->sendData(pd, dataSocket)) {
    ar_log_error() << "process info send failed.\n";
  }
}

// Callback for forwarding an incoming message to its final destination.
// @param pd Incoming record
// @param dataSocket Connection upon which we received the data
void messageProcessingCallback(arStructuredData* pd,
                               arSocket* dataSocket) {
  // Print user data.
  SZGactivateUser(pd->getDataString(lang.AR_PHLEET_USER));
  bool forward = false; // forward the message?
  // Fill in the fields for the message ack, and
  // send it back to the client who sent us this message.
  arStructuredData* messageAckData 
    = dataParser->getStorage(lang.AR_SZG_MESSAGE_ACK);
  // Must fill in the "match".
  _transferMatchFromTo(pd, messageAckData);
  // Put a default ID into the SZGmessageAckData record.
  int theMessageID = 0;
  messageAckData->dataIn(lang.AR_SZG_MESSAGE_ACK_ID, &theMessageID, 
                         AR_INT, 1);
  // Find the destination component's ID.
  int* dataPtr = (int*) pd->getDataPtr(lang.AR_SZG_MESSAGE_DEST,AR_INT);
  if (!dataPtr) {
    ar_log_error() << "ignoring message with NULL data pointer."
         << "\n\t(Does a client have an incompatible protocol?)\n";
  }
  else {
    // Try to send the message. Query the dataServer to get
    // a communication endpoint with the given ID.
    arSocket* destSocket = dataServer->getConnectedSocket(*dataPtr);
    if (destSocket) {
      // Destination component hasn't died.

      // We oughta be able to deliver the message. Assign it an ID.
      // NOT THREADSAFE... SHOULD ENCAPUSLATE MESSAGEID
      // ASSIGNMENT IN A MUTEX PROTECTED FUNCTION.
      theMessageID = nextMessageID++;

      // fill in the message's ID field
      pd->dataIn(lang.AR_SZG_MESSAGE_ID, &theMessageID, AR_INT, 1);
      // check to see if a response has been requested. if so,
      if (pd->getDataInt(lang.AR_SZG_MESSAGE_RESPONSE) > 0) {
        // a response has been requested, so record the ID of the
        // component that's allowed to respond,
        // and the ID of where to send the response.
        SZGaddMessageToDB(theMessageID, destSocket->getID(),
	  dataSocket->getID(), pd->getDataInt(lang.AR_PHLEET_MATCH));
      }
      if (!dataServer->sendData(pd,destSocket)) {
        ar_log_error() << "message send failed.\n";
      }
      else {
	// Message probably forwarded ok.
        forward = true;
      }
    }
  }
  if (!SZGack(messageAckData, forward) ||
     (forward &&
      !messageAckData->dataIn(lang.AR_SZG_MESSAGE_ACK_ID, &theMessageID, AR_INT, 1)) ||
      !dataServer->sendData(messageAckData,dataSocket)) {
    ar_log_error() << "message ack send failed.\n";
  }
  dataParser->recycle(messageAckData);
}

// Callback for processing the message admin data, which includes responses
// @param pd Incoming data record
// @param dataSocket Connection upon which we received the data
void messageAdminCallback(arStructuredData* pd,
			  arSocket* dataSocket) {
  const string messageAdminType = pd->getDataString(lang.AR_SZG_MESSAGE_ADMIN_TYPE);
  bool status = false;
  int responseOwner = -1;
  int responseDestination = -1;
  arPhleetMsg messageData;
  string key;
  arSocket* responseSocket = NULL;

  // Store the response.
  arStructuredData* messageAckData = dataParser->getStorage(lang.AR_SZG_MESSAGE_ACK);
  // Propagate the match from message to response.
  _transferMatchFromTo(pd, messageAckData);

  if (messageAdminType == "SZG Response") {
    const int messageID = pd->getDataInt(lang.AR_SZG_MESSAGE_ADMIN_ID);
    responseOwner = SZGgetMessageOwnerID(messageID);
    if (responseOwner < 0) {
      // No message has this ID.
      // Owner probably died while one app died and another started.
      ar_log_debug() << "ignoring response to unowned message.\n";
    }
    else {
      // A message with this ID expects a response.
      responseDestination = SZGgetMessageOriginatorID(messageID);
      if (responseOwner != dataSocket->getID()) {
        ar_log_error() << "illegal response attempt from component "
	     << dataSocket->getID()
	     << " (" << dataServer->getSocketLabel(dataSocket->getID())
	     << "), owner is " << responseOwner << ".\n";
      }
      else {
        responseSocket = dataServer->getConnectedSocket(responseDestination);
        if (!responseSocket) {
	  ar_log_error() << "no destination to respond to.\n";
	}
	else {
	  // Fill in the match.
          const int match = SZGgetMessageMatch(messageID);
          pd->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
          if (!dataServer->sendData(pd, responseSocket)) {
	    ar_log_error() << "response failed.\n";
	  }
	  else {
	    status = true;
	  }
	}
        const string s = pd->getDataString(lang.AR_SZG_MESSAGE_ADMIN_STATUS);
        if (s != "SZG_CONTINUE") {
	  // The message won't be continued.
          SZGremoveMessageFromDB(messageID);
	  if (s != "SZG_SUCCESS") {
	    ar_log_error() << "message response had unexpected status field '"
		 << s << "' (expected SZG_SUCCESS or SZG_CONTINUE).\n";
	  }
	}
      }
    }
  }

  else if (messageAdminType == "SZG Trade Message") {
    const int messageID = pd->getDataInt(lang.AR_SZG_MESSAGE_ADMIN_ID);
    key = pd->getDataString(lang.AR_SZG_MESSAGE_ADMIN_BODY);
    status = SZGaddMessageTradeToDB(
      key, messageID, dataSocket->getID(), pd->getDataInt(lang.AR_PHLEET_MATCH));
  }

  else if (messageAdminType == "SZG Message Request") {
    key = pd->getDataString(lang.AR_SZG_MESSAGE_ADMIN_BODY);
    arPhleetMsg oldInfo;
    // NOTE: SZGmessageRequest overwrites the owner ID, which we'll need
    // later. Consequently, we need to preserve the original owner ID here.
    // No need to check the return value or complain. If there is an error,
    // SZGmessageRequest will get that itself.
    SZGgetMessageTradeInfo(key, oldInfo);
    // messageData is passed by reference:
    if (SZGmessageRequest(key, dataSocket->getID(), messageData)) {
      // Notify originator of the trade that the trade has occurred.
      (void)SZGack(messageAckData, true);
      int messageID = -1;
      // Put in the match from the original trade.
      messageAckData->dataIn(lang.AR_PHLEET_MATCH, &messageData.idTradingMatch, AR_INT, 1);
      messageAckData->dataIn(lang.AR_SZG_MESSAGE_ACK_ID, &messageID, AR_INT, 1);
      // Send back to the originator of the message trade, not the new owner.
      responseSocket = dataServer->getConnectedSocket(oldInfo.idOwner);
      if (!responseSocket) {
	ar_log_error() << "missing originator of message trade.\n";
      }
      else if (!dataServer->sendData(messageAckData,responseSocket)) {
	ar_log_error() << "failed to notify originator about message trade.  Originator may have failed.\n";
      }
      // Fill in the ID field of the record to be sent back to the component
      // requesting the trade with the message's ID.
      // Reuse the messageAck storage.
      messageID = messageData.id;
      // Put the normal match back in. (THIS WILL BE SENT LATER)
      _transferMatchFromTo(pd, messageAckData);
      messageAckData->dataIn(lang.AR_SZG_MESSAGE_ACK_ID, &messageID, AR_INT, 1);
      status = true;
    }
  }

  else if (messageAdminType == "SZG Revoke Trade") {
    key = pd->getDataString(lang.AR_SZG_MESSAGE_ADMIN_BODY);
    status = SZGrevokeMessageTrade(key, dataSocket->getID());
  }

  // ACK, with ID field possibly not filled in.
  if (!SZGack(messageAckData, status) ||
      !dataServer->sendData(messageAckData, dataSocket)) {
    ar_log_error() << "failed to ack.\n";
  }
  dataParser->recycle(messageAckData);
}

// Let a component request notification when another component exits.
void killNotificationCallback(arStructuredData* data,
			      arSocket* dataSocket) {
  int componentID = data->getDataInt(lang.AR_SZG_KILL_NOTIFICATION_ID);
  if (!dataServer->getConnectedSocket(componentID)) {
    // NO SUCH COMPONENT EXISTS. report back immediately
    if (!dataServer->sendData(data, dataSocket)) {
      ar_log_error() << "failed to send kill notification.\n";
      return;
    }
    // AND DO NOT INSERT NOTIFICATION REQUEST INTO DATABASE
    return;
  }
  // IMPORTANT NOTE: THIS ONLY WORKS BECAUSE THE data server IS SERIALIZING
  // CALLS TO THE szgserver. NOTE THE LACK OF ATOMICITY!
  // the lock is currently held.
  SZGrequestKillNotification(dataSocket->getID(), componentID,
                             data->getDataInt(lang.AR_PHLEET_MATCH));
}

// Helper functions for lockRequestCallback, lockReleaseCallback.

string lockRequestInit(arStructuredData* lockResponseData,
                       arStructuredData* pd) {
  const string lockName(pd->getDataString(lang.AR_SZG_LOCK_RELEASE_NAME));
  (void)lockResponseData->dataInString(lang.AR_SZG_LOCK_RESPONSE_NAME, 
                                       lockName);
  return lockName;
}

void lockRequestFinish(arStructuredData* lockResponseData,
                       const bool ok,
                       const int ownerID, arSocket* dataSocket) {
  if (!lockResponseData->dataIn(lang.AR_SZG_LOCK_RESPONSE_OWNER,
                                &ownerID, AR_INT, 1) ||
      !lockResponseData->dataInString(lang.AR_SZG_LOCK_RESPONSE_STATUS,
                                      szgSuccess(ok)) ||
      !dataServer->sendData(lockResponseData,dataSocket)) {
    ar_log_error() << "failed to send lock response.\n";
  }
}

// Callback to process a lock request.
// @param pd Incoming data record (lock request)
// @param dataSocket Connection upon which we received the data
void lockRequestCallback(arStructuredData* pd,
			 arSocket* dataSocket) {
  arStructuredData* lockResponseData
    = dataParser->getStorage(lang.AR_SZG_LOCK_RESPONSE);
  // Must propogate the "match".
  _transferMatchFromTo(pd, lockResponseData);
  const string lockName 
    = pd->getDataString(lang.AR_SZG_LOCK_REQUEST_NAME);
  (void)lockResponseData->dataInString(lang.AR_SZG_LOCK_RESPONSE_NAME, 
                                       lockName);
  int ownerID = -1;
  const bool ok = SZGgetLock(lockName, dataSocket->getID(), ownerID);
  lockRequestFinish(lockResponseData, ok, ownerID, dataSocket);
  dataParser->recycle(lockResponseData);
}

// Process a request to release a lock.
// @param pd Incoming data record (lock release)
// @param dataSocket Connection upon which we received the data
void lockReleaseCallback(arStructuredData* pd,
			 arSocket* dataSocket) {
  const int ownerID = -1;
  arStructuredData* lockResponseData =
    dataParser->getStorage(lang.AR_SZG_LOCK_RESPONSE);
  // Propagate the "match".
  _transferMatchFromTo(pd, lockResponseData);
  const bool ok = SZGreleaseLock(lockRequestInit(lockResponseData, pd), 
    dataSocket->getID());
  lockRequestFinish(lockResponseData, ok, ownerID, dataSocket);
  dataParser->recycle(lockResponseData);
}

// Process a request to print all currently held locks
// @param pd Incoming data record (lock release)
// @param dataSocket Connection upon which we received the data
void lockListingCallback(arStructuredData* pd,
			 arSocket* dataSocket) {
  const int listSize = lockOwnershipDB.size();
  int* IDs = new int[listSize];
  int iID = 0;
  arSemicolonString locks;
  for (SZGlockOwnershipDB::iterator i=lockOwnershipDB.begin();
       i != lockOwnershipDB.end(); i++) {
    locks /= i->first;
    IDs[iID++] = i->second;
  }
  pd->dataInString(lang.AR_SZG_LOCK_LISTING_LOCKS, locks);
  pd->dataIn(lang.AR_SZG_LOCK_LISTING_COMPONENTS, IDs, AR_INT, listSize);
  if (!dataServer->sendData(pd, dataSocket))
    ar_log_error() << "failed to send lock listing response.\n";
  delete [] IDs;
}

// Let a component request notification when a lock is released.
void lockNotificationCallback(arStructuredData* data,
			      arSocket* dataSocket) {
  const string lockName(data->getDataString(lang.AR_SZG_LOCK_NOTIFICATION_NAME));
  // There is no need to propogate the match in the failure case, since
  // the received message is simply returned.
  if (!SZGcheckLock(lockName)) {
    // the lock is NOT currently held, report back immediately
    if (!dataServer->sendData(data, dataSocket)) {
      ar_log_error() << "failed to send lock release notification.\n";
      return;
    }
    // AND DO NOT INSERT NOTIFICATION REQUEST INTO DATABASE
    return;
  }
  // IMPORTANT NOTE: THIS ONLY WORKS BECAUSE THE data server IS SERIALIZING
  // CALLS TO THE szgserver. NOTE THE LACK OF ATOMICITY!
  // the lock is currently held.
  SZGrequestLockNotification(dataSocket->getID(), lockName,
                             data->getDataInt(lang.AR_PHLEET_MATCH));
}

// Callback to process a request to register a service
// @param pd Incoming data record (contains info about the service to
// be registered)
// @param dataSocket Connection upon which we received the data
void registerServiceCallback(arStructuredData* pd, arSocket* dataSocket) {
  // Check the status field first. This indicates whether we are receiving
  // an initial service registration request OR a retry that the remote
  // component has demanded because it could not use some of the returned ports
  // Unpack the record into easy-to-use variables.
  const int match = pd->getDataInt(lang.AR_PHLEET_MATCH);
  const string status(pd->getDataString(lang.AR_SZG_REGISTER_SERVICE_STATUS));
  const string serviceName(pd->getDataString(lang.AR_SZG_REGISTER_SERVICE_TAG));
  const string networks(pd->getDataString(lang.AR_SZG_REGISTER_SERVICE_NETWORKS));
  const string addresses(pd->getDataString(lang.AR_SZG_REGISTER_SERVICE_ADDRESSES));
  const int size = pd->getDataInt(lang.AR_SZG_REGISTER_SERVICE_SIZE);
  const string computer(pd->getDataString(lang.AR_SZG_REGISTER_SERVICE_COMPUTER));
  int temp[2];
  pd->dataOut(lang.AR_SZG_REGISTER_SERVICE_BLOCK, temp, AR_INT, 2);
  const int& firstPort = temp[0];
  const int& blockSize = temp[1];
 
  arPhleetService result;
  arStructuredData* data = NULL;

  if (status == "SZG_TRY") {
    // check to see if the service if already registered. if not, assign ports
    // and return them to the requesting component
    result = connectionBroker.requestPorts(
        dataSocket->getID(), serviceName, computer, networks, addresses,
	size, firstPort, blockSize);
LAgain:
    data = dataParser->getStorage(lang.AR_SZG_BROKER_RESULT);
    data->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
    data->dataInString(lang.AR_SZG_BROKER_RESULT_STATUS, 
                       szgSuccess(result.valid));
    if (result.valid) {
      // Found an unbound port, and nobody else was using the service tag.
      // Don't fill in the address field:  the service binds to INADDR_ANY.
      data->dataIn(lang.AR_SZG_BROKER_RESULT_PORT, result.portIDs,
                   AR_INT, result.numberPorts);
    }
    if (!dataServer->sendData(data, dataSocket)) {
      ar_log_error() << "failed to respond to service registration request.\n";
    }
  }

  else if (status == "SZG_RETRY") {
    // The previously assigned ports failed.  Get new ones.
    result = connectionBroker.retryPorts(dataSocket->getID(), serviceName);
    goto LAgain;
  }

  else if (status == "SZG_SUCCESS") {
    // Confirmation that the client bound to the assigned ports.
    const bool status = connectionBroker.confirmPorts(dataSocket->getID(), serviceName);
    data = dataParser->getStorage(lang.AR_SZG_BROKER_RESULT);
    // Propogate the "match".
    data->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
    data->dataInString(lang.AR_SZG_BROKER_RESULT_STATUS, szgSuccess(status));
    if (!dataServer->sendData(data, dataSocket)) {
      ar_log_error() << "failed to respond to service confirmation.\n";
    }
  
    // The service is truly registered.
    // Notify the components that are waiting.
    SZGRequestList waiting = connectionBroker.getPendingRequests(serviceName);
    for (SZGRequestList::iterator i = waiting.begin();
	 i != waiting.end(); i++) {
      // Do not put a service request on the queue, so
      // the last arg of requestService() is false.
      arPhleetAddress addr = connectionBroker.requestService(
        i->componentID, i->computer, i->match, i->serviceName, i->networks, false);
      // The match for the async call to requestServiceCallback(...)
      int oldMatch = i->match;
      data->dataIn(lang.AR_PHLEET_MATCH, &oldMatch, AR_INT, 1);
      data->dataInString(lang.AR_SZG_BROKER_RESULT_STATUS, 
                         szgSuccess(addr.valid));
      if (addr.valid) {
        data->dataInString(lang.AR_SZG_BROKER_RESULT_ADDRESS, addr.address);
        data->dataIn(lang.AR_SZG_BROKER_RESULT_PORT, addr.portIDs, AR_INT,
                     addr.numberPorts);
      }
      arSocket* dest = dataServer->getConnectedSocket(i->componentID);
      if (!dataServer->sendData(data, dest)) {
        ar_log_error() << "failed to send async broker result to component "
	     << i->componentID << ".\n";
      }
    }
  }

  else {
    ar_log_error() << "ignoring service registration with unexpected status field '"
      << status << "'.\n";
    return;
  }

  dataParser->recycle(data);
}


// Handles requests for service locations. In the simplest case, the client
// requests a named service which is currently registered with the szgserver.
// The server then determines the appropriate network path, returning that
// to the client.
void requestServiceCallback(arStructuredData* pd, arSocket* dataSocket) {
  const string computer = pd->getDataString(lang.AR_SZG_REQUEST_SERVICE_COMPUTER);
  const int match = pd->getDataInt(lang.AR_PHLEET_MATCH);
  const string serviceName = pd->getDataString(lang.AR_SZG_REQUEST_SERVICE_TAG);
  const string networks = pd->getDataString(lang.AR_SZG_REQUEST_SERVICE_NETWORKS);
  const string async = pd->getDataString(lang.AR_SZG_REQUEST_SERVICE_ASYNC);
  bool asyncFlag = async == "SZG_TRUE";

  // If async is true, then if the request fails (there's no matching service)
  // the connection broker will store the request and will generate a 
  // response (using the match even) in registerServiceCallback(...).
  arPhleetAddress result =
    connectionBroker.requestService(dataSocket->getID(), computer, match,
                                    serviceName, networks, asyncFlag);
  // Send a broker request result.
  arStructuredData* data 
    = dataParser->getStorage(lang.AR_SZG_BROKER_RESULT);
  // Propogate the "match".
  data->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
  data->dataInString(lang.AR_SZG_BROKER_RESULT_STATUS, 
                     szgSuccess(result.valid));
  if (result.valid) {
    // got a match
    data->dataInString(lang.AR_SZG_BROKER_RESULT_ADDRESS, result.address);
    data->dataIn(lang.AR_SZG_BROKER_RESULT_PORT, result.portIDs, AR_INT,
                 result.numberPorts);
    if (!dataServer->sendData(data, dataSocket)) {
      ar_log_error() << "failed to send broker result in response to service request.\n";
    }
  }
  else {
    // No compatible service. Either no such service currently
    // exists or it is only offered on incompatible networks.
    // Respond if we are in synchronous mode. (otherwise the response will
    // occur in registerServiceCallback(...).
    if (!asyncFlag && !dataServer->sendData(data, dataSocket)) {
      ar_log_error() << "failed to send broker result in response to service request.\n";
    }
  }
  
  dataParser->recycle(data);
}

// Handles requests for total lists of services (a dps analogy)
// (or for the component IDs of specific ones, as is required when one
// wants to kill a component offering a particular service so that a new
// one can start up)
void getServicesCallback(arStructuredData* pd,
			arSocket* dataSocket) {
  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  // this method is not threadsafe vis-a-vis the connection broker.
  // no problem right now, since the szgserver executes requests in 
  // sequence.
  // WEIRD. IT SEEMS LIKE WE ARE USING A NEW PIECE OF DATA. WHY NOT
  // SEND IT BACK IN PLACE? 
  const string type(pd->getDataString(lang.AR_SZG_GET_SERVICES_TYPE));
  arStructuredData* data = dataParser->getStorage(lang.AR_SZG_GET_SERVICES);

  // Propogate the "match".
  _transferMatchFromTo(pd, data);

  if (type == "active") {
    const string 
      serviceName(pd->getDataString(lang.AR_SZG_GET_SERVICES_SERVICES));
    if (serviceName == "NULL") {
      // respond with the list of all services
      data->dataInString(lang.AR_SZG_GET_SERVICES_SERVICES,
                         connectionBroker.getServiceNames());
      data->dataInString(lang.AR_SZG_GET_SERVICES_COMPUTERS,
		         connectionBroker.getServiceComputers());
      int* IDs = NULL;
      const int numberServices = connectionBroker.getServiceComponents(IDs);
      data->dataIn(lang.AR_SZG_GET_SERVICES_COMPONENTS, IDs, AR_INT,
                   numberServices);
      // we manage memory allocated to store the IDs
      delete [] IDs;
    }
    else {
      // we must respond with a particular service's ID, if that service 
      // exists, and otherwise return -1
      data->dataInString(lang.AR_SZG_GET_SERVICES_SERVICES, serviceName);
      int result = connectionBroker.getServiceComponentID(serviceName);
      data->dataIn(lang.AR_SZG_GET_SERVICES_COMPONENTS, &result, AR_INT, 1);
    }
  }

  else if (type == "pending") {
    SZGRequestList result = connectionBroker.getPendingRequests();
    const int listSize = result.size();
    int* IDs = new int[listSize];
    int iID = 0;
    arSemicolonString names;
    arSlashString computers;
    for (SZGRequestList::iterator i = result.begin(); i != result.end(); i++) {
      names /= i->serviceName;
      computers /= i->computer;
      IDs[iID++] = i->componentID;
    }
    data->dataInString(lang.AR_SZG_GET_SERVICES_SERVICES, names);
    data->dataInString(lang.AR_SZG_GET_SERVICES_COMPUTERS, computers);
    data->dataIn(lang.AR_SZG_GET_SERVICES_COMPONENTS, IDs, AR_INT, listSize);
    delete [] IDs;
  }

  else {
    ar_log_error() << "service listing had invalid request type '"
         << type << "'.\n";
  }

  if (!dataServer->sendData(data, dataSocket))
    ar_log_error() << "failed to send service list.\n";
  dataParser->recycle(data);
}

void serviceReleaseCallback(arStructuredData* pd,
			    arSocket* dataSocket) {
  // IMPORTANT NOTE: there are major problems here with atomicity.
  // CURRENTLY, the fact that the szgserver processes messages one-at-a-time
  // saves us. TODO TODO TODO TODO TODO TODO
  // NOTE: since the data is processed in place on failure, no need to
  // explicitly propogate the match. (though there is inside the connection
  // broker).
  const string 
    serviceName(pd->getDataString(lang.AR_SZG_SERVICE_RELEASE_NAME));
  const string 
    computer(pd->getDataString(lang.AR_SZG_SERVICE_RELEASE_COMPUTER));
  // see if the current service is *not* currently held.
  if (!connectionBroker.checkService(serviceName)) {
    // immediately respond
    if (!dataServer->sendData(pd, dataSocket)) {
      ar_log_error() << "failed to respond to service release.\n";
      return;
    }
  }
  // we will respond later, when the service does, in fact, become available.
  // This is the source of the lack of atomicity. No call to the connection
  // broker should occur between the above and here.
  // BUG: THIS IS NOT ATOMIC AND REALLY SHOULD RESPOND WITH A BOOL.
  connectionBroker.registerReleaseNotification(dataSocket->getID(),
				    pd->getDataInt(lang.AR_PHLEET_MATCH),
				    computer,
				    serviceName);
}

void serviceInfoCallback(arStructuredData* pd,
			 arSocket* dataSocket) {
  // NOTE: since we are just sending the same data back, the match does not
  // need to be propogated!
  const string op(pd->getDataString(lang.AR_SZG_SERVICE_INFO_OP));
  const string name(pd->getDataString(lang.AR_SZG_SERVICE_INFO_TAG));
  if (op == "get") {
    pd->dataInString(lang.AR_SZG_SERVICE_INFO_STATUS,
                          connectionBroker.getServiceInfo(name));
    if (!dataServer->sendData(pd, dataSocket)) {
      ar_log_error() << "failed to send service info.\n";
    }
  }
  else if (op == "set") {
    const string info(pd->getDataString(lang.AR_SZG_SERVICE_INFO_STATUS));
    bool status = connectionBroker.setServiceInfo(dataSocket->getID(),
                                                  name, info);
    const string statusString = szgSuccess(status);
    pd->dataInString(lang.AR_SZG_SERVICE_INFO_STATUS, statusString);
    if (!dataServer->sendData(pd, dataSocket)) {
      ar_log_error() << "failed to get service info.\n";
    }
  }
  else {
    ar_log_error() << "got wrong service info operation '" << op << "'.\n";
  }
}

// Handle receipt of data records from connected arSZGClients.
// (szgserver processes client requests serially, which may
// be bad but is hard to change:  see arDataServer.cpp for
// how locking enforces serialization.)
// @param pd Parsed record from the client
// @param dataSocket Connection on which the record was received
void dataConsumptionFunction(arStructuredData* pd, void*, arSocket* dataSocket) {

  // Ensure that arDataServer's read thread serializes calls to this function.
  // UNSURE IF THIS CHECK EVEN MAKES SENSE.
  static bool fInside = false;
  if (fInside) {
    ar_log_error() << "internal error: nonserialized dataConsumptionFunction.\n";
    return;
  }
  fInside = true;

  const int theID = pd->getID();
  if (theID == lang.AR_ATTR_GET_REQ) {
    // Propagate the match.
    attributeGetRequestCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_ATTR_GET_RES) {
    // only client not server should get this
    ar_log_error() << "ignoring AR_ATTR_GET_RES message.\n";
  }
  else if (theID == lang.AR_ATTR_SET) {
    // Propagate the match.
    attributeSetCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_CONNECTION_ACK) {
    // Connected app wants to change its label.
    // Insert the label into the table.
    dataServer->setSocketLabel(dataSocket,
      pd->getDataString(lang.AR_CONNECTION_ACK_LABEL));
    // This gets the szgserver name as a reply... useful when shipping
    // the szgserver name to a connecting component.
    pd->dataInString(lang.AR_CONNECTION_ACK_LABEL,serverName);
    // Match isn't propagated because we're just
    // sending back the received data (which already has the match value).
    if (!dataServer->sendData(pd, dataSocket)) {
      ar_log_error() << "failed to send connection ack reply.\n";
    }
  }
  else if (theID == lang.AR_KILL) {
    // The process with this ID died unexpectedly (it left the
    // socket open, perhaps because the host crashed, perhaps because
    // a wireless network TCP connection was interrupted).
    // So forget about this guy.
    const int id = pd->getDataInt(lang.AR_KILL_ID);
    // No response to this command. (MAYBE THAT SHOULD CHANGE?)
    // So don't propagate the match.

    // Kill the component in question.
    arSocket* killSocket = dataServer->getConnectedSocket(id);
    if (killSocket) {
      // Tell the component to *rudely* shut down
      // (as opposed to the polite messaging way).
      if (!dataServer->sendData(pd, killSocket)) {
	ar_log_remark() << "failed to send kill.\n";
      }
    }
    // Remove the socket from our table.
    // Bug: closing the socket on this
    // side may prevent the socket on the other
    // side from receiving the kill message we sent! Why?
    if (!dataServer->removeConnection(id)) {
      ar_log_error() << "failed to 'kill -9' pid " << id << ".\n";
    }
  }
  else if (theID == lang.AR_PROCESS_INFO) {
    // Returns ID and/or label for a specific process.
    // Don't propagate match, becaues we just
    // return the received data, with a few fields filled in.
    processInfoCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_MESSAGE) {
    // Callback propagates match.
    messageProcessingCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_MESSAGE_ADMIN) {
    // Callback propagates match.
    messageAdminCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_KILL_NOTIFICATION) {
    // Callback propagates match.
    killNotificationCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_LOCK_REQUEST) {
    // Callback propagates match.
    lockRequestCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_LOCK_RELEASE) {
    // Callback propagates match.
    lockReleaseCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_LOCK_LISTING) {
    // The received message just has some fields filled-in and
    // is then returned to sender. Consequently, no need to
    // propogate the match.
    lockListingCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_LOCK_NOTIFICATION) {
    // Callback propagates match, if lock is held.
    lockNotificationCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_REGISTER_SERVICE) {
    // Callback propagates match.
    registerServiceCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_REQUEST_SERVICE) {
    // Callback propagates match, for both immediate and async
    // responses (via the connection broker).
    requestServiceCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_GET_SERVICES) {
    // Callback propagates match.
    getServicesCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_SERVICE_RELEASE) {
    // Callback propagates match.
    serviceReleaseCallback(pd, dataSocket);
  }
  else if (theID == lang.AR_SZG_SERVICE_INFO) {
    // Callback propagates match.
    serviceInfoCallback(pd, dataSocket);
  }
  else {
    ar_log_error() << "ignoring record with unknown ID " << theID
	 << ".\n  (Version mismatch between szgserver and client?)\n";
  }
  fInside = false;
}

// arDataServer calls this when a connection goes away
// (when a read or write call on the socket returns false).
// @param theSocket Socket whose connection died
void SZGdisconnectFunction(void*, arSocket* s) {
  SZGremoveComponentFromDB(s->getID());
}

int main(int argc, char** argv) {
  (void)ar_setLogLevel("REMARK", false);
  ar_log().setHeader("szgserver");

  if (argc < 3) {
    ar_log_critical() <<
      "usage: szgserver name port [mask.1 ...]\n\texample: szgserver yoyodyne 8888\n";
    return 1;
  }

  ar_log().setTimestamp(true);
  if (argc > 3) {
    for (int i = 3; i < argc; i++) {
      string arg( argv[i] );
      if (arg == "-debug") {
        (void)ar_setLogLevel("DEBUG", false);
      } else {
        serverAcceptMask.push_back(string(argv[i]));
      }
    }
  }

  // If another szgserver on the network has the same name, abort.
  // We can discover this like dhunt's arSZGClientServerResponseThread(),
  // but we, not dhunt's stdout, need the result.
  connectionBroker.setReleaseNotificationCallback(SZGreleaseNotificationCallback);

  serverName = string(argv[1]);
  // todo: errorcheck serverPort, so it's outside the block of ports for connection brokering
  serverPort = atoi(argv[2]);
  // Determine serverIP, to tell client where to connect while we bind to INADDR_ANY.
  arPhleetConfig config;
  if (!config.read()) {
    ar_log_critical() << "syntax error in config file (try dconfig).\n";
    return 1;
  }

  // Find the first address interface in the list,
  // the one reported to dhunt and dlogin.
  computerAddresses = config.getAddresses();
  computerMasks = config.getMasks();
  if (computerAddresses.empty()) {
    ar_log_critical() << "config file defines no networks.\n";
    return 1;
  }

  serverIP = computerAddresses[0];
  bool fAbort = false;
  arThread dummy(serverDiscoveryFunction, &fAbort);
  // Give thread a chance to fail (it will set fAbort).
#ifndef AR_USE_WIN_32
  ar_usleep(100000);
#endif
  if (fAbort)
    return 1;

  // Initialize the data server.
  dataServer = new arDataServer(1000);
  // we might want to do TCP-wrappers style filtering on the connections
  dataServer->setAcceptMask(serverAcceptMask);
  dataServer->setDisconnectFunction(SZGdisconnectFunction);

  // Start the data parser.
  dataParser = new arStructuredDataParser(lang.getDictionary());
  if (!dataServer->setInterface("INADDR_ANY") ||
      !dataServer->setPort(serverPort)) {
    ar_log_critical() << "data server has invalid IP:port " <<
      serverIP  << ":" << serverPort << ".\n";
    return 1;
  }

  dataServer->setConsumerFunction(dataConsumptionFunction);
  dataServer->setConsumerObject(NULL);
  dataServer->smallPacketOptimize(true);
  if (!dataServer->beginListening(lang.getDictionary()))
    return 1;

  // Give other threads a chance to fail (they will set fAbort).
  ar_usleep(100000);

  while (!fAbort) {
    dataServer->acceptConnection();
  }
  return fAbort ? 1 : 0;
}
