//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DEVICE_CLIENT_H
#define AR_DEVICE_CLIENT_H


#include <string>
#include <vector>

#include "arInputNode.h"
#include "arNetInputSource.h"
#include "arSZGClient.h"
#include "arMessageHandler.h"
#include "arLogStream.h"
#include "arDriversCalling.h"


class SZG_CALL arDeviceClient : public arMessageHandler {
    public:
        arDeviceClient( arSZGClient* szgClient );
        virtual ~arDeviceClient();

        bool init( int slotNum, std::vector<arIOFilter*>* filterVecPtr=NULL );
        bool init( std::string serviceName, std::vector<arIOFilter*>* filterVecPtr=NULL );

        arInputNode* getInputNode() { return &_inputNode; }

        bool start( void (*msgHandlerThreadFunc)( void* ) );
        bool stop();

    protected:
        bool _inited;
        arNetInputSource _src;
        arInputNode _inputNode;

        bool _init( std::vector<arIOFilter*>* filterVecPtr );
};


#endif

