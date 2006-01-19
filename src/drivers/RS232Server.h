//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arDataServer.h"
#include "arDataUtilities.h"
#include "arSZGClient.h"
#include "arMath.h"
#include "arInput.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arDriversCalling.h"
#include <iostream>
using namespace std;

/// Encapsulation of an RS-232 port.

typedef struct {
  const char* port; // "COM1"
  int timeout;      // 20 msec
  bool rw;          // read+write instead of read-only
  int baud;         // 9600
  // Implicit:  8 bits, no parity, one stop bit, RTS_CONTROL_DISABLE
} RS232Port;

extern int ar_RS232main(int argc, char** argv,
  const char* label, const char* dictName,
  const char* fieldName, int usecSleep, arMatrix4 getMatrix(void),
  void simTask(void*), void rs232Task(void*), const RS232Port& rs232,
  int numAxes = 0, const float* getAxes(void) = NULL);
