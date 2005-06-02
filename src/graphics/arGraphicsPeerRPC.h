//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_PEER_RPC_H
#define AR_GRAPHICS_PEER_RPC_H

#include "arGraphicsPeer.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

SZG_CALL string ar_graphicsPeerStripName(string& messageBody);

SZG_CALL string ar_graphicsPeerHandleMessage(arGraphicsPeer* peer,
                                             const string& messageType,
				             const string& messageBody);

#endif

