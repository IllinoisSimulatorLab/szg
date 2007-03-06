//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INTEL_GAMEPAD_DRIVER_H
#define AR_INTEL_GAMEPAD_DRIVER_H

#include "arInputSource.h"
#include "arInputHeaders.h"

#include "arDriversCalling.h"

// Driver for RF-wireless gamepad, (no longer) manufactured by Intel.

class arIntelGamepadDriver: public arInputSource {
 public:
  bool init(arSZGClient& client);
  bool start();
 private:
  arThread _eventThread;
#ifdef AR_USE_WIN_32
  IDirectInputDevice2* _pKeyboard;
  friend void ar_intelGamepadDriverEventTask(void*);
#endif
};

#endif
