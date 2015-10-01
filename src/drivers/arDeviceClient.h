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

        bool init( std::vector<arIOFilter*> filters, int slotNum=-1, std::string serviceName="" );

        arInputNode* getInputNode() { return &_inputNode; }

        bool start( void (*msgHandlerThreadFunc)( void* ) );
        bool stop();

    protected:
        arNetInputSource _src;
        arInputNode _inputNode;
};


#endif

