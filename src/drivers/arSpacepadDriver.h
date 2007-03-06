//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SPACEPAD_DRIVER
#define AR_SPACEPAD_DRIVER

#include "arInputSource.h"

#include "arDriversCalling.h"

class SZG_CALL arSpacepadDriver: public arInputSource{
  friend void ar_spacepadDriverEventTask(void*);
 public:
  arSpacepadDriver();
  ~arSpacepadDriver();

  bool init(arSZGClient&);
  bool start();
  bool stop();
 private:
  arThread _eventThread;

  unsigned short isa_base_addr;
  unsigned short isa_status_addr;
  short recaddr;
  short phaseerror_count;
  float positionScaling;
  float angleScaling;

  // calibration
  arMatrix4 _transmitterOffset;
  arMatrix4 _sensorRot[2];

  arMatrix4 _getSpacepadMatrix(int sensorID);
  int _point_cmd(void);
  int _position_angles_cmd(void);
  int _position_matrix_cmd(void);
  int _hemisphere_cmd(int);

  int _send_isa_cmd(unsigned char* cmd, short cmdsize);
  int _wait_tosend_word(unsigned short word);
  int _send_isa_word(unsigned short word);
  int _read_isa_status();
  int _get_isa_record(short* rxbuf, short recsize, short outputmode);
  long _waitforword();
  long _get_isa_word();
  void _reset_through_isa();
};


#endif
