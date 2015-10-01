//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_MESSAGE_HANDLER_H
#define AR_MESSAGE_HANDLER_H


#include <string>
#include <vector>

#include "arSZGClient.h"
#include "arThread.h"
#include "arLogStream.h"

using namespace std;


class SZG_CALL arMessage {
    public:
        arMessage( int messID, string user, string typ, string body, string con ) :
            id( messID ),
            userName( user ),
            messageType( typ ),
            messageBody( body ),
            context( con ) {}
        int id;
        string userName;
        string messageType;
        string messageBody;
        string context;
};


class SZG_CALL arMessageHandler {
    public:
        arMessageHandler( arSZGClient* cli ) : _continueMessageTask(true), _szgClientPtr(cli) {}
        virtual ~arMessageHandler();

        arSZGClient* getSzgClient() { return _szgClientPtr; }

        void addMessage( arMessage msg );
        std::vector<arMessage> getMessages();

        void quitMessageTask();
        bool continueMessageTask();

    protected:
        bool _continueMessageTask;
        arSZGClient* _szgClientPtr;
        std::vector<arMessage> _messages;
        arLock _lock;
};


void SZG_CALL asyncMessageTask( void* voidMsgHandler );


#endif
