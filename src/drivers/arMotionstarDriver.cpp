//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//
// This file includes modified sample code from
// Acension Technologies' bnsample.c.
//********************************************************

#include "arPrecompiled.h"
#include "arMotionstarDriver.h"

DriverFactory(arMotionstarDriver, "arInputSource")

void ar_motionstarDriverEventTask(void* motionstarDriver) {
  arMotionstarDriver* d = (arMotionstarDriver*) motionstarDriver;
  d->_sendCommand(MSG_RUN_CONTINUOUS, 0);
  if (!d->_getResponse(RSP_RUN_CONTINUOUS)) {
    goto LDone;
  }
  ar_log_remark() << "arMotionstarDriver beginning event task.\n";
  while (d->_getResponse(DATA_PACKET))
    d->_parseData(&d->_response);
LDone:
  ar_log_error() << "arMotionstarDriver: no response from driver.\n";
}

arMotionstarDriver::arMotionstarDriver():
  _birdnetIP(string("NULL")),
  _angScale(180.0/32767.0), // conversion constant
  _commandSocket(AR_STANDARD_SOCKET),
  _sequence(0),
  _TCP_PORT(6000),
  _numberRemaps(0),
  _useButton(false),
  _lastButtonValue(-1) {
}

bool arMotionstarDriver::init(arSZGClient& SZGClient) {
  _birdnetIP = SZGClient.getAttribute("SZG_MOTIONSTAR", "IPhost");
  ar_log_debug() << "arMotionstarDriver: tracker host is " << _birdnetIP << ".\n";

  // this test could be better
  if (_birdnetIP.length()<7) {
    ar_log_error() << "bad or no SZG_MOTIONSTAR/IPhost.\n";
    return false;
  }

//  _setAlphaMin = true;
//  _setAlphaMax = true;
//  _setVm = true;
//  long filterBuf[7];
//  if (!SZGClient.getAttributeLongs( "SZG_MOTIONSTAR", "alphaMin", filterBuf, 7 )) {
//    ar_log_error() << "arMotionstarDriver: no SZG_MOTIONSTAR/alphaMin.\n";
//    _setAlphaMin = false;
//  } else {
//    for (i=0; i<7; i++) {
//      if ((filterBuf[i] < 0)||(filterBuf[i] > 32767)) {
//        ar_log_error() << "arMotionstarDriver: alphaMin value " << filterBuf[i]
//             << " out of bounds (0-32767).\n";
//        _setAlphaMin = false;
//      }
//      _alphaMin[i] = (unsigned short)filterBuf[i];
//    }
//  }
//  if (!SZGClient.getAttributeLongs( "SZG_MOTIONSTAR", "alphaMax", filterBuf, 7 )) {
//    ar_log_error() << "arMotionstarDriver: no SZG_MOTIONSTAR/alphaMax.\n";
//    _setAlphaMax = false;
//  } else {
//    for (i=0; i<7; i++) {
//      if ((filterBuf[i] < 0)||(filterBuf[i] > 32767)) {
//        ar_log_error() << "arMotionstarDriver: alphaMax value " << filterBuf[i]
//             << " out of bounds (0-32767).\n";
//        _setAlphaMax = false;
//      }
//      _alphaMax[i] = (unsigned short)filterBuf[i];
//    }
//  }
//  if (!SZGClient.getAttributeLongs( "SZG_MOTIONSTAR", "Vm", filterBuf, 7 )) {
//    ar_log_error() << "arMotionstarDriver: no SZG_MOTIONSTAR/Vm.\n";
//    _setVm = false;
//  } else {
//    for (i=0; i<7; i++) {
//      if ((filterBuf[i] < 0.)||(filterBuf[i] > 32767)) {
//        ar_log_error() << "arMotionstarDriver: Vm value " << filterBuf[i]
//             << " out of bounds (0-32767).\n";
//        _setVm = false;
//      }
//      _Vm[i] = (unsigned short)filterBuf[i];
//    }
//  }

  //unused int birdFormat = BN_POSANG;
  const int birdsRequired = BN_MAX_ADDR;
  const float birdRate = 93.3;
  const int reportRate = 1;

  _commandSocket.ar_create();
  if (_commandSocket.ar_connect(_birdnetIP.c_str(), _TCP_PORT)<0) {
    ar_log_error() << "arMotionstarDriver failed to open command socket.\n";
    return false;
  }

  // start the system working
  if (!_sendWakeup()) {
    ar_log_error() << "arMotionstarDriver failed to wake up.\n";
    return false;
  }

  // get the system status
  BN_SYSTEM_STATUS* sys = NULL;
  if (!_getStatusAll(&sys)) {
    ar_log_error() << "arMotionstarDriver failed to get status.\n";
    return false;
  }

  // set measurement rate requested
  char buf[10];
  sprintf(buf, "%06.0f", birdRate * 1000.);
  strncpy(sys->rate, buf, 6);
  const int nDevices = sys->chassisDevices;

  // Are we using the "monowand" button?
  // NOTE: strictly speaking, this is local to the ISL and should be
  // ELIMINATED from this driver! The "right thing" would be to hack up
  // a new loadable module that would have the ISL-only features
  // contained in it... so arISLMotionstarDriver.so...
  _useButton = SZGClient.getAttribute("SZG_MOTIONSTAR", "use_button") == "true";

  // Export the signature.
  int sig[3];
  if (!SZGClient.getAttributeInts("SZG_MOTIONSTAR", "signature", sig, 3)) {
    // button overrides old wand's (shifts them up)!
    _setDeviceElements(_useButton ? 1 : 0, 0, nDevices-1);
  } else {
    ar_log_error() << "SZG_MOTIONSTAR/signature overriding device signature with ("
         << sig[0] << ", " << sig[1] << ", " << sig[2] << ").\n";
    _setDeviceElements(sig);
  }

  // send the system setup
  if (!_setStatusAll(sys)) {
    ar_log_error() << "arMotionstarDriver failed to set system status.\n";
    return(-1);
  }

  BN_BIRD_STATUS* bird = NULL;
  int i = 0;
  for (i=2; i<=nDevices; i++) {
    // get the status of an individual bird
    if (!_getStatusBird(i, &bird)) {
      ar_log_error() << "arMotionstarDriver: no status from bird " << i-1 << "\n";
      return false;
    }

    // change the data format to something new for all birds
    if (i <= birdsRequired+1) {
      // set reporting format to position/angles
      bird->header.dataFormat = 0x64;
    }
    else{
      // no data
      bird->header.dataFormat = 0;
    }
    // set new report rate for all birds
    bird->header.reportRate = reportRate;
    bird->header.setup |= BN_FLOCK_APPENDBUT;
    _posScale = (float)ntohs(bird->header.scaling) / 32767.;

    // set new filter tables for all birds
//    if (_setAlphaMin) {
//      for (j=0; j<7; j++)
//        bird->alphaMin.entry[j] = htons(_alphaMin[j]);
//    }
//    if (_setAlphaMax) {
//      for (j=0; j<7; j++)
//        bird->alphaMax.entry[j] = htons(_alphaMax[j]);
//    }
//    if (_setVm) {
//      for (j=0; j<7; j++)
//        bird->Vm.entry[j] = htons(_Vm[j]);
//    }

    if (!_setStatusBird(i, bird)) {
      ar_log_error() << "arMotionstarDriver failed to set status of bird " << i-1 << "\n";
      return false;
    }
  }
  ar_log_debug() << "arMotionstarDriver inited.\n";
  return true;
}

bool arMotionstarDriver::start() {
  return _eventThread.beginThread(ar_motionstarDriverEventTask, this);
}

bool arMotionstarDriver::stop() {
  return true;
}

bool arMotionstarDriver::_sendWakeup() {
  _sendCommand(MSG_WAKE_UP, 0);
  return _getResponse(RSP_WAKE_UP);
}

bool arMotionstarDriver::_getStatusAll(BN_SYSTEM_STATUS** sys) {
  _sendCommand(MSG_GET_STATUS, 0);
  if (!_getResponse(RSP_GET_STATUS)) {
    return false;
  }
  *sys = (BN_SYSTEM_STATUS*)_response.data;
  return true;
}

bool arMotionstarDriver::_setStatusAll(BN_SYSTEM_STATUS* sys) {
  _sendStatus(0, (char*)sys, sizeof(BN_SYSTEM_STATUS));
  return _getResponse(RSP_SEND_SETUP);
}

bool arMotionstarDriver::_getStatusBird(int addr, BN_BIRD_STATUS** bird) {
  _sendCommand(MSG_GET_STATUS, addr);
  if (!_getResponse(RSP_GET_STATUS)) {
    return false;
  }
  BN_BIRD_STATUS* bptr = (BN_BIRD_STATUS *)_response.data;
  *bird = bptr;
  return true;
}

bool arMotionstarDriver::_setStatusBird(int addr, BN_BIRD_STATUS* bird) {
  _sendStatus(addr, (char*)bird, sizeof(BN_BIRD_STATUS));
  return _getResponse(RSP_SEND_SETUP);
}

bool arMotionstarDriver::_sendStatus(int xtype, char *data, int size) {
  BN_PACKET pkt;
  pkt.header.type = MSG_SEND_SETUP;
  pkt.header.xtype = xtype;
  pkt.header.sequence = htons(_sequence++);
  pkt.header.errorCode = 0;
  pkt.header.protocol = 3;
  pkt.header.numBytes = htons((short)size);
  memcpy(pkt.data, data, size);
  if (!_commandSocket.ar_safeWrite((char*)&pkt, sizeof(BN_HEADER)+size)) {
    ar_log_error() << "arMotionstarDriver: Status send failed.\n";
    return false;
  }
  return true;
}

bool arMotionstarDriver::_sendCommand(int cmd, int xtype) {
  BN_HEADER header;
  header.type = cmd;
  header.xtype = xtype;
  header.sequence = _sequence++;
  header.errorCode = 0;
  header.protocol = 3;
  header.numBytes = 0;

  if (!_commandSocket.ar_safeWrite((char*)&header, sizeof(BN_HEADER))) {
    ar_log_error() << "arMotionstarDriver: command send failed.\n";
    return false;
  }
  return true;
}

bool arMotionstarDriver::_getResponse(int rsp) {
  const int headerSize = sizeof(BN_HEADER);  /* need at least the header */
  char* cptr = (char *)&_response.header;
  if (!_commandSocket.ar_safeRead(cptr, headerSize)) {
    ar_log_error() << "arMotionstarDriver failed to read response header.\n";
    return false;
  }
  const int bodySize = ntohs(_response.header.numBytes);
  if (bodySize) {
    if (!_commandSocket.ar_safeRead(cptr+headerSize, bodySize)) {
      ar_log_error() << "arMotionstarDriver failed to read response body.\n";
      return false;
    }
  }
  if (_response.header.type != rsp) {
    ar_log_error() << "arMotionstarDriver Command response "
         << _response.header.type << " not of " << "expected type " << rsp << ".\n";
    return false;
  }
  return true;
}

void arMotionstarDriver::_parseData(BN_PACKET *packet) {
#ifdef UNUSED
  // Get timestamp data.
  long seconds = ntohl(packet->header.seconds);
  int milliseconds = ntohs(packet->header.milliseconds);
#endif
  int n = ntohs(packet->header.numBytes);
  char* pch = (char*) packet->data;
  bool fButtonPushed = false;
  while (n > 0) {
    BN_DATA* dptr = (BN_DATA*) pch;
    // Motionstar birds start at 2, Syzygy birds start at 0.
    const unsigned char addr = (dptr->addr & ~BUTTON_FLAG) - 2;
    short* sptr = (short*) dptr->data;
    switch (dptr->format >> 4) {
    case 4:
      // Copy pos/ang data to local storage.
      _receivedData[0] = ((short)ntohs(*sptr))*_posScale;
      _receivedData[1] = ((short)ntohs(*(sptr+1)))*_posScale;
      _receivedData[2] = ((short)ntohs(*(sptr+2)))*_posScale;
      _receivedData[3] = ((short)ntohs(*(sptr+3)))*_angScale;
      _receivedData[4] = ((short)ntohs(*(sptr+4)))*_angScale;
      _receivedData[5] = ((short)ntohs(*(sptr+5)))*_angScale;
      _generateEvent(addr);
      break;
    case -2: // Determined empirically in Cube.  dptr->format == -31.
      fButtonPushed = true;
      break;
    default:
      printf("arMotionstarDriver warning: unsupported packet format %d (%d)\n",
          dptr->format >> 4, dptr->format);
      break;
    }
    int tmp = (dptr->format & 0xf) << 1;
    if (dptr->addr & BUTTON_FLAG) {
      // const int button = *(short*)(pch+(tmp+2));
      // if (button) printf(" Button %d\n", button);
      tmp += 2;
    }
    tmp += 2;   // include bytes for addr and format
    n -= tmp;
    pch += tmp;
  }
  if (_useButton)
    _generateButtonEvent(fButtonPushed ? 1 : 0);
}

void arMotionstarDriver::_generateButtonEvent(int value) {
  static int prev = -1; // Guaranteed to mismatch, first time through.
    // todo: Nonreentrant.  make "prev" a private member.
  if (value != prev) {
    sendButton(0, value);
    prev = value;
  }
}

void arMotionstarDriver::_generateEvent(int sensorID) {
  // Convert from motionstar coords (units=inches, right-handed, xy horizontal, z down)
  // to generic Syzygy coordinates (units=feet, right-handed, xz horizontal, y up).
  _receivedData[0] /= 12.;                      // x->x
  float z = _receivedData[1]/12.;               // y->z
  _receivedData[1] = -_receivedData[2]/12.;     // z->-y
  _receivedData[2] = z;
  _receivedData[3] = -_receivedData[3];         // azimuth -> -azimuth
  _receivedData[5] = -_receivedData[5];         // roll -> -roll

  const arMatrix4 transMatrix(
      ar_translationMatrix(_receivedData[0],
                           _receivedData[1],
                           _receivedData[2])
      );

  const arMatrix4 rotMatrix(
      ar_rotationMatrix('y',  ar_convertToRad(_receivedData[3])) *
      ar_rotationMatrix('x',  ar_convertToRad(_receivedData[4])) *
      ar_rotationMatrix('z',  ar_convertToRad(_receivedData[5]))
      );

  sendMatrix(sensorID, transMatrix * rotMatrix );
}

// Old hack code, mostly moved to a PForth filter
//void arMotionstarDriver::_tweak() {
//  float* p = _receivedData;
  // convert from inches to feet, for cave-coords.
//  p[0] /= 12.;
//  p[1] /= 12.;
//  p[2] /= 12.;

//  const float rot = 1./sqrt(2.); // cos and sin of 45 degrees
//  float x = rot*p[0] - rot*p[1];
//  float z = rot*p[0] + rot*p[1];
//  float y = 5. - p[2];

   // x and z are raw [1.2, 10.4];  convert to [-5, 5]
//  const float x0 = 1.2;
//  const float x1 = 10.4;
//  const float y0 = -5.;
//  const float y1 = 5.;
//  x = y0 + (y1-y0)/(x1-x0)*(x-x0);
//  z = y0 + (y1-y0)/(x1-x0)*(z-x0);

//  p[0] = x;
//  p[1] = y;
//  p[2] = z;

  // angles now... azi el roll
//  float azi = -p[3] - 135;
//  float el = p[4];
//  float roll = -p[5];

//  if (azi < -180.) azi += 360.;
//  else if (azi > 180.) azi -= 360.;

//  if (roll < -180.) roll += 360.;
//  else if (roll > 180.) roll -= 360.;

//  p[3] = azi;
//  p[4] = el;
//  p[5] = roll;
//}
