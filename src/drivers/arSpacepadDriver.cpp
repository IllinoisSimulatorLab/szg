//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// This driver is based on code of Ascension Technologies
// Cutting and pasting has occurred!
/****************************************************************************
*****************************************************************************
    isabus.c  - ISAbus general routines - DOS PC Compatible

    written for:    Ascension Technology Corporation
                    PO Box 527
                    Burlington, Vermont  05402

                    802-860-6440


    written by:     Vlad Kogan
 6/19/95 vk.. reset_through_isa() added
 10/5/95 vk.. wait_tosend_word() function added
         - calls to send_isa_word() changed to calls to wait_tosend_word,
         which returns on success OR TRANSMIT TIME OUT. Done to eliminate
         software hang ups.

       <<<< Copyright 1991 Ascension Technology Corporation >>>>
*****************************************************************************
****************************************************************************/

#include "arPrecompiled.h"

// Ascension Technology constants
#define TRUE    1
#define FALSE   0

#define ASC_POINT 0
#define ASC_CONTINUOUS 1
#define ASC_STREAM  2

#define POS         1
#define ANGLE       2
#define MATRIX      3
#define QUATER      4
#define POSANGLE    5
#define POSMATRIX   6
#define POSQUATER   7

#define RXSTATISTICSTIMEOUTINSECS 20
#define RXTIMEOUTINSECS 4
#define TXTIMEOUTINSECS 4
#define ISA_BASE_MASK 0xFC03
#define COM_CHAN_ISA   TRUE
#define COM_CHAN_RS232 !COM_CHAN_ISA
#define ISAOKTOWRITE   1
#define ISAOKTOREAD    2

#define NODATAAVAIL     -1
#define RXERRORS        -2
#define RXTIMEOUT       -3
#define TXTIMEOUT       -4
#define RXPHASEERROR    -7

//*******************************************************************
// end Ascension header code
//*******************************************************************

#include "arSpacepadDriver.h"
#ifdef AR_USE_WIN_32
#include <conio.h>
#endif


// todo: explicitly define outpw and inpw properly for debug build.

#ifdef _DEBUG
// avoid a link error (but do nothing, as a result!)
#define outpw(a,b) ((void)0)
#define inpw(a) (0)
#endif

DriverFactory(arSpacepadDriver, "arInputSource")

void ar_spacepadDriverEventTask(void* driver){
  arSpacepadDriver* d = (arSpacepadDriver*) driver;
  while (true){
    d->queueMatrix(0,d->_getSpacepadMatrix(1));
    d->queueMatrix(1,d->_getSpacepadMatrix(2));
    d->sendQueue();
    ar_usleep(10000);
  }
}

arSpacepadDriver::arSpacepadDriver(){
  isa_base_addr = 772;   // 0x304, this is the default card address
  isa_status_addr = 774;
  recaddr = 0;           // which bird we are sampling
                         // 0 or 1 = bird 1, 2 = bird 2
  phaseerror_count = 0;
  positionScaling = 12.0/32768.0; // position info in feet
  angleScaling = 180.0/32768.0;
}

arSpacepadDriver::~arSpacepadDriver(){
}

bool arSpacepadDriver::init(arSZGClient& c){
#ifndef AR_USE_WIN_32
  ar_log_error() << "arSpacepadDriver implemented only in Windows.\n";
  return false;
#endif
  _setDeviceElements(0,0,2);
  // read in the calibration information
  const string transmitterOffset = c.getAttribute("SZG_SPACEPAD","transmitter_offset");
  if (transmitterOffset != "NULL"){
    ar_parseFloatString(transmitterOffset, _transmitterOffset.v, 16);
    ar_log_debug() << "arSpacepadDriver: transmitter offset is " << _transmitterOffset << ".\n";
  }
  float temp[4];
  const string sensor0Rot =
    c.getAttribute("SZG_SPACEPAD","sensor0_rot");
  if (sensor0Rot != "NULL"){
    ar_parseFloatString(sensor0Rot, temp, 4);
    _sensorRot[0] = ar_rotationMatrix(arVector3(temp), ar_convertToRad(temp[3]));
    ar_log_debug() << "arSpacepadDriver: sensor 0 rotation = " << _sensorRot[0] <<".\n";
  }
  const string sensor1Rot =
    c.getAttribute("SZG_SPACEPAD","sensor1_rot");
  if (sensor1Rot != "NULL"){
    ar_parseFloatString(sensor1Rot, temp, 4);
    _sensorRot[1] = ar_rotationMatrix(arVector3(temp), ar_convertToRad(temp[3]));
    ar_log_debug() << "arSpacepadDriver: sensor 1 rotation = " << _sensorRot[1] <<".\n";
  }
  _reset_through_isa();
  ar_log_debug() << "arSpacepadDriver inited.\n";
  return true;
}

bool arSpacepadDriver::start(){
#ifndef AR_USE_WIN_32
  ar_log_error() << "arSpacepadDriver implemented only in Windows.\n";
  return false;
#endif
  _eventThread.beginThread(ar_spacepadDriverEventTask,this);
  return true;
}

bool arSpacepadDriver::stop(){
  return false;
}

arMatrix4 arSpacepadDriver::_getSpacepadMatrix(int sensorID){
  if (sensorID < 1 || sensorID > 2){
    ar_log_error() << "arSpacepadDriver: only sensors 1 and 2 are supported.\n";
    return arMatrix4();
  }

  // Confusingly use the global recaddr.
  recaddr = sensorID;
  short dataStorage[20];
  float values[12];
  // Set the hemisphere for every sensor (I think).  Hack, fails for >4 sensors.
  static int initted = 0;
  // make sure the hemisphere of the spacepad is set correctly.
  if (initted<4){
    if (!_hemisphere_cmd(0)){
      ar_log_error() << "arSpacepadDriver failed to set hemisphere.\n";
    }
    // restore recaddr
    initted++;
  }
  if (_position_matrix_cmd() != TRUE){
    ar_log_error() << "arSpacepadDriver failed to request data\n";
    goto LAbort;
  }
  if (_point_cmd() != TRUE){
    ar_log_error() << "arSpacepadDriver failed to issue the point command request.\n";
    goto LAbort;
  }
  // The 2nd parameter below is twice the number of shorts
  // returned.  2*6=12 for position/angles and 2*12=24 for position/matrix
  // (i.e. it is the number of bytes requested from the card)
  if (_get_isa_record(dataStorage, 24, ASC_POINT) < 0){
    ar_log_error() << "arSpacepadDriver failed to read data\n";
LAbort:
    _reset_through_isa();
    return arMatrix4();
  }
  values[0] = dataStorage[0]*positionScaling;
  values[1] = dataStorage[1]*positionScaling;
  values[2] = dataStorage[2]*positionScaling;
  values[3] = dataStorage[3]/32768.0;
  values[4] = dataStorage[4]/32768.0;
  values[5] = dataStorage[5]/32768.0;
  values[6] = dataStorage[6]/32768.0;
  values[7] = dataStorage[7]/32768.0;
  values[8] = dataStorage[8]/32768.0;
  values[9] = dataStorage[9]/32768.0;
  values[10] = dataStorage[10]/32768.0;
  values[11] = dataStorage[11]/32768.0;
  const arMatrix4 rotMatrix(values[3], values[6], values[9 ], 0,
                            values[4], values[7], values[10], 0,
                            values[5], values[8], values[11], 0,
                            0,         0,         0,          1);
  // Conjugate with this matrix to change the coordinate system.
  const arMatrix4 switchMatrix(1,0,0,0,
			       0,0,1,0,
			       0,-1,0,0,
			       0,0,0,1);

  // Transform the translation matrix into syzygy coordinates.
  arVector3 transVector(switchMatrix.inverse() * arVector3(values));
  const arMatrix4 transMatrix(1,0,0,transVector[0],
	                      0,1,0,transVector[1],
	                      0,0,1,transVector[2],
	                      0,0,0,1);

  // Invert the matrix reported by the spacepad,
  // because it is intended to be right-multiplied by coordinates.
  // Premultiply by a rotation about 'y',
  // to follow Syzygy's convention of negative 'z' axis face front.
  // Calibrate with transmitter offset and sensor rotation.
  return _transmitterOffset *
         transMatrix * switchMatrix.inverse() * rotMatrix.inverse() * switchMatrix *
         ar_rotationMatrix('y', ar_convertToRad(-90)) * _sensorRot[sensorID-1];
}
   
int arSpacepadDriver::_point_cmd(void){
  static unsigned char cmd_char = 'B';
  return (_send_isa_cmd(&cmd_char,1) == 1) ? TRUE : FALSE;
}

int arSpacepadDriver::_position_angles_cmd(void){
  static unsigned char cmd_char = 'Y';
  return (_send_isa_cmd(&cmd_char,1) == 1) ? TRUE : FALSE;
}

int arSpacepadDriver::_position_matrix_cmd(void){
  static unsigned char cmd_char = 'Z';
  return (_send_isa_cmd(&cmd_char,1) == 1) ? TRUE : FALSE;
}

int arSpacepadDriver::_hemisphere_cmd(int hemi_parameter){
  static unsigned char hemisphere_cdata[] = {'L',0XC,0};  // command string
  if (hemi_parameter == 0){
    hemisphere_cdata[2] = 0;
  }
  else{
    hemisphere_cdata[2] = 1;
  }
  return (_send_isa_cmd(hemisphere_cdata,3) == 3) ? TRUE : FALSE;
}
  

//************************************************************
// Ascension Technologies code begins here. Slightly modified
// to make it C++_like
//************************************************************


int arSpacepadDriver::_send_isa_cmd(unsigned char* cmd, short cmdsize){
  short txcount = 0;
  unsigned short com_word = 0;

  // most significant byte = Fx, where x is the bird ID
  if (recaddr > 0){
    com_word = (0xF0 | (unsigned short) recaddr) << 8;
  }
  while (txcount < cmdsize){
    if ((txcount != 0) || (com_word == 0)){
      com_word = (unsigned short) *cmd << 8;
      cmd++;
      txcount++;
    }
    if (txcount <  cmdsize){
      com_word = com_word | (unsigned short) *cmd;
      cmd++;
      txcount++;
    }
    if (_wait_tosend_word(com_word) != TRUE){
      return FALSE;
    }
  }
  return txcount;
}

int arSpacepadDriver::_wait_tosend_word(unsigned short word){
  while ((_send_isa_word(word)) != TRUE)
    ;
  return TRUE;
}

int arSpacepadDriver::_send_isa_word(unsigned short word){
#ifdef AR_USE_WIN_32
  if (!(_read_isa_status() & ISAOKTOWRITE)){
    return FALSE;
  }
  // write to the isa bus
  _outpw(isa_base_addr, word);
  return TRUE;
#else
  word = 23; // avoid compiler warning
  return FALSE;
#endif
}


int arSpacepadDriver::_read_isa_status(){
#ifdef AR_USE_WIN_32
  // read from the isa status register
  return _inpw(isa_status_addr);
#else
  return 0;
#endif
}

/*
    Parameters Passed:  rxbuf       -   pointer to a buffer to store the
                                        received characters
                        recsize     -   number of characters to receive
                        outputmode  -   POINT, CONTINUOUS or STREAM

    Return Value:       If successful, returns recsize
                        else, RXERRORS if Rx errors were detected while
                        receiving data, or RXPHASEERRORS if in POINT or
                        CONTINUOUS mode and a phase error is detected.

    Remarks:            A record of data has the LSByte of the first
                        word set to a 1.  The routine verifies that
                        the first character received is in PHASE.  If
                        in STREAM mode, the routine resynches and tries
                        to capture the data into the rxbuf.  If in POINT
                        or CONTINUOUS mode, then the routine returns
                        indicating a RXPHASEERROR.

*/
int arSpacepadDriver::_get_isa_record(short* rxbuf,
                                      short recsize,
                                      short outputmode){
  short rxcount = 0;
  long rxword = 0;
  short resynch = 0;
  short * rxbufptr = NULL;

  resynch = TRUE;
  do{
    if (resynch){
      rxcount = 0;                /* initialize char counter */
      rxbufptr = rxbuf;           /* setup buffer pointer */
      resynch = FALSE;
    }
    //Get first word and if error and NOT in STREAM mode..return
    if (rxcount == 0){
      if ((rxword = _waitforword()) < 0){
        if ((outputmode != ASC_STREAM) || (rxword == RXTIMEOUT)){
          phaseerror_count = 0;
          return RXERRORS;
        }
	// If an error occured and we are in STREAM mode, resynch
        if (outputmode == ASC_STREAM){
          phaseerror_count++;
          resynch = TRUE;
          continue;
        }
      }
      // check to make sure the phase bit is '1'. If not and in STREAM mode,
      // resynch, else return with an error
      if (!(rxword & 0x0001)){
        if (outputmode == ASC_STREAM){
          phaseerror_count++;
          resynch = TRUE;
          continue;
        }
        else{
          phaseerror_count = 0;
          return RXPHASEERROR;
        }
      }
    }
    else{
      // rxcount > 0, get rest of data from ISA bus, otherwise handle error
      if ((rxword = _waitforword()) >= 0){
	// check phase bit
        if (rxword & 0x0001){
          if (outputmode == ASC_STREAM){
            phaseerror_count++;
            resynch = TRUE;
            continue;
          }
          else{
            phaseerror_count = 0;
            return RXPHASEERROR;
	  }
	}
      }
      else{
	// handle error
        if (outputmode == ASC_STREAM){
          phaseerror_count++;
          resynch = TRUE;
          continue;
        }
        else{
          phaseerror_count = 0;
          return RXERRORS;
        }
      }
    }
    // store the retrieved information
    *rxbufptr++ = (short)rxword;
    rxcount += 2;
  }
  while ((resynch) || (rxcount < recsize));

  return rxcount;
}

long arSpacepadDriver::_waitforword(){
  long rxword = 0;

  while ((rxword = _get_isa_word()) == NODATAAVAIL){
  }
  if (rxword < 0){
    return RXERRORS;
  }
  return rxword;
}

long arSpacepadDriver::_get_isa_word(){
#ifdef AR_USE_WIN_32
  if (_read_isa_status() & ISAOKTOREAD){
    return _inpw(isa_base_addr);
  }
#endif
  return NODATAAVAIL;
}

void arSpacepadDriver::_reset_through_isa(){
#ifdef AR_USE_WIN_32
  // write to the ISA bus
 _outpw(isa_status_addr, 0);
 _outpw(isa_status_addr, 1);
#endif
}
