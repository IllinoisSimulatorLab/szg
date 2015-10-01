//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"

#include "arMessageHandler.h"
#include "arPhleetCalling.h"


arMessageHandler::~arMessageHandler() {
    _lock.lock();
    _messages.clear();
    _lock.unlock();
}
    

void arMessageHandler::addMessage( arMessage msg ) {
    _lock.lock();
    _messages.push_back( msg );
    _lock.unlock();
}

std::vector<arMessage> arMessageHandler::getMessages() {
    std::vector<arMessage> msgsOut;
    _lock.lock();
    msgsOut = _messages;
    _messages.clear();
    _lock.unlock();
    return msgsOut;
}

void arMessageHandler::quitMessageTask() {
    _lock.lock();
    _continueMessageTask = false;
    _lock.unlock();
}

bool arMessageHandler::continueMessageTask() {
    bool val;
    _lock.lock();
    val = _continueMessageTask;
    _lock.unlock();
    return val;
}


void asyncMessageTask( void* voidMsgHandler ) {
    arMessageHandler* msgHandlerPtr = static_cast<arMessageHandler*>( voidMsgHandler );
    arSZGClient* szgClient = msgHandlerPtr->getSzgClient();
    int messageID;
    string userName;
    string messageType;
    string messageBody;
    string context;
    while (szgClient->running() && msgHandlerPtr->continueMessageTask()) {
        messageID = szgClient->receiveMessage( &userName, &messageType, &messageBody, &context );
        msgHandlerPtr->addMessage( arMessage( messageID, userName, messageType, messageBody, context ) );
    }
    szgClient->closeConnection();
    // Wait for other threads to exit cleanly.
    ar_usleep(75000);
}

