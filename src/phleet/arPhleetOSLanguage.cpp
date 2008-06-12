//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPhleetOSLanguage.h"

arPhleetOSLanguage::arPhleetOSLanguage():
  _connectionAck("connection_ack"),
  _killID("kill_ID"),
  _processInfo("process_info"),
  _attributeGetRequest("attribute_get_request"),
  _attributeGetResponse("attribute_get_response"),
  _attributeSetRequest("attribute_set_request"),
  _message("message"),
  _messageAdmin("message_admin"),
  _messageAck("message_ack"),
  _killNotification("kill_notification"),
  _lockRequest("lock_request"),
  _lockRelease("lock_release"),
  _lockResponse("lock_response"),
  _lockListing("lock_listing"),
  _lockNotification("lock_notification"),
  _registerService("register_service"),
  _requestService("request_service"),
  _brokerResult("broker_result"),
  _getServices("get_services"),
  _serviceRelease("service_release"),
  _serviceInfo("service_info") {

  // Don't forget to get the IDs of the fields shared by every record
  AR_PHLEET_USER = _connectionAck.getAttributeID("phleet_user");
  AR_PHLEET_CONTEXT = _connectionAck.getAttributeID("phleet_context");
  AR_PHLEET_AUTH = _connectionAck.getAttributeID("phleet_auth");
  AR_PHLEET_MATCH = _connectionAck.getAttributeID("phleet_match");

  AR_CONNECTION_ACK_LABEL = _connectionAck.add("Label", AR_CHAR);
  AR_CONNECTION_ACK = _dictionary.add(&_connectionAck);

  AR_KILL_ID = _killID.add("KillID", AR_INT);
  AR_KILL = _dictionary.add(&_killID);

  AR_PROCESS_INFO_TYPE = _processInfo.add("Type", AR_CHAR);
  AR_PROCESS_INFO_ID = _processInfo.add("ID", AR_INT);
  AR_PROCESS_INFO_LABEL = _processInfo.add("Label", AR_CHAR);
  AR_PROCESS_INFO = _dictionary.add(&_processInfo);

  AR_ATTR_GET_REQ_ATTR = _attributeGetRequest.add("Attribute", AR_CHAR);
  AR_ATTR_GET_REQ_TYPE = _attributeGetRequest.add("Type", AR_CHAR);
  AR_ATTR_GET_REQ = _dictionary.add(&_attributeGetRequest);

  AR_ATTR_GET_RES_ATTR = _attributeGetResponse.add("Attribute", AR_CHAR);
  AR_ATTR_GET_RES_VAL = _attributeGetResponse.add("Value", AR_CHAR);
  AR_ATTR_GET_RES = _dictionary.add(&_attributeGetResponse);

  AR_ATTR_SET_TYPE = _attributeSetRequest.add("Type", AR_INT);
  AR_ATTR_SET_ATTR = _attributeSetRequest.add("Attribute", AR_CHAR);
  AR_ATTR_SET_VAL = _attributeSetRequest.add("Value", AR_CHAR);
  AR_ATTR_SET = _dictionary.add(&_attributeSetRequest);

  AR_SZG_MESSAGE_ID = _message.add("ID", AR_INT);
  AR_SZG_MESSAGE_RESPONSE = _message.add("Response", AR_INT);
  AR_SZG_MESSAGE_TYPE = _message.add("Type", AR_CHAR);
  AR_SZG_MESSAGE_BODY = _message.add("Body", AR_CHAR);
  AR_SZG_MESSAGE_DEST = _message.add("Destination", AR_INT);
  AR_SZG_MESSAGE = _dictionary.add(&_message);

  AR_SZG_MESSAGE_ADMIN_ID = _messageAdmin.add("ID", AR_INT);
  AR_SZG_MESSAGE_ADMIN_STATUS = _messageAdmin.add("Status", AR_CHAR);
  AR_SZG_MESSAGE_ADMIN_TYPE = _messageAdmin.add("Type", AR_CHAR);
  AR_SZG_MESSAGE_ADMIN_BODY = _messageAdmin.add("Body", AR_CHAR);
  AR_SZG_MESSAGE_ADMIN = _dictionary.add(&_messageAdmin);

  AR_SZG_MESSAGE_ACK_ID = _messageAck.add("ID", AR_INT);
  AR_SZG_MESSAGE_ACK_STATUS = _messageAck.add("Status", AR_CHAR);
  AR_SZG_MESSAGE_ACK = _dictionary.add(&_messageAck);

  AR_SZG_KILL_NOTIFICATION_ID = _killNotification.add("ID", AR_INT);
  AR_SZG_KILL_NOTIFICATION = _dictionary.add(&_killNotification);

  AR_SZG_LOCK_REQUEST_NAME = _lockRequest.add("Name", AR_CHAR);
  AR_SZG_LOCK_REQUEST = _dictionary.add(&_lockRequest);

  AR_SZG_LOCK_RELEASE_NAME = _lockRelease.add("Name", AR_CHAR);
  AR_SZG_LOCK_RELEASE = _dictionary.add(&_lockRelease);

  AR_SZG_LOCK_RESPONSE_NAME = _lockResponse.add("Name", AR_CHAR);
  AR_SZG_LOCK_RESPONSE_STATUS = _lockResponse.add("Status", AR_CHAR);
  AR_SZG_LOCK_RESPONSE_OWNER = _lockResponse.add("Owner", AR_INT);
  AR_SZG_LOCK_RESPONSE = _dictionary.add(&_lockResponse);

  AR_SZG_LOCK_LISTING_LOCKS = _lockListing.add("Locks", AR_CHAR);
  AR_SZG_LOCK_LISTING_COMPUTERS = _lockListing.add("Computers", AR_CHAR);
  AR_SZG_LOCK_LISTING_COMPONENTS = _lockListing.add("Components", AR_INT);
  AR_SZG_LOCK_LISTING = _dictionary.add(&_lockListing);

  AR_SZG_LOCK_NOTIFICATION_NAME = _lockNotification.add("Name", AR_CHAR);
  AR_SZG_LOCK_NOTIFICATION = _dictionary.add(&_lockNotification);

  AR_SZG_REGISTER_SERVICE_STATUS = _registerService.add("Status", AR_CHAR);
  AR_SZG_REGISTER_SERVICE_TAG = _registerService.add("Tag", AR_CHAR);
  AR_SZG_REGISTER_SERVICE_NETWORKS
    = _registerService.add("Networks", AR_CHAR);
  AR_SZG_REGISTER_SERVICE_ADDRESSES
    = _registerService.add("Addresses", AR_CHAR);
  AR_SZG_REGISTER_SERVICE_SIZE
    = _registerService.add("Size", AR_INT);
  AR_SZG_REGISTER_SERVICE_COMPUTER
    = _registerService.add("Computer", AR_CHAR);
  AR_SZG_REGISTER_SERVICE_BLOCK
    = _registerService.add("Block", AR_INT);
  AR_SZG_REGISTER_SERVICE_PORT
    = _registerService.add("Port", AR_INT);
  AR_SZG_REGISTER_SERVICE = _dictionary.add(&_registerService);

  AR_SZG_REQUEST_SERVICE_COMPUTER = _requestService.add("Computer", AR_CHAR);
  AR_SZG_REQUEST_SERVICE_TAG = _requestService.add("Tag", AR_CHAR);
  AR_SZG_REQUEST_SERVICE_NETWORKS = _requestService.add("Networks", AR_CHAR);
  AR_SZG_REQUEST_SERVICE_ASYNC = _requestService.add("Async", AR_CHAR);
  AR_SZG_REQUEST_SERVICE = _dictionary.add(&_requestService);

  AR_SZG_BROKER_RESULT_STATUS = _brokerResult.add("Status", AR_CHAR);
  AR_SZG_BROKER_RESULT_ADDRESS = _brokerResult.add("Address", AR_CHAR);
  AR_SZG_BROKER_RESULT_PORT = _brokerResult.add("Port", AR_INT);
  AR_SZG_BROKER_RESULT = _dictionary.add(&_brokerResult);

  AR_SZG_GET_SERVICES_SERVICES = _getServices.add("Services", AR_CHAR);
  AR_SZG_GET_SERVICES_TYPE = _getServices.add("Type", AR_CHAR);
  AR_SZG_GET_SERVICES_COMPUTERS = _getServices.add("Computers", AR_CHAR);
  AR_SZG_GET_SERVICES_COMPONENTS = _getServices.add("Components", AR_INT);
  AR_SZG_GET_SERVICES = _dictionary.add(&_getServices);

  AR_SZG_SERVICE_RELEASE_NAME = _serviceRelease.add("Name", AR_CHAR);
  AR_SZG_SERVICE_RELEASE_COMPUTER = _serviceRelease.add("Computer", AR_CHAR);
  AR_SZG_SERVICE_RELEASE = _dictionary.add(&_serviceRelease);

  AR_SZG_SERVICE_INFO_OP = _serviceInfo.add("Op", AR_CHAR);
  AR_SZG_SERVICE_INFO_STATUS = _serviceInfo.add("Status", AR_CHAR);
  AR_SZG_SERVICE_INFO_TAG = _serviceInfo.add("Tag", AR_CHAR);
  AR_SZG_SERVICE_INFO = _dictionary.add(&_serviceInfo);
}

arPhleetOSLanguage::~arPhleetOSLanguage() {

}
