//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arPhleetConnectionBroker.h"

arPhleetConnectionBroker::arPhleetConnectionBroker(){
  ar_mutex_init(&_brokerLock);
  _releaseNotificationCallback = NULL;
}

/// Requests allocation of a block of ports on a given computer for use
/// by a service. Returns an arPhleetService w/ valid field marked true
/// and with the allocated port numbers if successful. Otherwise returns
/// arPhleetAddress with the valid field marked false. This can fail
/// if the named service is already being offered or if ports are not
/// available. 
/// @param componentID the phleet ID of the component requesting the ports
/// @param serviceName the name of the to-be-offered service
/// @param computer the name of the computer on which the service will
/// be offered
/// @param networks a slash-delimited list giving the names of the networks
/// to which the service-hosting computer is attached
/// @param address a slash-delimited list giving the addresses of the NICs
/// (in the same order as the descriptive network names) the service-hosting
/// computer has
/// @param size the number of ports needed for the service
/// @param firstPort the first port in the port pool of the service-hosting
/// computer
/// @param blockSize the size of the port pool of the service-hosting 
/// computer (note that the port pool is contiguous)
arPhleetService arPhleetConnectionBroker::requestPorts(int componentID, 
                                                     const string& serviceName,
                                                     const string& computer, 
                                                     const string& networks,
                                                     const string& addresses, 
                                                     int size,
                                                     int firstPort, 
                                                     int blockSize){
  arPhleetService result;
  ar_mutex_lock(&_brokerLock);
  // does the service already exist? if so, we fail.
  if (_temporaryServices.find(serviceName) != _temporaryServices.end()){
    // NOTE: this is not necessarily an error. For instance,
    // master/slave applications might see if they can get the master
    // service in order to determine who will be the master.
    // DO NOT PRINT ANYTHING.
    result.valid = false;
    ar_mutex_unlock(&_brokerLock);
    return result;
  }
  if (_usedServices.find(serviceName) != _usedServices.end()){
    // DO NOT PRINT ANYTHING. (same reason as above)
    result.valid = false;
    ar_mutex_unlock(&_brokerLock);
    return result;
  }
  // has a computer record been created yet for this computer...
  // if not, create one
  if (_computerData.find(computer) == _computerData.end()){
    arBrokerComputerData temp;
    // need to initialize the computer record
    temp.networks = networks;
    temp.addresses = addresses;
    temp.firstPort = firstPort;
    temp.blockSize = blockSize;
    for (int n=firstPort; n<firstPort+blockSize; n++){
      temp.availablePorts.push_back(n);
    }
    _computerData.insert(SZGComputerData::value_type(computer,temp));
  }
  SZGComputerData::iterator i(_computerData.find(computer));
  // must be check to see if the port block config needs changing
  _resizeComputerPorts(i->second, firstPort, blockSize);
  // create a phleet service record
  arPhleetService tempService;
  tempService.valid = true;
  tempService.info = string("");
  tempService.computer = computer;
  tempService.componentID = componentID;
  tempService.networks = networks;
  tempService.addresses = addresses;
  tempService.numberPorts = size;
  // assign ports and edit the computer record appropriately. also edit
  // the phleet service record.
  // BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG BUG
  // not checking for ports that haven't been issued
  for (int m=0; m<size; m++){
    if (i->second.availablePorts.empty()){
      cout << "arPhleetConnectionBroker error: computer "
	   << i->first << " currently has no available ports.\n";
      result.valid = false;
      // HMMM... not sure that this really exits cleanly...
      // maybe we should release the ports that might have been
      // already assigned??
      ar_mutex_unlock(&_brokerLock);
      return result;
    }
    int port = i->second.availablePorts.front();
    i->second.availablePorts.pop_front();
    tempService.portIDs[m] = port;
    i->second.temporaryPorts.push_back(port);
  }
  // does a record yet exist for this component? if not create. edit in 
  // any case
  if (_componentData.find(componentID) == _componentData.end()){
    arBrokerComponentData tempC;
    tempC.computer = computer;
    _componentData.insert(SZGComponentData::value_type(componentID,tempC));
  }
  SZGComponentData::iterator j(_componentData.find(componentID));
  j->second.temporaryTags.push_back(serviceName);
  for (int nn=0; nn<size; nn++){
    j->second.temporaryPorts.push_back(tempService.portIDs[nn]);
  }
  // put the phleet service record on the temporary service list and return
  // the assigned ports via an arPhleetService
  _temporaryServices.insert(SZGServiceData::value_type(serviceName,
						       tempService));
  
  ar_mutex_unlock(&_brokerLock);
  return tempService;
}

/// The szgserver has no way of knowing if the service will be able to
/// bind to allocated ports (since that depends on activity on a remote
/// computer, which activity might be unconnected to Syzygy). 
/// Consequently, the service may fail to bind to the ports given in
/// requestPorts(...). In that case, the service will try to bind to
/// different ports. The ports it was previously given are returned
/// to be released into the available pool (might just have been a temporary
/// binding problem). And new ports are returned, as above.
/// @param componentID the phleet ID of the component requesting the ports
/// @param serviceName the name of the to-be-offered service
arPhleetService arPhleetConnectionBroker::retryPorts(
    int componentID, const string& serviceName){
  ar_mutex_lock(&_brokerLock);
  // the service should exist in the temporary service list (i.e. where
  // services go that have been temporarily registered, but have not had
  // there ports confirmed) and must be owned by the requesting component.
  const SZGServiceData::iterator i = _temporaryServices.find(serviceName);
  if (i == _temporaryServices.end()){
    cerr << "arPhleetConnectionBroker warning: component attempted to\n"
	 << "retry ports on nonexistant service name.\n";
LAbort:
      arPhleetService result;
      result.valid = false;
      ar_mutex_unlock(&_brokerLock);
      return result;
  }
  if (i->second.componentID != componentID){
    cerr << "arPhleetConnectionBroker warning: component attempted to\n"
	 << "retry ports on service it does not own.\n";
    goto LAbort;
  }
  // find the computer record. if it hasn't already been created, there has
  // been an error. also, find the component record, if it hasn't been 
  // created, this is, again, an error. the ports assigned to the service 
  // are moved from the temporary list to the available list (of the computer
  // record) and are removed from the temporary list of the component record
  const SZGComputerData::iterator j = _computerData.find(i->second.computer);
  if (j == _computerData.end()){
    cerr << "arPhleetConnectionBroker error: computer record not found\n"
	 << "on retry ports request.\n";
    goto LAbort;
  }
  const SZGComponentData::iterator k = _componentData.find(componentID);
  if (k == _componentData.end()){
    cerr << "arPhleetConnectionBroker error: component record not found\n"
	 << "on retry ports request.\n";
    goto LAbort;
  }
  int nn=0;
  for (nn = 0; nn < i->second.numberPorts; nn++){
    // clear port value from the computer's list of temporary ports
    j->second.temporaryPorts.remove(i->second.portIDs[nn]);
    // clear port value from the component's list of temporary ports
    k->second.temporaryPorts.remove(i->second.portIDs[nn]);
    // add the port value to the computer's list of available ports,
    // at the *back* of the list
    // Note how we make sure we are not pushing an invalid port onto
    // the list.
    if (_portValid(i->second.portIDs[nn], j->second)){
      j->second.availablePorts.push_back(i->second.portIDs[nn]);
    }
  }
  // assign ports and edit the computer record, component, and phleet
  // service records
  for (nn = 0; nn < i->second.numberPorts; nn++){
    // NOTE: we make sure that there is, in fact, a port to broker
    if (j->second.availablePorts.empty()){
      cerr << "arPhleetConnectionBroker error: computer "
	   << j->first << " has no available ports.\n";
    goto LAbort;
    }
    const int port = j->second.availablePorts.front();
    j->second.availablePorts.pop_front();
    i->second.portIDs[nn] = port;
    j->second.temporaryPorts.push_back(port);
    k->second.temporaryPorts.push_back(port);
  }
  // return the altered service record
  ar_mutex_unlock(&_brokerLock);
  return i->second;
}

/// The szgserver does not know that the service has successfully bound to
/// the enumerated ports until this method has been executed. When this
/// method is invoked, the service is registered as live with the szgserver.
/// NOTE: this handshake prevents a race condition whereby the szgserver
/// might assign a service some ports and then direct a client to that
/// location BEFORE the service has bound to the ports. Returns true if
/// this process succeeds and false otherwise.
/// @param componentID the phleet ID of the component confirming it has bound
/// to the ports.
/// @param serviceName the name of the offered service
bool arPhleetConnectionBroker::confirmPorts(int componentID, 
                                            const string& serviceName){
  ar_mutex_lock(&_brokerLock);
  // find the service on the temporary list. if it does not exist, or is
  // owned by a different component
  SZGServiceData::iterator i = _temporaryServices.find(serviceName);
  if (i == _temporaryServices.end()){
    cout << "arPhleetConnectionBroker warning: confirmation invoked on\n"
	 << "unknown service name.\n";
    ar_mutex_unlock(&_brokerLock);
    return false;
  }
  if (i->second.componentID != componentID){
    cout << "arPhleetConnectionBroker error: confirmation invoked on\n"
	 << "unowned service name.\n";
    ar_mutex_unlock(&_brokerLock);
    return false;
  }
  // remove the service from the temporary list and place it on the active
  // list. furthermore, update the component record and the computer record
  // appropriately
  string computer = i->second.computer;
  SZGComputerData::iterator j = _computerData.find(computer);
  if (j == _computerData.end()){
    cout << "arPhleetConnectionBroker error: computer record not found\n"
	 << "on confirm ports request.\n";
    ar_mutex_unlock(&_brokerLock);
    return false;
  }
  SZGComponentData::iterator k = _componentData.find(componentID);
  if (k == _componentData.end()){
    cout << "arPhleetConnectionBroker error: component record not found\n"
	 << "on confirm ports request.\n";
    ar_mutex_unlock(&_brokerLock);
    return false;
  }
  arPhleetService tempService = i->second;
  _temporaryServices.erase(i);
  _usedServices.insert(SZGServiceData::value_type(serviceName, tempService));
  for (int nn=0; nn<tempService.numberPorts; nn++){
    // deal with the computer data
    j->second.temporaryPorts.remove(tempService.portIDs[nn]);
    j->second.usedPorts.push_back(tempService.portIDs[nn]);
    // deal with the component data
    k->second.temporaryPorts.remove(tempService.portIDs[nn]);
    k->second.usedPorts.push_back(tempService.portIDs[nn]);
    k->second.temporaryTags.remove(serviceName);
    k->second.usedTags.push_back(serviceName);
  }
  
  // IMPORTANT NOTE: we do not notify components with pending service requests
  // that a compatible service has just been posted. It is the szgserver's
  // job, each time it calls this method, to also issue a 
  // getPendingRequests(...) call.

  // unlock the mutex and return success
  ar_mutex_unlock(&_brokerLock);
  return true;
}

/// Checks to see if a service with the given name exists, either on the
/// temporary list or on the used list. If so, go ahead and return the
/// "info" string managed by that service. If not, return the empty string.
/// NOTE: maybe at some point it would be better to return FAILURE 
/// (since the empty string is also a valid response!) BUG BUG BUG BUG BUG
/// @param serviceName The full name of the service.
string arPhleetConnectionBroker::getServiceInfo(const string& serviceName){
  string result = string("");
  // Must lock the data structure.
  ar_mutex_lock(&_brokerLock);
  SZGServiceData::iterator i = _temporaryServices.find(serviceName);
  // Is the service on the temporary list?
  if (i != _temporaryServices.end()){
    result = i->second.info;
    ar_mutex_unlock(&_brokerLock);
    return result;
  }
  i = _usedServices.find(serviceName);
  // Is the service on the used list?
  if (i != _usedServices.end()){
    result = i->second.info;
    ar_mutex_unlock(&_brokerLock);
    return result;
  }
  // We haven't found the service.
  ar_mutex_unlock(&_brokerLock);
  return result;
}

/// Attempt to set the "info" for a service, as specified by the given
/// (full) service name. This will fail if the service in question does not
/// exist (either on the temporary or used lists) or if the service is not
/// owned by the component with the given ID. It succeeds otherwise and
/// sets the "info" field for the service, returning true.
/// @param componentID The ID of the component requesting the change.
/// @param serviceName The full name of the service.
/// @param info The new info string to be associated with the service in the
///  event of success.
bool arPhleetConnectionBroker::setServiceInfo(int componentID, 
                                              const string& serviceName,
                                              const string& info){
  // Must lock the broker.
  ar_mutex_lock(&_brokerLock);
  SZGServiceData::iterator i = _temporaryServices.find(serviceName);
  // Is the service on the temporary list?
  if (i != _temporaryServices.end()){
    // Does the component in question own it?
    if (i->second.componentID != componentID){
      // No. Return immediately.
      ar_mutex_unlock(&_brokerLock);
      return false;
    }
    // Yes. Set and return.
    i->second.info = info;
    ar_mutex_unlock(&_brokerLock);
    return true;
  }
  i = _usedServices.find(serviceName);
  // Is the service on the used list?
  if (i != _usedServices.end()){
    // Does the component in question own it?
    if (i->second.componentID != componentID){
      // No. Return immediately.
      ar_mutex_unlock(&_brokerLock);
      return false;
    }
    // Yes. Set and return.
    i->second.info = info;
    ar_mutex_unlock(&_brokerLock);
    return true;
  }
  // We haven't found the service.
  ar_mutex_unlock(&_brokerLock);
  return false;
}

/// Used to determine if a service is, at this moment, either on the
/// temporary list or the used list. If so, return true. Otherwise, false. 
/// @param serviceName Name of the service to be checked
bool arPhleetConnectionBroker::checkService(const string& serviceName){
  bool result = false;
  ar_mutex_lock(&_brokerLock);
  SZGServiceData::iterator i = _temporaryServices.find(serviceName);
  if (i != _temporaryServices.end()){
    result = true;
  }
  else{
    i = _usedServices.find(serviceName);
    if (i != _usedServices.end()){
      result = true;
    }
  }
  ar_mutex_unlock(&_brokerLock);
  return result;
}

/// Used when a component requests a service. First, check to see if a service
/// with that name has been offered. Second, see if one of the networks
/// on which the component can communicate is compatible with one on which
/// the service can communicate. If so, generate an arPhleetAddress w/ valid
/// field marked true containing this information. If not, return an
/// arPhleetAddress with valid field marked false. NOTE: in the case of
/// failure, the connection broker actually remembers the request! The
/// service might, for instance, be offered again.
/// @param componentID the phleet ID of the component requesting the service
/// @param computer the name of the computer on which the component runs
/// @param match the "async rpc" message tag... we need to save this in
/// the service request area so that we can make responses with matching
/// tags
/// @param serviceName the name of the service
/// @param networks a slash-delimited list containing the names of the 
/// networks (in order of descending preference) on which the component would
/// like to communicate.
/// @param async if set to true and we can't find the service, then
/// the request is registered on the pending connection queue for later
/// use by the szgserver via the getPendingRequests(...) method
arPhleetAddress arPhleetConnectionBroker::requestService(int    componentID,
						 const string& computer,
                                                 int    match,
                                                 const string& serviceName,
                                                 const arSlashString& networks,
                                                 bool   async){
  arPhleetAddress result;
  ar_mutex_lock(&_brokerLock);
  // see if the service already exists on the active list
  // if not, we need to insert the service request into the notification 
  // queue. return arPhleetAddress with valid field set to false
  const SZGServiceData::iterator i = _usedServices.find(serviceName);
  if ( i == _usedServices.end() ){
    // DO NOT PRINT A WARNING/ERROR MESSAGE HERE! This is a normal
    // turn of events that, for instance, might occur if a client starts
    // before a server
    result.valid = false;
    if (async){
      // must insert into the pending connections queue
      arPhleetServiceRequest temp;
      temp.componentID = componentID;
      temp.computer = computer;
      temp.match = match;
      temp.serviceName = serviceName;
      temp.networks = networks;
      _requestedServices.push_front(temp);// use LIFO under the assumption
                                          // that service requests will be
                                          // filled quickly 
    }
    ar_mutex_unlock(&_brokerLock);
    return result;
  }

  // does a compatible network exist?
  const int numberNetworksClient = networks.size();
  string matchNetwork("NULL");
  string matchAddress;
  for (int n=0; n<numberNetworksClient; ++n){
    const string& testNetwork = networks[n];
    const arPhleetService& service = i->second;
    int numberNetworksServer = service.networks.size();
    for (int m=0; m<numberNetworksServer; ++m){
      if (testNetwork == service.networks[m]){
        matchNetwork = testNetwork;
        matchAddress = service.addresses[m];
        goto LFound;
      }
    }
  }
LFound:
  if (matchNetwork == "NULL"){
    // no compatible network exists.
    result.valid = false;
  }
  else{
    // we've got a match... stuff address and ports into return value
    result.valid = true;
    result.address = matchAddress;
    result.numberPorts = i->second.numberPorts;
    for (int nn=0; nn<i->second.numberPorts; nn++){
      result.portIDs[nn] = i->second.portIDs[nn];
    }
  }
  ar_mutex_unlock(&_brokerLock);
  return result;
}

/// When a client makes a request for a service's address but no service of
/// that name is currently offered, the request is put on a pending queue.
/// When a service is offered, the szgserver must be able to get a list
/// of component IDs that are have requested this service, so that they can
/// be notified of its location. This method provides that list and removes
/// the record of the pending requests from the broker's lists.
/// @param serviceName the name of the service whose pending requests we want
SZGRequestList arPhleetConnectionBroker::getPendingRequests
                                           (const string& serviceName){
  SZGRequestList result;
  ar_mutex_lock(&_brokerLock);
  // push service requests that match the service name onto the queue to
  // be returned
  SZGRequestList::iterator i = _requestedServices.begin();
  while ( i != _requestedServices.end() ){
    if ( i->serviceName == serviceName ){
      result.push_back(*i);
      // removes the current element and returns an iterator to the next
      // element
      i = _requestedServices.erase(i);
    }
    else{
      i++;
    }
  }
  ar_mutex_unlock(&_brokerLock);
  return result;
}

/// Sometimes we actually want all pending service requests
SZGRequestList arPhleetConnectionBroker::getPendingRequests(){
  // TODO TODO TODO TODO TODO TODO TODO TODO
  // Boy, there sure is ALOT of unnecessary copying here
  SZGRequestList result;
  ar_mutex_lock(&_brokerLock);
  SZGRequestList::iterator i = _requestedServices.begin();
  while (i != _requestedServices.end()){
    result.push_back(*i);
    i++;
  }
  ar_mutex_unlock(&_brokerLock);
  return result;
}

/// Returns a ;-delimited list containing all service names
/// (because service names may contain '/'). 
/// In other words, arSlashString's can't nest!
string arPhleetConnectionBroker::getServiceNames(){
  arSemicolonString result;
  ar_mutex_lock(&_brokerLock);
  for (SZGServiceData::iterator i = _usedServices.begin();
       i != _usedServices.end(); i++){
      result /= i->first;
  }
  ar_mutex_unlock(&_brokerLock);
  return result;
}

/// Returns a slash-delimited list containing all computers on which
/// services are running, in the same order as the service names returned in 
/// getServiceNames(...)
arSlashString arPhleetConnectionBroker::getServiceComputers(){
  arSlashString result;
  ar_mutex_lock(&_brokerLock);
  for (SZGServiceData::iterator i = _usedServices.begin();
       i != _usedServices.end(); i++){
    result /= i->second.computer;
  }
  ar_mutex_unlock(&_brokerLock);
  return result;
}

/// Returns the number of services running.
/// @param IDs a pointer reference. This is overwritten by internally
/// allocated memory, containing the component IDs of the various services,
/// arranged in the same order as getServiceNames(...)
int arPhleetConnectionBroker::getServiceComponents(int*& IDs){
  int result = 0;
  ar_mutex_lock(&_brokerLock);
  SZGServiceData::iterator i;
  // lame 2 pass algorithm
  /// \todo STL length method here!
  for (i=_usedServices.begin(); i!=_usedServices.end(); i++)
    result++;
  IDs = new int[result];
  int which = 0;
  for (i=_usedServices.begin(); i!=_usedServices.end(); i++){
    IDs[which++] = i->second.componentID;
  }
  ar_mutex_unlock(&_brokerLock);
  return result;
}

/// Given a particular service name, this returns the owning component ID,
/// if such exists, and otherwise -1
/// @param serviceName the name of the service in which we are interested
int arPhleetConnectionBroker::getServiceComponentID(const string& serviceName){
  int result = -1;
  ar_mutex_lock(&_brokerLock);
  SZGServiceData::iterator i = _usedServices.find(serviceName);
  if (i != _usedServices.end()){
    result = i->second.componentID;
  }
  ar_mutex_unlock(&_brokerLock);
  return result;
}

/// When the particular service name has been released, we must notify
/// the given component. It is important that we pass in the name of the
/// computer on which the component runs... since a component record
/// might be created by this call.
void arPhleetConnectionBroker::registerReleaseNotification(int componentID,
						   int match,
                                                   const string& computer,
						   const string& serviceName){
  ar_mutex_lock(&_brokerLock);
  // See if the service actually exists.
  SZGServiceData::iterator k = _usedServices.find(serviceName);
  if (k == _usedServices.end()){
    return;
  }
  // could be that we actually need to create a component data entry here
  if (_componentData.find(componentID) == _componentData.end()){
    arBrokerComponentData temp;
    temp.computer = computer;
    _componentData.insert(SZGComponentData::value_type(componentID,temp));
  }
  // get the component's data
  SZGComponentData::iterator i = _componentData.find(componentID);
  // go ahead and add the serviceName to the list of services upon whose
  // release the component expects to be notified.
  i->second.releaseTags.push_back(serviceName);
  // Now, add the notification to the service's list
  arPhleetNotification notification;
  notification.componentID = componentID;
  notification.match = match;
  k->second.notifications.push_back(notification);
  ar_mutex_unlock(&_brokerLock);
}

/// Called whenever a service is removed from the "used" pool OR
/// removed from the temporary pool and not promoted to "used". Some
/// clients may wish to be notified when a service suddenly becomes
/// available. This is important in application launching.
/// NOTE: the only way a service is currently removed is via the
/// removeComponent method, but, in the future, a service might very
/// well disappear because of an explicit shutdown, without its
/// component being removed. NOTE: there might be locking problems
/// in the szgserver in this case. Currently, a special lock situation
/// protects our callback _releaseNotificationCallback because it is only
/// called when a client is removed from the database.
/// TODO TODO TODO TODO TODO TODO
void arPhleetConnectionBroker::_removeService(const string& serviceName){
  // NOTE: this method MUST be called from within the scope of
  // a _brokerLock
  SZGServiceData::iterator i = _usedServices.find(serviceName);
  if (i == _usedServices.end()){
    // perhaps this is on the list of temporary services?
    i = _temporaryServices.find(serviceName);
    if (i == _temporaryServices.end()){
      // service didn't actually exist.
      return;
    }
  }
  // If we've reached here, then the service must exist.
  // Step through its list of notifications.
  for (list<arPhleetNotification>::iterator j 
         = i->second.notifications.begin();
       j != i->second.notifications.end(); ++j){
    if (_releaseNotificationCallback){
      _releaseNotificationCallback(j->componentID, j->match, serviceName);
    }
    // we must find the relevant component and remove the service from its
    // release tags
    // TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
    // THIS IS ALL REALLY HORRIBLY INEFFICIENT
    SZGComponentData::iterator k = _componentData.find(j->componentID);
    if (k != _componentData.end()){
      k->second.releaseTags.remove(serviceName);
    }
  }
  // don't forget to remove the service from the list of temporary
  // and used services (it'll only be on one of them)
  _temporaryServices.erase(serviceName);
  _usedServices.erase(serviceName);
}

/// Used when a component exits the system, for clean-up. Necessary since,
/// for instance, that component might have been offering a service, which
/// now needs to be removed from the internal table of active services.
void arPhleetConnectionBroker::removeComponent(int componentID){
  ar_mutex_lock(&_brokerLock);
  // remove any service requests with this component ID
  SZGRequestList::iterator ii = _requestedServices.begin();
  while ( ii != _requestedServices.end() ){
    if ( (*ii).componentID == componentID ){
      // this returns an iterator to the next element in the list
      ii = _requestedServices.erase(ii);
    }
    else{
      ii++;
    }
  }
  // find the component record
  SZGComponentData::iterator i = _componentData.find(componentID);
  if (i == _componentData.end()){
    // DO NOT GIVE A VERBAL WARNING HERE. This method will be called for
    // every component that leaves the system... and it is normal for
    // a component to have no record in the connection broker (think dex)
    ar_mutex_unlock(&_brokerLock);
    return;
  }
  // go ahead and remove all the owned stuff
  // first, remove all temporary service tags
  list<string>::iterator j;
  for (j = i->second.temporaryTags.begin();
       j != i->second.temporaryTags.end();
       j++){
    _removeService(*j);
  }
  // next, remove all used service tags
  for (j = i->second.usedTags.begin();
       j != i->second.usedTags.end();
       j++){
    _removeService(*j);
  }
  // go down the list of release tags and remove this component from all of
  // them
  for (j = i->second.releaseTags.begin();
       j != i->second.releaseTags.end();
       j++){
    // remove the componentID from the service's release list
    // MORE BLATANT INEFFICIENCY!!! TODO TODO TODO TODO TODO TODO
    SZGServiceData::iterator n = _usedServices.find(*j);
    // Go through the list and remove any notifications to this
    // component.
    // BUG: is it really correct to not look at the temporary services
    // as well?
    if (n != _usedServices.end()){
      list<arPhleetNotification>::iterator nn 
        = n->second.notifications.begin();
      while (nn != n->second.notifications.end()){
        if (nn->componentID == componentID)
          nn = n->second.notifications.erase(nn);
	else
	  ++nn;
      }
    }
  }
  // all temporary ports should be returned to the appropriate computer's pool
  // as should all used ports
  const string computer = i->second.computer;
  const SZGComputerData::iterator k = _computerData.find(computer);
  if (k != _computerData.end()){
    list<int>::iterator l;
    for (l = i->second.temporaryPorts.begin();
         l != i->second.temporaryPorts.end();
         l++){
      k->second.temporaryPorts.remove(*l);
      // important that we push them onto the end of the list
      // but only if the port is still valid (the port block
      // might have changed)
      if (_portValid(*l, k->second)){
        k->second.availablePorts.push_back(*l);
      }
    }
    for (l = i->second.usedPorts.begin();
	 l != i->second.usedPorts.end();
	 l++){
      k->second.usedPorts.remove(*l);
      // important that we push them onto the end of the list
      // but only if the port is still valid (the port block
      // might have changed)
      if (_portValid(*l, k->second)){
        k->second.availablePorts.push_back(*l);
      }
    }
  }
  else{
    // this really is an error condition
    cout << "arPhleetConnectionBroker error: on removal of component, could\n"
	 << "not find expected computer record ( "
	 << computer << ").\n";
  }

  // finally, remove the component record
  _componentData.erase(i);
  ar_mutex_unlock(&_brokerLock);
}

/// prints out the entire state of the connection broker. extremely verbose,
/// especially for larger systems.
void arPhleetConnectionBroker::print(){
  list<int>::iterator nn;
  list<string>::iterator mm;
  ar_mutex_lock(&_brokerLock);
  cout << "***************************************************\n";
  for (SZGComputerData::iterator i = _computerData.begin();
       i != _computerData.end();
       i++){
    cout << "computer = " << i->first << "\n";
    cout << "  networks = " << i->second.networks << "\n";
    cout << "  addresses = " << i->second.addresses << "\n";
    cout << "  first port = " << i->second.firstPort << "\n";
    cout << "  block size = " << i->second.blockSize << "\n";
    cout << "  available ports = ";
    for (nn = i->second.availablePorts.begin();
	 nn != i->second.availablePorts.end();
	 nn++){
      cout << *nn << " ";
    }
    cout << "\n";
    cout << "  temporary ports = ";
    for (nn = i->second.temporaryPorts.begin();
	 nn != i->second.temporaryPorts.end();
	 nn++){
      cout << *nn << " ";
    }
    cout << "\n";
    cout << "  used ports = ";
    for (nn = i->second.usedPorts.begin();
	 nn != i->second.usedPorts.end();
	 nn++){
      cout << *nn << " ";
    }
    cout << "\n";
  }
  for (SZGComponentData::iterator j = _componentData.begin();
       j != _componentData.end();
       j++){
    cout << "component ID = " << j->first << "\n";
    cout << "  running on computer = " << j->second.computer << "\n";
    cout << "  temporary service tags = ";
    for (mm = j->second.temporaryTags.begin();
	 mm != j->second.temporaryTags.end();
	 mm++){
      cout << *mm << " ";
    }
    cout << "\n";
    cout << "  used service tags = ";
    for (mm = j->second.usedTags.begin();
	 mm != j->second.usedTags.end();
	 mm++){
      cout << *mm << " ";
    }
    cout << "\n";
    cout << "  service release tags = ";
    for (mm = j->second.releaseTags.begin();
	 mm != j->second.releaseTags.end();
	 mm++){
      cout << *mm << " ";
    }
    cout << "\n";
    cout << "  temporary ports = ";
    for (nn = j->second.temporaryPorts.begin();
	 nn != j->second.temporaryPorts.end();
	 nn++){
      cout << *nn << " ";
    }
    cout << "\n";
    cout << "  used ports = ";
    for (nn = j->second.usedPorts.begin();
	 nn != j->second.usedPorts.end();
	 nn++){
      cout << *nn << "\n";
    }
    cout << "\n";
  }
  SZGServiceData::iterator k;
  for (k = _temporaryServices.begin();
       k != _temporaryServices.end();
       k++){
    cout << "temporary phleet service = " << k->first << "\n";
    cout << "  running on computer = " << k->second.computer << "\n";
    cout << "  owned by component = " << k->second.componentID << "\n";
    cout << "  networks = " << k->second.networks << "\n";
    cout << "  addresses = " << k->second.addresses << "\n";
    cout << "  ports = ";
    for (int n=0; n < k->second.numberPorts; n++){
      cout << k->second.portIDs[n] << " ";
    }
    cout << "\n";
  }
  for (k = _usedServices.begin();
       k != _usedServices.end();
       k++){
    cout << "used phleet service = " << k->first << "\n";
    cout << "  running on computer = " << k->second.computer << "\n";
    cout << "  owned by component = " << k->second.componentID << "\n";
    cout << "  networks = " << k->second.networks << "\n";
    cout << "  addresses = " << k->second.addresses << "\n";
    cout << "  ports = ";
    for (int n=0; n < k->second.numberPorts; n++){
      cout << k->second.portIDs[n] << " ";
    }
    cout << "\n";
  }
  cout << "Pending service requests =\n";
  for (SZGRequestList::iterator l = _requestedServices.begin();
       l != _requestedServices.end();
       ++l){
    cout << "  Request =\n";
    cout << "    Component ID = " << l->componentID << "\n";
    cout << "    Service name = " << l->serviceName << "\n";
    cout << "    Networks = " << l->networks <<"\n";
  }
  ar_mutex_unlock(&_brokerLock);
}

void arPhleetConnectionBroker::
  _resizeComputerPorts(arBrokerComputerData& computer,
                       int first, int size){
  // do not lock this... since it is already called from inside
  // a locked method
  if (computer.firstPort != first ||
      computer.blockSize != size){
    list<int> temp;
    for (int i=first; i<first+size; i++){
      temp.push_back(i);
    }
    // Remove everything in the temporaryPorts and usedPorts lists.
    list<int>::iterator iter;
    for (iter=computer.temporaryPorts.begin(); 
         iter!=computer.temporaryPorts.end(); iter++){
      temp.remove(*iter);
    }
    for (iter=computer.usedPorts.begin(); 
         iter!=computer.usedPorts.end(); iter++){
      temp.remove(*iter);
    }
    // we have a new list of available ports
    computer.availablePorts = temp;
    // update the information about the port block
    computer.firstPort = first;
    computer.blockSize = size;
  }
}
 
bool arPhleetConnectionBroker::_portValid(int port, 
                                          arBrokerComputerData& computer){
  return port >= computer.firstPort && 
         port < computer.firstPort + computer.blockSize;
}
