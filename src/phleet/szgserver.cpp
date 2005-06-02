//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arPhleetOSLanguage.h"
#include "arDataServer.h"
#include "arThread.h"
#include "arUDPSocket.h"
#include "arPhleetConfigParser.h"
#include "arPhleetConnectionBroker.h"
#include <iostream>
#include <stdio.h>
using namespace std;

// a parser that can manage storage for that dictionary
arPhleetOSLanguage       lang;
arStructuredDataParser*  dataParser = NULL;
arDataServer*            dataServer = NULL;
arPhleetConnectionBroker connectionBroker;

// We might want to do TCP-wrappers style filtering on the incoming IPs.
list<string> serverAcceptMask;

// Need to keep a record of the addresses of the NICs in this computer.
// This pertains to the "discovery" process.
arSlashString computerAddresses;
arSlashString computerMasks;

string serverName;
int    serverPort = -1;
string serverInterface;

//*******************************************
// global data storage used by the szgserver
//*******************************************

// One container stores database values particular to szgserver itself.
// This basically includes only the pseudoDNS at this stage.
typedef map<string,string,less<string> > SZGparamDatabase;
SZGparamDatabase rootContainer;

// One parameter database for each user who has logged-on
typedef map<string,SZGparamDatabase*,less<string> > SZGuserDatabase;
SZGuserDatabase userDatabase;

// The current parameter database
map<string,string,less<string> >* valueContainer = NULL;

arMutex receiveDataMutex;

// message IDs start at 1
int nextMessageID = 1;

class arPhleetMessage{
 public:
  arPhleetMessage(){}
  ~arPhleetMessage(){}

  int messageID;           // the ID of this message
  int messageOwner;        // the ID of the message's owner
                           // (i.e. the component to which it was
                           //  originally directed or possibly someone else
                           // if there was a message trade) 
  int responseDestination; // the response's destination
  int match;               // the "match" for the response
  int tradingMatch;        // the "match" for the trade, if one is currently
                           // occuring
};

// DOH!!! The next section of STL shows VERY BAD DATA STRUCTURE DESIGN!!

// storage that associates a message ID with the info about the message
// (such as message ID, the ID of the component that can respond, the
// component to which a response can be directed, etc.)
typedef map<int,arPhleetMessage,less<int> > SZGmessageOwnershipDatabase;
SZGmessageOwnershipDatabase messageOwnershipDatabase;

// storage that associates a message key with info about the message
typedef map<string,arPhleetMessage,less<string> > SZGmessageTradingDatabase;
SZGmessageTradingDatabase messageTradingDatabase;

// when a connection goes away, we need to be able to respond to all messages
// owned by that particular component (but to which a response has not yet
// been posted)... i.e. by sending an "error message"
typedef map<int,list<int>,less<int> > SZGcomponentMessageOwnershipDatabase;
SZGcomponentMessageOwnershipDatabase componentMessageOwnershipDatabase;

// when a connection goes away, we need to be able to remove all the
// "message trades" initiated by that component and send error responses to
// the original senders
typedef map<int,list<string>,less<int> > SZGcomponentTradingOwnershipDatabase;
SZGcomponentTradingOwnershipDatabase componentTradingOwnershipDatabase;

// storage that associates a named lock with the ID of the component that
// holds the lock
typedef map<string,int,less<string> > SZGlockOwnershipDatabase;
SZGlockOwnershipDatabase lockOwnershipDatabase;

// when a connection goes away, we need to be able to release all the locks
// held by that connection
typedef map<int,list<string>,less<int> > SZGcomponentLockOwnershipDatabase;
SZGcomponentLockOwnershipDatabase componentLockOwnershipDatabase;

// when a lock is released, sometimes other components wish to be notified.
typedef map<string,list<arPhleetNotification>,less<string> > 
  SZGlockNotificationDatabase;
SZGlockNotificationDatabase lockNotificationDatabase;

// organizes the lock release notifications owned by a particular component
typedef map<int,list<string>,less<int> > SZGlockNotificationOwnershipDatabase;
SZGlockNotificationOwnershipDatabase lockNotificationOwnershipDatabase;

// when component goes away, sometimes other components wish to be notified.
typedef map<int,list<arPhleetNotification>,less<int> > 
  SZGkillNotificationDatabase;
SZGkillNotificationDatabase killNotificationDatabase;

// the kill notifications owned by a particular component. the list<int> is
// of the observed component IDs (i.e. the components that we are watching
// to see if they go away)
typedef map<int,list<int>,less<int> > SZGkillNotificationOwnershipDatabase;
SZGkillNotificationOwnershipDatabase killNotificationOwnershipDatabase;

//**************************************************
// end of global data storage used by the szgserver
//**************************************************


//**************************************************
// Misc. utility functions
//**************************************************

void _transferMatchFromTo(arStructuredData* from, arStructuredData* to){
  int match = from->getDataInt(lang.AR_PHLEET_MATCH);
  to->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
}

void SZGactivateUser(const string& userName){
  // normal user
  SZGuserDatabase::iterator i = userDatabase.find(userName);
  if (i != userDatabase.end()){
    valueContainer = i->second;
  }
  else{
    // better insert a new parameter database for this user
    SZGparamDatabase* newDatabase = new SZGparamDatabase;
    userDatabase.insert(SZGuserDatabase::value_type(userName,newDatabase));
    valueContainer = newDatabase;
  }
}

//********************************************************************
// functions dealing with manipulating the message databases
//********************************************************************

/// A helper function for the larger message database manipulators.
/// This inserts a given message ID into the list maintained for a given
/// component (which is the list of message IDs to which the component is
/// expected to respond).
void SZGinsertMessageIDIntoComponentsList(int componentID, int messageID){
  SZGcomponentMessageOwnershipDatabase::iterator
    i(componentMessageOwnershipDatabase.find(componentID));
  if (i == componentMessageOwnershipDatabase.end()){
    // the component owns no messages currently
    list<int> messageList;
    messageList.push_back(messageID);
    componentMessageOwnershipDatabase.insert
      (SZGcomponentMessageOwnershipDatabase::value_type(componentID,
							messageList));
  }
  else{
    i->second.push_back(messageID);
  }
}

/// A helper function for the larger message database manipulators.
/// Removes the given message ID from the list maintained for the
/// given component (which is the list of message IDs to which the component
/// is expected to respond).
void SZGremoveMessageIDFromComponentsList(int componentID, int messageID){
  SZGcomponentMessageOwnershipDatabase::iterator
    i(componentMessageOwnershipDatabase.find(componentID));
  if (i == componentMessageOwnershipDatabase.end()){
    cerr << "szgserver internal error: no message list for component "
         << componentID << ".\n";
  }
  else{
    i->second.remove(messageID);
  }
}

/// A helper function for the larger message database manipulators.
/// Adds the given key to the list maintained for the given component
/// (which is the list of keys on which the component has initiated trades)
void SZGinsertKeyIntoComponentsList(int componentID, const string& key){
  SZGcomponentTradingOwnershipDatabase::iterator
    i(componentTradingOwnershipDatabase.find(componentID));
  if (i == componentTradingOwnershipDatabase.end()){
    // the component has posted no trades as of yet
    list<string> tradeList;
    tradeList.push_back(key);
    componentTradingOwnershipDatabase.insert
      (SZGcomponentTradingOwnershipDatabase::value_type(componentID,
							tradeList));
  }
  else{
    i->second.push_back(key);
  }
}

/// Helper function for the message database manipulators.
/// @param componentID A component
/// @param key Key to be removed from the list of keys on which the component has initiated trades
void SZGremoveKeyFromComponentsList(int componentID, const string& key){
  SZGcomponentTradingOwnershipDatabase::iterator
    i(componentTradingOwnershipDatabase.find(componentID));
  if (i == componentTradingOwnershipDatabase.end()){
    cerr << "szgserver internal error: no trading key in component's list.\n";
  }
  else{
    i->second.remove(key);
  }
}

/// Add a message to the internal database storage.
/// @param messageID ID of the message
/// @param componentOwnerID ID of the component which has the right
/// to respond to this message
/// @param componentOriginatorID ID of the component which sent the
/// message
void SZGaddMessageToDatabase(int messageID, 
                             int componentOwnerID,
                             int componentOriginatorID,
                             int match){
  arPhleetMessage message;
  message.messageID = messageID;
  message.messageOwner = componentOwnerID;
  message.responseDestination = componentOriginatorID;
  message.match = match;
  messageOwnershipDatabase.insert(SZGmessageOwnershipDatabase::value_type
				  (messageID, message));
  // Add to the component's list of owned messages.
  SZGinsertMessageIDIntoComponentsList(componentOwnerID, messageID);
}

/// Return the ID of the component that owns this message
/// @param messageID ID of the message
int SZGgetMessageOwnerID(int messageID){
  SZGmessageOwnershipDatabase::iterator
    i(messageOwnershipDatabase.find(messageID));
  return (i == messageOwnershipDatabase.end()) ? -1 : i->second.messageOwner;
}

/// Return the match corresponding to the original message. Helpful when
/// we are going to send a response back to the originator. It that case,
/// we need to fill in the original match so that the async stuff on the
/// arSZGClient side can route the messages correctly.
int SZGgetMessageMatch(int messageID){
  SZGmessageOwnershipDatabase::iterator
    i(messageOwnershipDatabase.find(messageID));
  return (i == messageOwnershipDatabase.end()) ? -1 : i->second.match;
} 

/// Return the ID of the component that originated this message
/// (and to which the response needs to be sent).
/// @param messageID ID of the message
int SZGgetMessageOriginatorID(int messageID){
  SZGmessageOwnershipDatabase::iterator
    i(messageOwnershipDatabase.find(messageID));
  return (i == messageOwnershipDatabase.end()) 
         ? -1 : i->second.responseDestination ;
}

/// Remove the message with the given ID from the database.
/// Since a message is entered in the database only when the sender requests
/// a response, this function is used when
/// a response to a message is received at the szgserver.
/// @param messageID ID of the message
bool SZGremoveMessageFromDatabase(int messageID){
  SZGmessageOwnershipDatabase::iterator
    i(messageOwnershipDatabase.find(messageID));
  if (i == messageOwnershipDatabase.end()){
    cerr << "szgserver warning: ignoring request to remove nonexistant "
	 << "message.\n";
    return false;
  }

  const int componentMessageOwner = i->second.messageOwner;
  messageOwnershipDatabase.erase(i);
  // remove this from the list associated with the component
  SZGremoveMessageIDFromComponentsList(componentMessageOwner, messageID);
  return true;
}

/// Initiate an "ownership trade", e.g.
/// when szgd wants to let the launched executable respond to
/// the "dex" command that launched it.
/// @param key Value by which the trade is indexed
/// @param messageID ID of the message
/// @param requestingComponentID ID of the component requesting that
/// the trade be posted
/// @param tradingMatch this is used to respond to the component that
/// initiated the trade when said trade has been completed.
bool SZGaddMessageTradeToDatabase(string key, 
                                  int messageID,
				  int requestingComponentID,
                                  int tradingMatch){
  SZGmessageOwnershipDatabase::iterator
    i(messageOwnershipDatabase.find(messageID));
  if (i == messageOwnershipDatabase.end()){
    cerr << "szgserver warning: trade cannot be started since no message "
	 << "with that ID exists.\n";
    return false;
  }
  const int responseOwner = i->second.messageOwner;
  if (responseOwner != requestingComponentID){
    cerr << "szgserver warning: trade cannot be started since the message "
	 << "is not owned by the component asking to start the trade.\n";
    return false;
  }
  SZGmessageTradingDatabase::iterator j(messageTradingDatabase.find(key));
  if (j != messageTradingDatabase.end()){
    // a trade has already been posted with this key... failure
    cerr << "szgserver warning: trade cannot be started since an "
	 << "identical key already exists.\n";
    return false;
  }
  // yes, we can, in fact, start to do the trade.
  i->second.tradingMatch = tradingMatch;
  messageTradingDatabase.insert(SZGmessageTradingDatabase::value_type
				(key, i->second));
  // Remove the corresponding data from the message ownership database.
  messageOwnershipDatabase.erase(i);

  // remove the corresponding data from the component's list
  SZGremoveMessageIDFromComponentsList(responseOwner, messageID);

  // add the trading key to the component's list of such things
  SZGinsertKeyIntoComponentsList(responseOwner, key);

  return true;
}

/// Complete a message ownership trade.  Return false on error.
/// @param key Value on which the trade was posted
/// @param newOwnerID ID of the component which will henceforth own
/// the right to respond to the message
/// @param message Gives the various attributes of the phleet message
bool SZGmessageRequest(const string& key, int newOwnerID,
                       arPhleetMessage& message){
  SZGmessageTradingDatabase::iterator j(messageTradingDatabase.find(key));
  if (j == messageTradingDatabase.end()){
    // no trade has been posted on this key... failure
    cerr << "szgserver warning: cannot trade message on non-existant key="
	 << key << ".\n";
    return false;
  }

  // fill-in messageData with the relevant values
  message = j->second;
  // must get the old owner before overwriting
  int oldOwner = message.messageOwner;
  // the change is that someone else owns the message now.
  message.messageOwner = newOwnerID;
  // remove from the trading database
  messageTradingDatabase.erase(j);
  // make the component requesting the trade the new message owner
  messageOwnershipDatabase.insert(SZGmessageOwnershipDatabase::value_type
    (message.messageID,message));

  // remove the key from the list associated with the trading component
  // message.messageOwner used to be where oldOwner is now
  SZGremoveKeyFromComponentsList(oldOwner, key);

  // add the messageID to the list associated with the new owner
  SZGinsertMessageIDIntoComponentsList(newOwnerID,message.messageID);

  return true;
}

/// Revoke a message trade that has yet to be completed, e.g.
/// when szgd fails to launch an executable, in which
/// case szgd wants to respond directly to the "dex" command.
/// @param key Value on which the message trade was posted
/// @param revokerID ID of the component that is requesting the trade
/// revocation
bool SZGrevokeMessageTrade(const string& key, int revokerID){
  SZGmessageTradingDatabase::iterator j(messageTradingDatabase.find(key));
  // is there a message with this key?
  if (j == messageTradingDatabase.end()){
    cerr << "szgserver warning: attempted to revoke a message trade on a "
	 << "nonexistant key=" << key <<".\n";
    return false;
  }
  // A message with this key exists, but are we allowed to revoke?
  arPhleetMessage message = j->second;
  if (message.messageOwner != revokerID){
    cerr << "szgserver warning: "
	 << "component unauthorized to revoke a message trade.\n";
    return false;
  }

  // remove the trade from the database
  messageTradingDatabase.erase(j);
  // restore the original message owenership
  messageOwnershipDatabase.insert(SZGmessageOwnershipDatabase::value_type
				  (message.messageID,message));
  // enter the message ID back into the ownership list of the component
  SZGinsertMessageIDIntoComponentsList(message.messageOwner,
				       message.messageID);
  // remove the key from the trading list associated with this component
  SZGremoveKeyFromComponentsList(message.messageOwner, key);
  return true;
}

/// Get the message info, if such exists, from the trading database.
/// Return whether or not a trade exists on that key. If so, fill in the
/// passed message object with the data.
bool SZGgetMessageTradeInfo(const string& key, arPhleetMessage& message){
  SZGmessageTradingDatabase::iterator j(messageTradingDatabase.find(key));
  // is there a message with this key?
  if (j == messageTradingDatabase.end()){
    return false;
  }
  message = j->second;
  return true;
}

//********************************************************************
// funtions dealing with the locks
//********************************************************************

/// Request a named lock.
/// Returns true iff no errors occur (the lock is granted).
/// @param lockName Name of the lock.
/// @param id ID of component requesting the lock
/// @param ownerID Set to -1 if the lock was not previously held,
/// otherwise set to the ID of the holding component.
bool SZGgetLock(const string& lockName, int id, int& ownerID){
  SZGlockOwnershipDatabase::iterator
    i(lockOwnershipDatabase.find(lockName));
  if (i != lockOwnershipDatabase.end()){
    ownerID = i->second;
    // note: we do not want to print out a warning here... since this
    // is actually a fairly common occurence
    return false;
  }

  // Nobody holds the lock currently. Go ahead and insert it.
  // First in the global list.
  lockOwnershipDatabase.insert(SZGlockOwnershipDatabase::value_type
			       (lockName, id));
  // Insert the name in the list associated with this component.
  SZGcomponentLockOwnershipDatabase::iterator
    j(componentLockOwnershipDatabase.find(id));
  if (j == componentLockOwnershipDatabase.end()){
    // The component has never owned a lock.
    list<string> lockList;
    lockList.push_back(lockName);
    componentLockOwnershipDatabase.insert
      (SZGcomponentLockOwnershipDatabase::value_type
       (id, lockList));
  }
  else{
    // The component either owns, or has owned, a lock.
    j->second.push_back(lockName);
  }
  ownerID = -1;
  return true;
}

/// Returns true if the lock is currently held and false otherwise
bool SZGcheckLock(const string& lockName){
  SZGlockOwnershipDatabase::iterator i(lockOwnershipDatabase.find(lockName));
  if (i == lockOwnershipDatabase.end()){
    return false;
  }
  return true;
}

/// Enters a request for notification when a given lock, currently held,
/// is released.
/// BUG BUG BUG BUG BUG BUG BUG: If a given component asks for multiple
///  notifications on the same lock name, then it will receive only the
///  first!
void SZGrequestLockNotification(int componentID, string lockName,
                                int match){
  arPhleetNotification notification;
  notification.componentID = componentID;
  notification.match = match;
  // enter it into the list organized by lock name
  SZGlockNotificationDatabase::iterator i
    = lockNotificationDatabase.find(lockName);
  if (i == lockNotificationDatabase.end()){
    // no notifications, yet, for this lock's release
    list<arPhleetNotification> temp;
    temp.push_back(notification);
    lockNotificationDatabase.insert(SZGlockNotificationDatabase::value_type
				    (lockName, temp));
  }
  else{
    // there are already notifications requested for this lock
    i->second.push_back(notification);
  }
  // we must also enter it into the list organized by component ID
  SZGlockNotificationOwnershipDatabase::iterator j
    = lockNotificationOwnershipDatabase.find(componentID);
  if (j == lockNotificationOwnershipDatabase.end()){
    // no notifications directed at this component
    list<string> tempS;
    tempS.push_back(lockName);
    lockNotificationOwnershipDatabase.insert
      (SZGlockNotificationOwnershipDatabase::value_type(componentID, tempS));
  }
  else{
    // there are already notifications directed towards this component
    j->second.push_back(lockName);
  }
}

/// Sends the lock release notifications, if any. NOTE: this is ALWAYS called
/// upon lock release. Because of some technicalities about the way we use
/// call, we need to be able to use the data server's regular methods 
/// (in the case of serverLock = false) or data server's "no lock" methods
/// (in the case of serverLock = true).
void SZGsendLockNotification(string lockName, bool serverLock){
  SZGlockNotificationDatabase::iterator i
    = lockNotificationDatabase.find(lockName);
  if (i != lockNotificationDatabase.end()){
    // there are actually some notifications
    arStructuredData* data 
      = dataParser->getStorage(lang.AR_SZG_LOCK_NOTIFICATION);
    data->dataInString(lang.AR_SZG_LOCK_NOTIFICATION_NAME, lockName);
    for (list<arPhleetNotification>::iterator j = i->second.begin();
	 j != i->second.end(); j++){
      // Must set the match.
      data->dataIn(lang.AR_PHLEET_MATCH, &(j->match), AR_INT, 1);
      arSocket* theSocket = NULL;
      // we use too different calls, sendData and sendDataNoLock,
      // depending upon the context in which we were called.
      if (serverLock){
        theSocket = dataServer->getConnectedSocketNoLock(j->componentID);
      }
      else{
        theSocket = dataServer->getConnectedSocket(j->componentID);
      }
      if (!theSocket){
	cerr << "szgserver warning: wanted to send lock notification to "
	     << "nonexistant component.\n";
      }
      else{
	// we use too different calls, sendData and sendDataNoLock,
	// depending upon the context in which we were called.
	if (serverLock){
          if (!dataServer->sendDataNoLock(data, theSocket)){
	    cerr << "szgserver warning: "
		 << "failed to send no-lock notification.\n";
	  }
	}
	else{
          if (!dataServer->sendData(data, theSocket)){
	    cerr << "szgserver warning: failed to send lock notification.\n";
	  }
	}
      }
      // now, we need to do a little clean-up...
      // THIS IS VERY, VERY INEFFICIENT. TODO TODO TODO TODO TODO TODO TODO
      SZGlockNotificationOwnershipDatabase::iterator k
	= lockNotificationOwnershipDatabase.find(j->componentID);
      if (k != lockNotificationOwnershipDatabase.end()){
        k->second.remove(lockName);
	// if the list is empty, better remove it.
        if (k->second.empty()){
          lockNotificationOwnershipDatabase.erase(k);
	}
      }
      else{
	cerr << "szgserver error: "
	     << "found no expected lock notification owner.\n";
      }
    }
    // Must remember to recycle the storage!
    dataParser->recycle(data);
    // finally, the lock has gone away, so remove its entry
    lockNotificationDatabase.erase(i);
  }
}

/// A component is going away. Remove any outstanding lock notification
/// requests from internal storage.
void SZGremoveComponentLockNotifications(int componentID){
  SZGlockNotificationOwnershipDatabase::iterator i
    = lockNotificationOwnershipDatabase.find(componentID);
  if (i != lockNotificationOwnershipDatabase.end()){
    // this component has some notifications
    for (list<string>::iterator j = i->second.begin();
	 j != i->second.end(); j++){
      // WOEFULLY INEFFICIENT... TODO TODO TODO TODO TODO TODO
      SZGlockNotificationDatabase::iterator k
	= lockNotificationDatabase.find(*j);
      if (k != lockNotificationDatabase.end()){
	// AARGH! Binding the component ID and the match together makes
	// this step more inefficient!
        list<arPhleetNotification>::iterator l = k->second.begin();
	while (l != k->second.end()){
          if (l->componentID == componentID){
            l = k->second.erase(l);
	  }
	  else{
	    l++;
	  }
	}
	// if the list is empty, better remove it!
	if (k->second.empty()){
	  lockNotificationDatabase.erase(k);
	}
      }
      else{
	cerr << "szgserver error: found no lock notification, "
	     << "needed by component list.\n";
      }
    }
    // finally, the component has gone away... so remove its info
    lockNotificationOwnershipDatabase.erase(i);
  }
}

/// Release a named lock.
/// Returns false on error (component doesn't own the lock).
/// NOTE: this is only called when a component explcitly requests
/// a lock be released. Consequently, we call a version of
/// SZGsendLockNotification(...) that uses the normal methods of arDataServer
/// (i.e. NOT the "no lock" methods).
/// @param lockName Name of the lock.
/// @param id ID of component releasing the lock
bool SZGreleaseLock(const string& lockName, int id){
  SZGlockOwnershipDatabase::iterator
    i(lockOwnershipDatabase.find(lockName));
  if (i == lockOwnershipDatabase.end()){
    // Lock is not held.
    cerr << "szgserver warning: already released lock "
	 << lockName << " as requested by component" << id
	 << " (" << dataServer->getSocketLabel(id)
	 << ").\n";
    return false;
  }

  if (i->second != id){
    // Lock is held by another component
    // component requesting the release doesn't own the lock
    cerr << "szgserver warning: failed to release lock "
	 << lockName << ": not held by component" << id
	 << " (" << dataServer->getSocketLabel(id)
	 << ").\n";
    return false;
  }

  // Remove the lock from the database.
  lockOwnershipDatabase.erase(i);

  // Remove the lock from the component's database.
  SZGcomponentLockOwnershipDatabase::iterator
    j(componentLockOwnershipDatabase.find(id));
  if (j == componentLockOwnershipDatabase.end()){
    cerr << "szgserver internal error: lock list missing on release.\n";
    return false;
  }

  j->second.remove(lockName);
  // if we've gotten this far, the lock has indeed been released.
  SZGsendLockNotification(lockName, false);
  return true;
}

/// Release all locks held by one component. NOTE: this method is ONLY
/// called when a component is removed from the database. Hence, we
/// call a version of SZGsendLockNotification(...) that uses the data
/// server's "no lock" methods.
/// @param id ID of component releasing the locks
void SZGreleaseLocksOwnedByComponent(int id){
  // (Too late to call dataServer->getSocketLabel(id).)
  // Get the lockList.
  SZGcomponentLockOwnershipDatabase::iterator
    i(componentLockOwnershipDatabase.find(id));
  if (i == componentLockOwnershipDatabase.end()){
    return;
  }
  for (list<string>::iterator k=i->second.begin(); k != i->second.end(); k++){
    const string lockName(*k);
    // erase it from the global storage
    SZGlockOwnershipDatabase::iterator j=lockOwnershipDatabase.find(lockName);
    if (j == lockOwnershipDatabase.end()){
      cerr << "szgserver internal error: found no lock name to remove on "
	   << "component shut down.\n";
    }
    else{
      SZGsendLockNotification(lockName, true);
      lockOwnershipDatabase.erase(j);
    }
  }
  // erase the component's lock list
  componentLockOwnershipDatabase.erase(i);
}

//********************************************************************
// Functions dealing with kill notifications.
// NOTE: THESE FUNCTIONS ARE ESSENTIALLY THE SAME AS THE LOCK RELEASE
// NOTIFICATIONS!
//********************************************************************

/// Enters a request for notification when a particular component goes
/// away. This is useful for efficient notification of a kill's success.
void SZGrequestKillNotification(int requestingComponentID, 
                                int observedComponentID,
                                int match){
  arPhleetNotification notification;
  notification.componentID = requestingComponentID;
  notification.match = match;
  // enter it into the list indexed by the observed component ID
  // (i.e. the component for whose demise we are waiting)
  SZGkillNotificationDatabase::iterator i
    = killNotificationDatabase.find(observedComponentID);
  if (i == killNotificationDatabase.end()){
    // no notifications, yet, for this component's demise
    list<arPhleetNotification> temp;
    temp.push_back(notification);
    killNotificationDatabase.insert(SZGkillNotificationDatabase::value_type
				    (observedComponentID, temp));
  }
  else{
    // there are already notifications requested for this component's demise
    i->second.push_back(notification);
  }
  // we must also enter it into the list organized by the REQUESTING
  // component's ID.
  SZGkillNotificationOwnershipDatabase::iterator j
    = killNotificationOwnershipDatabase.find(requestingComponentID);
  if (j == killNotificationOwnershipDatabase.end()){
    // no kill notifications directed at this component yet
    list<int> tempI;
    // We need to know how to remove the notification request from the
    // OBSERVED component's database if the REQUESTING component goes away
    // before the observed component does.
    tempI.push_back(observedComponentID);
    killNotificationOwnershipDatabase.insert
      (SZGkillNotificationOwnershipDatabase::value_type
        (requestingComponentID, tempI));
  }
  else{
    // there are already notifications directed towards this REQUESTING 
    // component. REMEMBER: we want to be able to refer back to the 
    // OBSERVED component (which is index by which we store the notification
    // requests.
    j->second.push_back(observedComponentID);
  }
}

/// Sends the kill release notifications, if any. This is ALWAYS called when
/// the component exits. Because of some technicalities about the way we use
/// call, we need to be able to use the data server's regular methods 
/// (in the case of serverLock = false) or data server's "no lock" methods
/// (in the case of serverLock = true).
void SZGsendKillNotification(int observedComponentID, bool serverLock){
  SZGkillNotificationDatabase::iterator i
    = killNotificationDatabase.find(observedComponentID);
  if (i != killNotificationDatabase.end()){
    // there are actually some notifications
    arStructuredData* data 
      = dataParser->getStorage(lang.AR_SZG_KILL_NOTIFICATION);
    data->dataIn(lang.AR_SZG_KILL_NOTIFICATION_ID, &observedComponentID,
		 AR_INT, 1);
    // This component has a list of other components that wish to be
    // notified when it exits.
    for (list<arPhleetNotification>::iterator j = i->second.begin();
	 j != i->second.end(); j++){
      // Must set the match.
      data->dataIn(lang.AR_PHLEET_MATCH, &(j->match), AR_INT, 1);
      arSocket* theSocket = NULL;
      // we use too different calls, sendData and sendDataNoLock,
      // depending upon the context in which we were called.
      // NOTE: the componentID held by the arPhleetNotification is the
      // ID of the REQUESTING component.
      if (serverLock){
        theSocket = dataServer->getConnectedSocketNoLock(j->componentID);
      }
      else{
        theSocket = dataServer->getConnectedSocket(j->componentID);
      }
      if (!theSocket){
	cerr << "szgserver warning: wanted to send kill notification to "
	     << "nonexistant component.\n";
      }
      else{
	// we use too different calls, sendData and sendDataNoLock,
	// depending upon the context in which we were called.
	if (serverLock){
          if (!dataServer->sendDataNoLock(data, theSocket)){
	    cerr << "szgserver warning: "
		 << "failed to send no-lock kill notification.\n";
	  }
	}
	else{
          if (!dataServer->sendData(data, theSocket)){
	    cerr << "szgserver warning: failed to send kill notification.\n";
	  }
	}
      }
      // now, we need to do a little clean-up...
      // THIS IS VERY, VERY INEFFICIENT. TODO TODO TODO TODO TODO TODO TODO
      // NOTE: this notification database is organized via REQUESTING
      // component ID (and j->componentID is REQUESTING COMPONENT ID)
      SZGkillNotificationOwnershipDatabase::iterator k
	= killNotificationOwnershipDatabase.find(j->componentID);
      if (k != killNotificationOwnershipDatabase.end()){
	// The lists in the killNotificationOwnershipDatabase are
	// of OBSERVED component IDs (i.e. we want to be notified when this
	// goes away)
        k->second.remove(observedComponentID);
	// If this component has no more (other) components whose potential
	// demise it is observing, go ahead and remove the list.
        if (k->second.empty()){
          killNotificationOwnershipDatabase.erase(k);
	}
      }
      else{
	cerr << "szgserver error: "
	     << "found no expected kill notification owner.\n";
      }
    }
    // Must recycle the storage!
    dataParser->recycle(data);
    // finally, we've sent all the kill notifications, so remove the entry.
    killNotificationDatabase.erase(i);
  }
}

/// A component is going away. Remove any outstanding kill notification
/// requests that it owns from internal storage.
void SZGremoveComponentKillNotifications(int requestingComponentID){
  SZGkillNotificationOwnershipDatabase::iterator i
    = killNotificationOwnershipDatabase.find(requestingComponentID);
  if (i != killNotificationOwnershipDatabase.end()){
    // this component has some kill notifications... what we do is go
    // through the it's list (which contains the IDs of the components it is
    // OBSERVING).
    for (list<int>::iterator j = i->second.begin();
	 j != i->second.end(); j++){
      // WOEFULLY INEFFICIENT... TODO TODO TODO TODO TODO TODO
      // NOTE: we find the observed component and look at its list of
      // registered kill notification requests.
      SZGkillNotificationDatabase::iterator k
	= killNotificationDatabase.find(*j);
      if (k != killNotificationDatabase.end()){
	// Remove every notification from the list which is related to the
	// requestingComponentID.
        list<arPhleetNotification>::iterator l = k->second.begin();
	while (l != k->second.end()){
	  // NOTE: the arPhleetNotification's componentID is the 
	  // ID of the component REQUESTING the notification!
          if (l->componentID == requestingComponentID){
            l = k->second.erase(l);
	  }
	  else{
	    l++;
	  }
	}
	// if the list is empty, better remove it!
	if (k->second.empty()){
	  killNotificationDatabase.erase(k);
	}
      }
      else{
	cerr << "szgserver error: found no kill notification, "
	     << "needed by component list.\n";
      }
    }
    // finally, the component has gone away... so remove its info
    killNotificationOwnershipDatabase.erase(i);
  }
}


//********************************************************************
// functions dealing with component management
//********************************************************************

/// The connection broker uses this callback to send the notifications of
/// service release to clients that have requested such. NOTE: this is OK
/// ONLY because it is called (indirectly) from SZGremoveComponentFromDatabase
/// (THIS IS RELATED TO THE CONNECTION BROKER... AND EXISTS IN CALLBACK
///  FORM BECAUSE THE CONNECTION BROKER CANNOT ITSELF DO EVERYTHING THAT IS
///  NECESSARY TO RELEASE A COMPONENT WHEN IT GOES AWAY)
void SZGreleaseNotificationCallback(int componentID,
				    int match,
                                    const string& serviceName){
  // since this is called from within the data server's lock, we have
  // to use the no-lock methods
  arSocket* destinationSocket 
    = dataServer->getConnectedSocketNoLock(componentID);
  if (destinationSocket){
    arStructuredData* data 
      = dataParser->getStorage(lang.AR_SZG_SERVICE_RELEASE);
    // Must propogate the match!
    data->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
    data->dataInString(lang.AR_SZG_SERVICE_RELEASE_NAME, serviceName);
    if (!dataServer->sendDataNoLock(data, destinationSocket)){
      cerr << "szgserver warning: in release notifcation, failed to send "
	   << "to specified socket.\n";
    }
  }
}

/// Clean up when a socket is removed from the database.  If a component dies
/// before replying to a message, szgserver itself must reply (with
/// SZG_FAILURE).  Also clean up message trades, locks, and services offered.
/// @param componentID ID of component which should have sent replies
void SZGremoveComponentFromDatabase(int componentID){
  // FOR THE CURIOUS: WHY ARE THERE sendDataNoLock's in here? Because this
  // is called from the automatic remove socket from data server call,
  // which occurs inside the mutex already.
  // Construct the failure message.
  int messageID = -1;
  int responseDest = -1;
  arStructuredData* messageAdminData = 
    dataParser->getStorage(lang.AR_SZG_MESSAGE_ADMIN);
  (void)messageAdminData->dataIn(lang.AR_SZG_MESSAGE_ADMIN_ID, &messageID,
				 AR_INT, 1);
  (void)messageAdminData->dataInString(lang.AR_SZG_MESSAGE_ADMIN_STATUS,
                                       "SZG_FAILURE");
  (void)messageAdminData->dataInString(lang.AR_SZG_MESSAGE_ADMIN_TYPE,
				       "SZG Response");
  (void)messageAdminData->dataInString(lang.AR_SZG_MESSAGE_ADMIN_BODY, "");
  // first, we need to send "failure" messages to all components expecting
  // a message response from this component
  const SZGcomponentMessageOwnershipDatabase::iterator i =
    componentMessageOwnershipDatabase.find(componentID);
  // The componentMessageOwnershipDatabase gives the messages owned by
  // a particular component.
  if (i != componentMessageOwnershipDatabase.end()){
    for (list<int>::iterator k=i->second.begin(); k!=i->second.end(); k++){
      // go through the list, sending failure messages as we go.
      messageID = *k;
      // we now need to find the component that originated the message
      const SZGmessageOwnershipDatabase::iterator j =
        messageOwnershipDatabase.find(messageID);
      if (j == messageOwnershipDatabase.end()){
        cerr << "szgserver warning: in socket clean-up, failed to find "
	     << "message ID information in database.\n";
      }
      else{
        messageAdminData->dataIn(lang.AR_SZG_MESSAGE_ADMIN_ID, &messageID, 
                                 AR_INT, 1);
	// Must propogate the match!
        messageAdminData->dataIn(lang.AR_PHLEET_MATCH, &(j->second.match),
                                 AR_INT, 1);
	// the first element of the pair is the owner of the response
	// the second element of the pair is the destination
	// to which we should send the message
        responseDest = j->second.responseDestination;
        messageOwnershipDatabase.erase(j);
        // NOTE: we cannot assume that the response destination still exists!
        arSocket* destinationSocket =
          dataServer->getConnectedSocketNoLock(responseDest);
        if (!destinationSocket){
	  cerr << "szgserver warning: vanished destination for message ID "
	       << messageID << ".\n";
	}
	else{
          dataServer->sendDataNoLock(messageAdminData, destinationSocket);
	}
      }
    }
    componentMessageOwnershipDatabase.erase(i);
  }

  // Remove all the message trades owned by this component,
  // again sending failure messages to the originating components.
  const SZGcomponentTradingOwnershipDatabase::iterator l =
    componentTradingOwnershipDatabase.find(componentID);
  if (l != componentTradingOwnershipDatabase.end()){
    for (list<string>::iterator n=l->second.begin(); n!=l->second.end(); n++){
      const string messageKey = *n;
      // we now need to find the component that sent the message upon
      // which the trade is posted
      const SZGmessageTradingDatabase::iterator m =
        messageTradingDatabase.find(messageKey);
      if (m == messageTradingDatabase.end()){
        cerr << "szgserver warning: no key info in socket cleanup.\n";
      }
      else{
        messageID = m->second.messageID;
        responseDest = m->second.responseDestination;
	// Must make sure that the match is propogated.
	messageAdminData->dataIn(lang.AR_PHLEET_MATCH,
				 &(m->second.match), AR_INT, 1);
        messageAdminData->dataIn(lang.AR_SZG_MESSAGE_ADMIN_ID, &messageID, 
                                 AR_INT, 1);
        messageTradingDatabase.erase(m);
        // NOTE: we cannot assume that the response destination still exists!
        arSocket* originatingSocket =
          dataServer->getConnectedSocketNoLock(responseDest);
        if (!originatingSocket){
	  cerr << "szgserver warning: on socket cleanup, message destination "
	       << "no longer exists for message ID " << messageID << "\n";
	}
	else{
          dataServer->sendDataNoLock(messageAdminData, originatingSocket);
	}
      }
    }
    componentTradingOwnershipDatabase.erase(l);
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

/// This thread listens on port 4620 for discovery requests (as via dhunt
/// or dconnect) and responds with a broadcast "I am here" packet on
/// port 4620. A positive feedback loop is avoided by including having
/// a flag that indicates whether this is a discovery request or a response.
///
/// What do these packets look like?
///
/// discovery packet (size 200 bytes)
/// bytes 0-3: A version number. Allows us to reject incompatible packets.
/// byte 4: Is this discovery or response? 0 for discovery, 1 for response.
/// bytes 4-131: The requested server name, NULL-terminated string.
/// bytes 132-199: All 0's
/// 
/// response packet (size 200 bytes)
/// bytes 0-3: A version number. Allows us to reject incompatible packets.
/// byte 4: Is this discovery or response? 0 for discovery 1 for response.
/// bytes 5-131: Our name, NULL-terminated string.
/// bytes 132-163: The interface upon which the remote whatnot should
///   connect, NULL-terminated string.
/// bytes 164-199: The port upon which the remote whatnot should connect,
///   NULL-terminated string. (yes, this is way more space than necessary).
///   (in fact, all trailing zeros)
/// @param pv Pointer to a bool that, when set to false, aborts szgserver.
void serverDiscoveryFunction(void* pv){
  char buffer[200];
  ar_stringToBuffer(serverInterface, buffer, sizeof(buffer));
  arSocketAddress incomingAddress;
  incomingAddress.setAddress(NULL, 4620);
  arUDPSocket _socket;
  _socket.ar_create();
  _socket.setBroadcast(true);
  // Allows multiple szgservers to exist on a single box!
  _socket.reuseAddress(true);
  if (_socket.ar_bind(&incomingAddress) < 0){
    cerr << "szgserver error: failed to bind to " << "INADDR_ANY:"
         << 4620
	 << ".\n\t(is another szgserver already running?)\n";
    *(bool *)pv = true; // abort
    return;
  }

  arSocketAddress fromAddress;
  while (true){
    _socket.ar_read(buffer,200,&fromAddress);
    if (!fromAddress.checkMask(serverAcceptMask)){
      // Do not complain if this might be a "response" packet.
      if (buffer[4] != 1){
        cout << "szgserver remark: received a suspicious discovery packet.\n";
        cout << " (IP address = " << fromAddress.getRepresentation() << ")\n";
      }
      // Look for the next discovery packet.
      continue;
    }
    // Check the version number.
    if (!(buffer[0] == 0 && buffer[1] == 0 && 
          buffer[2] == 0 && buffer[3] == 1)){
      cout << "szgserver remark: received a discovery packet with incorrect "
	   << "format from " << fromAddress.getRepresentation() << ".\n";
      continue;
    }
    // Is this a discovery packet or a response packet?
    if (buffer[4] == 1){
      // Response packet. Discard.
      continue;
    }
    const string remoteServerName(buffer+5);
    // Determine whether or not we should respond.
    if (remoteServerName == serverName || remoteServerName == "*"){
      memset(buffer, 0, sizeof(buffer));
      // Put in the version number.
      buffer[3] = 1;
      // This is a response.
      buffer[4] = 1;
      // Put in the szgserver name.
      ar_stringToBuffer(serverName, buffer+5, 127);
      // Put in the szgserver interface.
      ar_stringToBuffer(serverInterface, buffer+132, 32);
      // Put in the szgserver port.
      sprintf(buffer+164,"%i",serverPort);
      // Walk through the server computer's NICs, as defined by the
      // szg.conf file. If one of them has the same broadcast address
      // as the fromAddress, then go ahead and broadcast to that
      // broadcast address.
      bool success = false;
      int i;
      for (i=0; i<computerAddresses.size(); i++){
        arSocketAddress tmpAddress;
        if (!tmpAddress.setAddress(computerAddresses[i].c_str(), 0)){
          cout << "szgserver remark: bad address "
	       << computerAddresses[i] << " in szg.conf.\n";
          continue;
	}
        const string broadcastAddress =
          tmpAddress.broadcastAddress(computerMasks[i].c_str());
        if (broadcastAddress == "NULL"){
	  cout << "szgserver remark: bad mask "
	       << computerMasks[i] << " for address "
	       << computerAddresses[i] << " in szg.conf.\n";
	  continue;
	}
        if (broadcastAddress ==
            fromAddress.broadcastAddress(computerMasks[i].c_str())){
          fromAddress.setAddress(broadcastAddress.c_str(), 4620);
          _socket.ar_write(buffer,200,&fromAddress);
	  success = true;
          break;
	}
      }
      if (!success){
	cout << "szgserver warning: failed to find correct broadcast address "
	     << "for response.\n  szg.conf is misconfigured.\n";
      }
    }
  }
}

/// Callback for accessing a database parameter
/// (or the process table, or a collection of database parameters)
/// @param theData Data record from the client
/// @param dataSocket Socket upon which the request travels
void attributeGetRequestCallback(arStructuredData* theData,
                                 arSocket* dataSocket){
  string value, attribute;
  map<string,string,less<string> >::iterator i;
  // Choose the user database.
  SZGactivateUser(theData->getDataString(lang.AR_PHLEET_USER));
  // The attribute name before it has been manipulated, etc.
  const string attrRaw(theData->getDataString(lang.AR_ATTR_GET_REQ_ATTR));

  // Fill in the data from storage, first getting some place to put it.
  arStructuredData* attrGetResponseData =
    dataParser->getStorage(lang.AR_ATTR_GET_RES);
  // Also, transfer the "match" value from the request to the response.
  _transferMatchFromTo(theData, attrGetResponseData);
  const string type(theData->getDataString(lang.AR_ATTR_GET_REQ_TYPE));
  if (type=="NULL"){
    // Send the process table.
    (void)attrGetResponseData->dataInString(
      lang.AR_ATTR_GET_RES_ATTR, attrRaw);
    (void)attrGetResponseData->dataInString(
      lang.AR_ATTR_GET_RES_VAL, dataServer->dumpConnectionLabels());
  }

  else if (type=="ALL"){
    // Concatenate all members of valueContainer into a return value.
    (void)attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_ATTR, 
                                            attrRaw);
    // BUG BUG BUG BUG BUG BUG BUG
    // This does not (yet) generate a new-style szg dbatch script!
    for (i = valueContainer->begin(); i != valueContainer->end(); ++i){
      // more useful:  output it in dbatch-style format.
      string first = i->first;
      // Replace both slashes with spaces,
      // to make it compatible with the syntax of dbatch.
      unsigned int slash = first.find("/");
      // IMPORTANT NOTE: The attribute might be GLOBAL (in which case it
      // has no slashes!) So... only replace the slashes if they exist!
      if (slash != string::npos){
        first.replace(slash, 1, " ");
        slash = first.find("/");
        first.replace(slash, 1, " ");
        const string& s = first + "   " + i->second + "\n";
        value += s;
      }
      else{
        value += (first + "  " + i->second + "\n");
      }
    }
    (void)attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_VAL, 
                                            value);
  }
  else if (type=="substring"){
    // Send an attribute or a list of attributes passing a substring test.
    attribute = attrRaw;
    value = string("(List):\n");
    for (i = valueContainer->begin(); i != valueContainer->end(); ++i){
      const string s(i->first + "  =  " + i->second + "\n");
      if (s.find(attribute) != string::npos){
	value += s;
      }
    }
    (void)attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_ATTR, 
                                            attribute);
    (void)attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_VAL, value);
  }
  else if (type=="value"){
    attribute = attrRaw; // AARGH! a layer of indirection that is no
                         // longer needed since no more name resolution
    i = valueContainer->find(attribute);
    value = (i == valueContainer->end()) ? string("NULL") : i->second;
    (void)attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_ATTR, 
                                            attribute);
    (void)attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_VAL, value);
  }
  else{
    cout << "szgserver internal error: got incorrect type for attribute "
	 << "get request.\n";
  }

  // Send the record.
  dataServer->sendData(attrGetResponseData, dataSocket);
  // recycle the storage
  dataParser->recycle(attrGetResponseData);
}

/// Callback for setting a parameter in the database.
/// @param theData Record containing the client request
/// @param dataSocket Socket upon which the communication occurred
void attributeSetCallback(arStructuredData* theData,
                          arSocket* dataSocket){
  // Print user data.
  SZGactivateUser(theData->getDataString(lang.AR_PHLEET_USER));
  const string attrRaw(theData->getDataString(lang.AR_ATTR_SET_ATTR));
  const string attribute = attrRaw; // AARGH! unnecessary since no more name
                                    // resolution
  const string value(theData->getDataString(lang.AR_ATTR_SET_VAL));
  const ARint requestType = theData->getDataInt(lang.AR_ATTR_SET_TYPE);

  // Insert the data into the table.
  map<string,string,less<string> >::iterator i
    (valueContainer->find(attribute));
  if (requestType == 0){
    // Remove from the table any data already with this key.
    if (i != valueContainer->end())
      valueContainer->erase(i);
    valueContainer->insert
      (map<string,string,less<string> >::value_type (attribute, value));

    // Acknowledge that all is well. NOTE: all we have to do with the data
    // is fill in the match.
    arStructuredData* connectionAckData 
      = dataParser->getStorage(lang.AR_CONNECTION_ACK);
    _transferMatchFromTo(theData, connectionAckData);
    if (!dataServer->sendData(connectionAckData,dataSocket)){
      cerr << "szgserver warning: AR_ATTR_SET send failed.\n";
    }
    dataParser->recycle(connectionAckData);
  }
  else{
    // Test-and-set.
    string returnString;
    if (i != valueContainer->end()){
      if (i->second == "NULL"){
	// Set the attr's value.
        valueContainer->erase(i);
        valueContainer->insert(
          map<string,string,less<string> >::value_type (attribute, value));
	returnString = value;
      }
      else{
	returnString = string("NULL");
      }
    }
    else{
      valueContainer->insert(
        map<string,string,less<string> >::value_type (attribute, value));
      returnString = value;
    }
    // Return the info, first getting some space to put it in.
    arStructuredData* attrGetResponseData =
      dataParser->getStorage(lang.AR_ATTR_GET_RES);
    _transferMatchFromTo(theData, attrGetResponseData);
    if (!attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_ATTR, 
                                           attribute) ||
        !attrGetResponseData->dataInString(lang.AR_ATTR_GET_RES_VAL, 
                                           returnString) ||
        !dataServer->sendData(attrGetResponseData, dataSocket)){
      cerr << "szgserver warning: AR_ATTR_GET_RES send failed.\n";
    }
    dataParser->recycle(attrGetResponseData);
  }
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

void processInfoCallback(arStructuredData* theData, arSocket* dataSocket){
 
  const string requestType =
    theData->getDataString(lang.AR_PROCESS_INFO_TYPE);
  int theID = -1;
  string theLabel;
  if (requestType == "self"){
    theID = dataSocket->getID();
    theLabel = dataServer->getSocketLabel(theID);
  }
  else if (requestType == "ID"){
    theLabel = theData->getDataString(lang.AR_PROCESS_INFO_LABEL);
    theID = dataServer->getFirstIDWithLabel(theLabel);
  }
  else if (requestType == "label"){
    theID = theData->getDataInt(lang.AR_PROCESS_INFO_ID);
    theLabel = dataServer->getSocketLabel(theID);
  }
  else{
    cerr << "szgserver warning: got unknown type on process info request.\n";
  }
  theData->dataInString(lang.AR_PROCESS_INFO_LABEL, theLabel);
  theData->dataIn(lang.AR_PROCESS_INFO_ID, &theID, AR_INT, 1);
  if (!dataServer->sendData(theData, dataSocket)){
    cerr << "szgserver warning: process info send failed.\n";
  }
}

/// Callback for forwarding an incoming message to its final destination.
/// @param theData Incoming record
/// @param dataSocket Connection upon which we received the data
void messageProcessingCallback(arStructuredData* theData,
                               arSocket* dataSocket){
  // Print user data.
  SZGactivateUser(theData->getDataString(lang.AR_PHLEET_USER));
  bool forward = false; // forward the message?
  // Fill in the fields for the message ack, and
  // send it back to the client who sent us this message.
  arStructuredData* messageAckData 
    = dataParser->getStorage(lang.AR_SZG_MESSAGE_ACK);
  // Must fill in the "match".
  _transferMatchFromTo(theData, messageAckData);
  // Put a default ID into the SZGmessageAckData record.
  int theMessageID = 0;
  messageAckData->dataIn(lang.AR_SZG_MESSAGE_ACK_ID, &theMessageID, 
                         AR_INT, 1);
  // Find the destination component's ID.
  int* dataPtr = (int*) theData->getDataPtr(lang.AR_SZG_MESSAGE_DEST,AR_INT);
  if (!dataPtr){
    // This should never happen, except for incompatible versions.
    cerr << "szgserver warning: ignoring message with null data pointer."
         << "\n\t(Does a client have an incompatible protocol?)\n";
  }
  else{
    // Try to send the message. Query the dataServer to get
    // a communication endpoint with the given ID.
    arSocket* destSocket = dataServer->getConnectedSocket(*dataPtr);
    if (!destSocket){
      // No such endpoint. Hmmm... this really isn't that bizarre of
      // an occurence. DO NOT PRINT ANYTHING!
      // We might, for instance, be trying to message a component that
      // has died for some other reason.
    }
    else{
      // there is a reasonable chance that we'll be able to deliver the
      // message. Assign it an ID.
      // NOTE: THIS IS NOT THREADSAFE... SHOULD ENCAPUSLATE MESSAGEID
      // ASIGNMENT IN A MUTEX PROTECTED FUNCTION.
      theMessageID = nextMessageID;
      nextMessageID++;
      // fill in the message's ID field
      theData->dataIn(lang.AR_SZG_MESSAGE_ID, &theMessageID, AR_INT, 1);
      // check to see if a response has been requested. if so,
      if (theData->getDataInt(lang.AR_SZG_MESSAGE_RESPONSE) > 0){
        // a response has been requested, so record the ID of the
        // component that's allowed to respond, along with the ID of where
        // the response should be routed.
        SZGaddMessageToDatabase(theMessageID, destSocket->getID(),
				dataSocket->getID(),
                                theData->getDataInt(lang.AR_PHLEET_MATCH));
      }
      if (!dataServer->sendData(theData,destSocket)){
        cerr << "szgserver warning: message send failed.\n";
      }
      else{
	// we've probably succeeded in forwarding our message
        forward = true;
      }
    }
  }
  if (!SZGack(messageAckData, forward) ||
     (forward &&
      !messageAckData->dataIn(lang.AR_SZG_MESSAGE_ACK_ID, &theMessageID, 
                              AR_INT, 1)) ||
      !dataServer->sendData(messageAckData,dataSocket)) {
    cerr << "szgserver warning: message ack send failed.\n";
  }
  dataParser->recycle(messageAckData);
}

/// Callback for processing the message admin data, which includes responses
/// @param theData Incoming data record
/// @param dataSocket Connection upon which we received the data
void messageAdminCallback(arStructuredData* theData,
			  arSocket* dataSocket){
  const string messageAdminType 
    = theData->getDataString(lang.AR_SZG_MESSAGE_ADMIN_TYPE);
  bool status = false;
  int messageID = -1;
  int responseOwner = -1;
  int responseDestination = -1;
  arPhleetMessage messageData;
  string key;
  arSocket* responseSocket = NULL;

  // We need somewhere to put the response.
  arStructuredData* messageAckData 
    = dataParser->getStorage(lang.AR_SZG_MESSAGE_ACK);
  // Must propogate the match from message to response.
  _transferMatchFromTo(theData, messageAckData);

  if (messageAdminType == "SZG Response"){
    messageID = theData->getDataInt(lang.AR_SZG_MESSAGE_ADMIN_ID);
    responseOwner = SZGgetMessageOwnerID(messageID);
    if (responseOwner < 0){
      cerr << "szgserver warning: "
	   << "unexpected response for messageID " << messageID << ".\n";
    }
    else{
      // a message with the given ID does exist (and is expecting a response)
      responseDestination = SZGgetMessageOriginatorID(messageID);
      if (responseOwner != dataSocket->getID()){
        cerr << "szgserver warning: illegal response attempt from component "
	     << dataSocket->getID()
	     << " (" << dataServer->getSocketLabel(dataSocket->getID())
	     << "), owner is " << responseOwner << ".\n";
      }
      else{
        responseSocket = dataServer->getConnectedSocket(responseDestination);
        if (!responseSocket){
	  cerr << "szgserver warning: missing response destination.\n";
	}
	else{
	  // Must go ahead and fill in the match.
          int match = SZGgetMessageMatch(messageID);
          theData->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
          if (!dataServer->sendData(theData, responseSocket)){
	    cerr << "szgserver warning: response failed.\n";
	  }
	  else{
	    status = true;
	  }
	}
        // If the message will not be continued (status field is SZG_CONTINUE),
	// remove the message from the database.
        const string responseMode =
          theData->getDataString(lang.AR_SZG_MESSAGE_ADMIN_STATUS);
        if (responseMode == string("SZG_SUCCESS")){
          SZGremoveMessageFromDatabase(messageID);
	}
	else if (responseMode != string("SZG_CONTINUE")){
	  cerr << "szgserver warning: message response received with invalid "
	       << "status field.\n";
	}
      }
    }
  }

  else if (messageAdminType == "SZG Trade Message"){
    messageID = theData->getDataInt(lang.AR_SZG_MESSAGE_ADMIN_ID);
    key = theData->getDataString(lang.AR_SZG_MESSAGE_ADMIN_BODY);
    status = SZGaddMessageTradeToDatabase(key, messageID, dataSocket->getID(),
                                    theData->getDataInt(lang.AR_PHLEET_MATCH));
  }

  else if (messageAdminType == "SZG Message Request"){
    key = theData->getDataString(lang.AR_SZG_MESSAGE_ADMIN_BODY);
    arPhleetMessage oldInfo;
    // NOTE: SZGmessageRequest overwrites the owner ID, which we'll need
    // later. Consequently, we need to preserve the original owner ID here.
    // No need to check the return value or complain. If there is an error,
    // SZGmessageRequest will get that itself.
    SZGgetMessageTradeInfo(key, oldInfo);
    // note that messageData is passed-in via reference in the following call
    if (SZGmessageRequest(key, dataSocket->getID(), messageData)){
      // Notify originator of the trade that the trade has occurred.
      (void)SZGack(messageAckData, true);
      messageID = -1;
      // Put in the match from the original trade.
      messageAckData->dataIn(lang.AR_PHLEET_MATCH, &(messageData.tradingMatch),
                             AR_INT, 1);
      messageAckData->dataIn(lang.AR_SZG_MESSAGE_ACK_ID, &messageID, 
                             AR_INT, 1);
      // We must, of course, send back to the originator of the message trade,
      // not the new owner.
      responseSocket 
        = dataServer->getConnectedSocket(oldInfo.messageOwner);
      if (!responseSocket){
	cerr << "szgserver warning: missing originator of message trade.\n";
      }
      else if (!dataServer->sendData(messageAckData,responseSocket)){
	cerr << "szgserver warning: failed to notify originator about "
	     << "message trade.  Originator may have failed.\n";
      }
      // Fill in the ID field of the record to be sent back to the component
      // requesting the trade with the message's ID.
      // Reuse the messageAck storage.
      messageID = messageData.messageID;
      // Don't forget to put the normal match back in. (THIS WILL BE SENT
      // LATER)
      _transferMatchFromTo(theData, messageAckData);
      messageAckData->dataIn(lang.AR_SZG_MESSAGE_ACK_ID, &messageID, 
                             AR_INT, 1);
      // we succeeded
      status = true;
    }
  }

  else if (messageAdminType == "SZG Revoke Trade"){
    key = theData->getDataString(lang.AR_SZG_MESSAGE_ADMIN_BODY);
    status = SZGrevokeMessageTrade(key, dataSocket->getID());
  }

  // Send an ACK back to the receiving component,
  // with ID field possibly not filled in.
  if (!SZGack(messageAckData, status) ||
      !dataServer->sendData(messageAckData, dataSocket)){
    cerr << "szgserver warning: failed to send message ack.\n";
  }
  // Must recycle the data.
  dataParser->recycle(messageAckData);
}

/// Let a component request notification when another component exits.
void killNotificationCallback(arStructuredData* data,
			      arSocket* dataSocket){
  int componentID = data->getDataInt(lang.AR_SZG_KILL_NOTIFICATION_ID);
  if (!dataServer->getConnectedSocket(componentID)){
    // NO SUCH COMPONENT EXISTS. report back immediately
    if (!dataServer->sendData(data, dataSocket)){
      cerr << "szgserver warning: failed to send kill notification.\n";
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

/// Helper functions for lockRequestCallback, lockReleaseCallback.

string lockRequestInit(arStructuredData* lockResponseData,
                       arStructuredData* theData){
  const string lockName(theData->getDataString(lang.AR_SZG_LOCK_RELEASE_NAME));
  (void)lockResponseData->dataInString(lang.AR_SZG_LOCK_RESPONSE_NAME, 
                                       lockName);
  return lockName;
}

void lockRequestFinish(arStructuredData* lockResponseData,
                       const bool ok,
                       const int ownerID, arSocket* dataSocket){
  if (!lockResponseData->dataIn(lang.AR_SZG_LOCK_RESPONSE_OWNER,
                                &ownerID, AR_INT, 1) ||
      !lockResponseData->dataInString(lang.AR_SZG_LOCK_RESPONSE_STATUS,
                                      szgSuccess(ok)) ||
      !dataServer->sendData(lockResponseData,dataSocket)) {
    cerr << "szgserver warning: lock response send failed.\n";
  }
}

/// Callback to process a lock request.
/// @param theData Incoming data record (lock request)
/// @param dataSocket Connection upon which we received the data
void lockRequestCallback(arStructuredData* theData,
			 arSocket* dataSocket){
  arStructuredData* lockResponseData
    = dataParser->getStorage(lang.AR_SZG_LOCK_RESPONSE);
  // Must propogate the "match".
  _transferMatchFromTo(theData, lockResponseData);
  const string lockName 
    = theData->getDataString(lang.AR_SZG_LOCK_REQUEST_NAME);
  (void)lockResponseData->dataInString(lang.AR_SZG_LOCK_RESPONSE_NAME, 
                                       lockName);
  int ownerID = -1;
  const bool ok = SZGgetLock(lockName, dataSocket->getID(), ownerID);
  lockRequestFinish(lockResponseData, ok, ownerID, dataSocket);
  dataParser->recycle(lockResponseData);
}

/// Process a request to release a lock.
/// @param theData Incoming data record (lock release)
/// @param dataSocket Connection upon which we received the data
void lockReleaseCallback(arStructuredData* theData,
			 arSocket* dataSocket){
  const int ownerID = -1;
  arStructuredData* lockResponseData 
    = dataParser->getStorage(lang.AR_SZG_LOCK_RESPONSE);
  // Must propogate the "match".
  _transferMatchFromTo(theData, lockResponseData);
  const bool ok =
    SZGreleaseLock(lockRequestInit(lockResponseData,theData), 
                                   dataSocket->getID());
  lockRequestFinish(lockResponseData, ok, ownerID, dataSocket);
  dataParser->recycle(lockResponseData);
}

/// Process a request to print all currently held locks
/// @param theData Incoming data record (lock release)
/// @param dataSocket Connection upon which we received the data
void lockListingCallback(arStructuredData* theData,
			 arSocket* dataSocket){
  const int listSize = lockOwnershipDatabase.size();
  int* IDs = new int[listSize];
  int iID = 0;
  arSemicolonString locks;
  for (SZGlockOwnershipDatabase::iterator i=lockOwnershipDatabase.begin();
       i != lockOwnershipDatabase.end(); i++){
    locks /= i->first;
    IDs[iID++] = i->second;
  }
  theData->dataInString(lang.AR_SZG_LOCK_LISTING_LOCKS, locks);
  theData->dataIn(lang.AR_SZG_LOCK_LISTING_COMPONENTS, IDs, AR_INT, listSize);
  if (!dataServer->sendData(theData, dataSocket))
    cerr << "szgserver warning: failed to send lock listing response.\n";
  delete [] IDs;
}

/// Let a component request notification when a lock is released.
void lockNotificationCallback(arStructuredData* data,
			      arSocket* dataSocket){
  const string 
    lockName(data->getDataString(lang.AR_SZG_LOCK_NOTIFICATION_NAME));
  // There is no need to propogate the match in the failure case, since
  // the received message is simply returned.
  if (!SZGcheckLock(lockName)){
    // the lock is NOT currently held, report back immediately
    if (!dataServer->sendData(data, dataSocket)){
      cerr << "szgserver warning: failed to send lock release notification.\n";
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

/// Callback to process a request to register a service
/// @param theData Incoming data record (contains info about the service to
/// be registered)
/// @param dataSocket Connection upon which we received the data
void registerServiceCallback(arStructuredData* theData,
                             arSocket* dataSocket){
  // Check the status field first. This indicates whether we are receiving
  // an initial service registration request OR a retry that the remote
  // component has demanded because it could not use some of the returned ports
  // Unpack the record into easy-to-use variables.
  const int match =
    theData->getDataInt(lang.AR_PHLEET_MATCH);
  const string 
    status(theData->getDataString(lang.AR_SZG_REGISTER_SERVICE_STATUS));
  const string serviceName(
    theData->getDataString(lang.AR_SZG_REGISTER_SERVICE_TAG));
  const string networks(
    theData->getDataString(lang.AR_SZG_REGISTER_SERVICE_NETWORKS));
  const string addresses(
    theData->getDataString(lang.AR_SZG_REGISTER_SERVICE_ADDRESSES));
  const int size = theData->getDataInt(lang.AR_SZG_REGISTER_SERVICE_SIZE);
  const string computer(
    theData->getDataString(lang.AR_SZG_REGISTER_SERVICE_COMPUTER));
  int temp[2];
  theData->dataOut(lang.AR_SZG_REGISTER_SERVICE_BLOCK, temp, AR_INT, 2);
  const int firstPort = temp[0];
  const int blockSize = temp[1];
 
  arPhleetService result;
  arStructuredData* data = NULL;

  if (status == "SZG_TRY"){
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
    if (result.valid){
      // Found an unbound port, and nobody else was using the service tag.
      // Don't fill in the address field:  the service binds to INADDR_ANY.
      data->dataIn(lang.AR_SZG_BROKER_RESULT_PORT, result.portIDs,
                   AR_INT, result.numberPorts);
    }
    if (!dataServer->sendData(data, dataSocket)){
      cerr << "szgserver warning: "
           << "failed to respond to service registration request.\n";
    }
    dataParser->recycle(data);
  }
  else if (status == "SZG_RETRY"){
    // The previously assigned ports failed.  Get new ones.
    result = connectionBroker.retryPorts(dataSocket->getID(), serviceName);
    goto LAgain;
  }
  else if (status == "SZG_SUCCESS"){
    // Confirmation that the client bound to the assigned ports.
    const bool status =
      connectionBroker.confirmPorts(dataSocket->getID(), serviceName);
    data = dataParser->getStorage(lang.AR_SZG_BROKER_RESULT);
    // Propogate the "match".
    data->dataIn(lang.AR_PHLEET_MATCH, &match, AR_INT, 1);
    data->dataInString(lang.AR_SZG_BROKER_RESULT_STATUS, szgSuccess(status));
    if (!dataServer->sendData(data, dataSocket)){
      cerr << "szgserver warning: "
           << "failed to respond to service confirmation.\n";
    }
  
    // Now that the service is truly registered, notify the
    // components that are waiting.
    SZGRequestList waiting = connectionBroker.getPendingRequests(serviceName);
    for (SZGRequestList::iterator i = waiting.begin();
	 i != waiting.end(); i++){
      // We do not want to put a service request on the queue...
      // hence, the final parameter of the requestService(...) call is false.
      arPhleetAddress addr = connectionBroker.requestService(
        i->componentID, i->computer, i->match,
	i->serviceName, i->networks, false);
      // The match for the async call to requestServiceCallback(...)
      int oldMatch = i->match;
      data->dataIn(lang.AR_PHLEET_MATCH, &oldMatch, AR_INT, 1);
      data->dataInString(lang.AR_SZG_BROKER_RESULT_STATUS, 
                         szgSuccess(addr.valid));
      if (addr.valid){
        data->dataInString(lang.AR_SZG_BROKER_RESULT_ADDRESS, addr.address);
        data->dataIn(lang.AR_SZG_BROKER_RESULT_PORT, addr.portIDs, AR_INT,
                     addr.numberPorts);
      }
      arSocket* dest = dataServer->getConnectedSocket(i->componentID);
      if (!dataServer->sendData(data, dest)){
        cerr << "szgserver warning: failed to send async broker result to "
	     << " component " << i->componentID << ".\n";
      }
    }
   
    dataParser->recycle(data);
  }
  else{
    cerr << "szgserver warning: ignoring service registration "
         << "with unexpected status field \"" << status << "\".\n";
  }
}


/// Handles requests for service locations. In the simplest case, the client
/// requests a named service which is currently registered with the szgserver.
/// The server then determines the appropriate network path, returning that
/// to the client.
void requestServiceCallback(arStructuredData* theData,
			    arSocket* dataSocket){
  const string computer 
    = theData->getDataString(lang.AR_SZG_REQUEST_SERVICE_COMPUTER);
  const int match = theData->getDataInt(lang.AR_PHLEET_MATCH);
  const string serviceName 
    = theData->getDataString(lang.AR_SZG_REQUEST_SERVICE_TAG);
  const string networks 
    = theData->getDataString(lang.AR_SZG_REQUEST_SERVICE_NETWORKS);
  const string async 
    = theData->getDataString(lang.AR_SZG_REQUEST_SERVICE_ASYNC);
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
  if (result.valid){
    // got a match
    data->dataInString(lang.AR_SZG_BROKER_RESULT_ADDRESS, result.address);
    data->dataIn(lang.AR_SZG_BROKER_RESULT_PORT, result.portIDs, AR_INT,
                 result.numberPorts);
    if (!dataServer->sendData(data, dataSocket)){
      cerr << "szgserver warning: failed to send broker result in response "
	   << "to service request.\n";
    }
  }
  else{
    // No compatible service. Either no such service currently
    // exists or it is only offered on incompatible networks.
    // Respond if we are in synchronous mode. (otherwise the response will
    // occur in registerServiceCallback(...).
    if (!asyncFlag && !dataServer->sendData(data, dataSocket)){
      cerr << "szgserver warning: failed to send broker result in response "
	   << "to service request.\n";
    }
  }
  
  dataParser->recycle(data);
}

/// Handles requests for total lists of services (a dps analogy)
/// (or for the component IDs of specific ones, as is required when one
/// wants to kill a component offering a particular service so that a new
/// one can start up)
void getServicesCallback(arStructuredData* theData,
			arSocket* dataSocket){
  // WARNING WARNING WARNING WARNING WARNING WARNING WARNING WARNING
  // this method is not threadsafe vis-a-vis the connection broker.
  // no problem right now, since the szgserver executes requests in 
  // sequence.
  // WEIRD. IT SEEMS LIKE WE ARE USING A NEW PIECE OF DATA. WHY NOT
  // SEND IT BACK IN PLACE? 
  const string type(theData->getDataString(lang.AR_SZG_GET_SERVICES_TYPE));
  arStructuredData* data = dataParser->getStorage(lang.AR_SZG_GET_SERVICES);
  // Must propogate the "match".
  _transferMatchFromTo(theData, data);
  if (type == "active"){
    const string 
      serviceName(theData->getDataString(lang.AR_SZG_GET_SERVICES_SERVICES));
    if (serviceName == "NULL"){
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
    else{
      // we must respond with a particular service's ID, if that service 
      // exists, and otherwise return -1
      data->dataInString(lang.AR_SZG_GET_SERVICES_SERVICES, serviceName);
      int result = connectionBroker.getServiceComponentID(serviceName);
      data->dataIn(lang.AR_SZG_GET_SERVICES_COMPONENTS, &result, AR_INT, 1);
    }
  }
  else if (type == "pending"){
    SZGRequestList result = connectionBroker.getPendingRequests();
    const int listSize = result.size();
    int* IDs = new int[listSize];
    int iID = 0;
    arSemicolonString names;
    arSlashString computers;
    for (SZGRequestList::iterator i = result.begin(); i != result.end(); i++){
      names /= i->serviceName;
      computers /= i->computer;
      IDs[iID++] = i->componentID;
    }
    data->dataInString(lang.AR_SZG_GET_SERVICES_SERVICES, names);
    data->dataInString(lang.AR_SZG_GET_SERVICES_COMPUTERS, computers);
    data->dataIn(lang.AR_SZG_GET_SERVICES_COMPONENTS, IDs, AR_INT, listSize);
    delete [] IDs;
  }
  else{
    cerr << "szgserver warning: service listing had invalid request type \""
         << type << "\".\n";
  }

  if (!dataServer->sendData(data, dataSocket))
    cerr << "szgserver warning: failed to send service list.\n";
  dataParser->recycle(data);
}

void serviceReleaseCallback(arStructuredData* theData,
			    arSocket* dataSocket){
  // IMPORTANT NOTE: there are major problems here with atomicity.
  // CURRENTLY, the fact that the szgserver processes messages one-at-a-time
  // saves us. TODO TODO TODO TODO TODO TODO
  // NOTE: since the data is processed in place on failure, no need to
  // explicitly propogate the match. (though there is inside the connection
  // broker).
  const string 
    serviceName(theData->getDataString(lang.AR_SZG_SERVICE_RELEASE_NAME));
  const string 
    computer(theData->getDataString(lang.AR_SZG_SERVICE_RELEASE_COMPUTER));
  // see if the current service is *not* currently held.
  if (!connectionBroker.checkService(serviceName)){
    // immediately respond
    if (!dataServer->sendData(theData, dataSocket)){
      cerr << "szgserver warning: failed to respond to service release.\n";
      return;
    }
  }
  // we will respond later, when the service does, in fact, become available.
  // This is the source of the lack of atomicity. No call to the connection
  // broker should occur between the above and here.
  // BUG: THIS IS NOT ATOMIC AND REALLY SHOULD RESPOND WITH A BOOL.
  connectionBroker.registerReleaseNotification(dataSocket->getID(),
				    theData->getDataInt(lang.AR_PHLEET_MATCH),
				    computer,
				    serviceName);
}

void serviceInfoCallback(arStructuredData* theData,
			 arSocket* dataSocket){
  // NOTE: since we are just sending the same data back, the match does not
  // need to be propogated!
  const string op(theData->getDataString(lang.AR_SZG_SERVICE_INFO_OP));
  const string name(theData->getDataString(lang.AR_SZG_SERVICE_INFO_TAG));
  if (op == "get"){
    theData->dataInString(lang.AR_SZG_SERVICE_INFO_STATUS,
                          connectionBroker.getServiceInfo(name));
    if (!dataServer->sendData(theData, dataSocket)){
      cout << "szgserver warning: failed to send service info.\n";
    }
  }
  else if (op == "set"){
    const string info(theData->getDataString(lang.AR_SZG_SERVICE_INFO_STATUS));
    bool status = connectionBroker.setServiceInfo(dataSocket->getID(),
                                                  name, info);
    const string statusString = status ? "SZG_SUCCESS" : "SZG_FAILURE";
    theData->dataInString(lang.AR_SZG_SERVICE_INFO_STATUS, statusString);
    if (!dataServer->sendData(theData, dataSocket)){
      cout << "szgserver warning: failed to get service info.\n";
    }
  }
  else{
    cout << "szgserver error: got incorrect service info operation = " << op
	 << ".\n";
  }
}

/// Handle receipt of data records from connected arSZGClients.
/// (szgserver processes client requests serially, which may
/// be bad but is hard to change:  see arDataServer.cpp for
/// how locking enforces serialization.)
/// @param theData Parsed record from the client
/// @param dataSocket Connection on which the record was received
void dataConsumptionFunction(arStructuredData* theData, void*,
                             arSocket* dataSocket){
  // Ensure that arDataServer's read thread serializes calls to this function.
  // UNSURE IF THIS CHECK EVEN MAKES SENSE.
  static bool fInside = false;
  if (fInside) {
    cerr << "szgserver internal error: "
	 << "nonserialized dataConsumptionFunction.\n";
    return;
  }
  fInside = true;

  const int theID = theData->getID();
  if (theID == lang.AR_ATTR_GET_REQ){
    // The callback handles propogating the "match"
    attributeGetRequestCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_ATTR_GET_RES){
    // this one shouldn't happen on this side
    cerr << "szgserver warning: ignoring AR_ATTR_GET_RES message.\n";
  }
  else if (theID == lang.AR_ATTR_SET){
    // The callback handles propogating the "match"
    attributeSetCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_CONNECTION_ACK){
    // The connected application wants to set or change its label
    // insert the label into the table.
    dataServer->setSocketLabel(dataSocket,
      theData->getDataString(lang.AR_CONNECTION_ACK_LABEL));
    // This gets the szgserver name as a reply... useful when shipping
    // the szgserver name to a connecting component.
    theData->dataInString(lang.AR_CONNECTION_ACK_LABEL,serverName);
    // NOTE: unnecessary to propogate the "match" since we are just
    // sending back the received data (which already has the match value).
    if (!dataServer->sendData(theData, dataSocket)){
      cerr << "szgserver warning: failed to send connection ack reply.\n";
    }
  }
  else if (theID == lang.AR_KILL){
    // The process with this ID died without our knowledge (it left the
    // socket open, perhaps because the host crashed, perhaps because
    // a wireless network TCP connection was interrupted).
    // So forget about this guy.
    const int id = theData->getDataInt(lang.AR_KILL_ID);
    // No response to this command. (MAYBE THAT SHOULD CHANGE?)
    // Consequently, no reason to propogate the "match".

    // We send the component in question a kill message.
    arSocket* killSocket = dataServer->getConnectedSocket(id);
    if (killSocket){
      // Go ahead and inform the component that it is to be *rudely*
      // shut down (as opposed to the polite messaging way of shutting
      // it down).
      if (!dataServer->sendData(theData, killSocket)){
	cout << "szgserver remark: failed to send kill data to remotely "
	     << "connected socket.\n";
      }
    }
    // Finally, remove the socket from our table.
    // BUG BUG BUG BUG BUG BUG BUG BUG BUG
    // This is really weird. It seems that closing the socket on this
    // side will go ahead and (potentially) prevent the socket on the other
    // side from receiving the kill message we sent! WHY IS THIS?
    // THIS TRULY IS A LITTLE BIT ALARMING!
    if (!dataServer->removeConnection(id)){
      cerr << "szgserver warning: failed to \"kill -9\" process id "
           << id << ".\n";
    }
  }
  else if (theID == lang.AR_PROCESS_INFO){
    // Returns ID and/or label for a specific process.
    // No need to propogate the "match" since the szgserver just
    // returns the received data, with a few fields filled-in.
    processInfoCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_MESSAGE){
    // The callback handles propogating the "match".
    messageProcessingCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_MESSAGE_ADMIN){
    // The callback handles propogating the "match".
    messageAdminCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_KILL_NOTIFICATION){
    // the callback handles propogating the match.
    killNotificationCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_LOCK_REQUEST){
    // The callback handles propogating the "match".
    lockRequestCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_LOCK_RELEASE){
    // The callback handles propogating the "match".
    lockReleaseCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_LOCK_LISTING){
    // The received message just has some fields filled-in and
    // is then returned to sender. Consequently, no need to
    // propogate the match.
    lockListingCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_LOCK_NOTIFICATION){
    // The callback handles propogating the "match". This is only
    // necessary if the lock is currently held.
    lockNotificationCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_REGISTER_SERVICE){
    // The callback handles propogating the "match".
    registerServiceCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_REQUEST_SERVICE){
    // The callback handles propogating the "match", both for immediate
    // responses and for async responses (via the connection broker).
    requestServiceCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_GET_SERVICES){
    // The callback handles propogating the "match".
    getServicesCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_SERVICE_RELEASE){
    // The callback handles propogating the "match".
    serviceReleaseCallback(theData, dataSocket);
  }
  else if (theID == lang.AR_SZG_SERVICE_INFO){
    // The callback handles propogating the match
    serviceInfoCallback(theData, dataSocket);
  }
  else{
    cerr << "szgserver warning: ignoring record with unknown ID " << theID
	 << ".\n  (Version mismatch between szgserver and client?)\n";
  }
  fInside = false;
}

/// arDataServer calls this when a connection goes away
/// (when a read or write call on the socket returns false).
/// @param theSocket Socket whose connection died
void SZGdisconnectFunction(void*, arSocket* theSocket){
  SZGremoveComponentFromDatabase(theSocket->getID());
}

int main(int argc, char** argv){
  if (argc < 3){
    cerr << "usage: szgserver name port [mask.1] ... [mask.n]\n"
	 << "\texample: szgserver yoyodyne 8888\n";
    return 1;
  }
  if (argc > 3){
    for (int i = 3; i < argc; i++){
      serverAcceptMask.push_back(string(argv[i]));
    }
  }

  // If another szgserver on the network has the same name, abort.
  // We can discover this like dhunt's arSZGClientServerResponseThread(),
  // but we (not dhunt's cout) need the result.
  connectionBroker.setReleaseNotificationCallback(
    SZGreleaseNotificationCallback);

  serverName = string(argv[1]);
  /// \todo errorcheck serverPort, so it's outside the block of ports for connection brokering
  serverPort = atoi(argv[2]);
  // Determine serverInterface, so we can tell client where to connect
  // while we bind to INADDR_ANY.
  arPhleetConfigParser parser;
  if (!parser.parseConfigFile()){
    cerr << "szgserver error: syntax error in config file. (Try dconfig.)\n";
    return 1;
  }

  // Find the first address interface in the list. This is the address
  // returned to dhunt and dlogin.
  computerAddresses = parser.getAddresses();
  computerMasks = parser.getMasks();
  if (computerAddresses.empty()){
    cerr << "szgserver error: config file defines no networks.\n";
    return 1;
  }
  serverInterface = computerAddresses[0];

  bool fAbort = false;
  arThread dummy(serverDiscoveryFunction, &fAbort);
  ar_mutex_init(&receiveDataMutex);
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

  // get the data parser going
  dataParser = new arStructuredDataParser(lang.getDictionary());

  // set the various characteristics of the data server
  // we want to bind to INADDR_ANY
  if (!dataServer->setInterface("INADDR_ANY") ||
      !dataServer->setPort(serverPort)){
    cerr << "szgserver error: invalid IP:port "
         << serverInterface  << ":" << serverPort
	 << " for data server.\n";
    return 1;
  }

  dataServer->setConsumerFunction(dataConsumptionFunction);
  dataServer->setConsumerObject(NULL);
  dataServer->smallPacketOptimize(true);
  if (!dataServer->beginListening(lang.getDictionary()))
    return 1;

  // Give any other threads a chance to fail (they will set fAbort).
  ar_usleep(100000);

  while (!fAbort) {
    dataServer->acceptConnection();
  }
  return fAbort ? 1 : 0;
}
