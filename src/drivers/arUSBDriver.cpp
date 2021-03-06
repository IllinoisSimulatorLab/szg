//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arUSBDriver.h"

#include <string>

DriverFactory(arUSBDriver, "arInputSource")

// From AVR309 application note

  //return values from functions defined in this file
enum {
  USB_NO_ERROR = 0,
  DEVICE_NOT_PRESENT,
  NO_DATA_AVAILABLE,
  INVALID_BAUDRATE,
  OVERRUN_ERROR,
  INVALID_DATABITS,
  INVALID_PARITY,
  INVALID_STOPBITS,
  };

#if 0
typedef unsigned char uchar;
typedef unsigned short ushort;

int __stdcall DoGetInfraCode(uchar * TimeCodeDiagram, int DummyInt, int * DiagramLength);
int __stdcall DoSetDataPortDirection(uchar DirectionByte);
int __stdcall DoGetDataPortDirection(uchar * DataDirectionByte);
int __stdcall DoSetOutDataPort(uchar DataOutByte);
int __stdcall DoGetOutDataPort(uchar * DataOutByte);
int __stdcall DoGetInDataPort(uchar * DataInByte);

int __stdcall DoSetDataPortDirections(uchar DirectionByteB, uchar DirectionByte1, uchar DirectionByte2, uchar UsedPorts);
int __stdcall DoGetDataPortDirections(uchar * DataDirectionByteB, uchar * DataDirectionByteC, uchar * DataDirectionByteD, uchar * UsedPorts);
int __stdcall DoSetOutDataPorts(uchar DataOutByteB, uchar DataOutByteC, uchar DataOutByteD, uchar UsedPorts);
int __stdcall DoGetOutDataPorts(uchar * DataOutByteB, uchar * DataOutByteC, uchar * DataOutByteD, uchar * UsedPorts);
int __stdcall DoGetInDataPorts(uchar * DataInByteB, uchar * DataInByteC, uchar * DataInByteD, uchar * UsedPorts);

int __stdcall DoEEPROMRead(ushort Address, uchar * DataInByte);
int __stdcall DoEEPROMWrite(ushort Address, uchar DataOutByte);
int __stdcall DoRS232Send(uchar DataOutByte);
int __stdcall DoRS232Read(uchar * DataInByte);
//int __stdcall DoSetRS232Baud(int BaudRate);
int __stdcall DoGetRS232Baud(int * BaudRate);
int __stdcall DoGetRS232Buffer(uchar * RS232Buffer, int DummyInt, int * RS232BufferLength);
int __stdcall DoRS232BufferSend(uchar * RS232Buffer, int DummyInt, int *  RS232BufferLength);
int __stdcall DoSetRS232DataBits(uchar DataBits);
int __stdcall DoGetRS232DataBits(uchar * DataBits);
int __stdcall DoSetRS232Parity(uchar Parity);
int __stdcall DoGetRS232Parity(uchar * Parity);
int __stdcall DoSetRS232StopBits(uchar StopBits);
int __stdcall DoGetRS232StopBits(uchar * StopBits);
#endif

// Hand-translated, since no AVR309.lib exists for the prototypes defined above

#define LoByte(_) (_ & 0xff)
#define HiByte(_) ((_ >> 8) & 0xff)

enum {
  FNCNumberDoSetInfraBufferEmpty        =1        , // restart of infra reading (if was stopped by RAM reading)
  FNCNumberDoGetInfraCode                =2        , // transmit of receved infra code (if some code in infra buffer)
  FNCNumberDoSetDataPortDirection        =3        , // set direction of data bits
  FNCNumberDoGetDataPortDirection        =4        , // get direction of data bits
  FNCNumberDoSetOutDataPort                =5        , // set data bits (if are bits as input, then set theirs pull-ups)
  FNCNumberDoGetOutDataPort                =6        , // get data bits (if are bits as input, then get theirs pull-ups)
  FNCNumberDoGetInDataPort                =7        , // get data bits - pin reading
  FNCNumberDoEEPROMRead                        =8        , // read EEPROM from given address
  FNCNumberDoEEPROMWrite                =9        , // write data byte to EEPROM to given address
  FNCNumberDoRS232Send                  =10        , // send one data byte to RS232 line
  FNCNumberDoRS232Read                        =11        , // read one data byte from serial line (only when FIFO not implemented in device)
  FNCNumberDoSetRS232Baud                =12        , // set baud speed of serial line
  FNCNumberDoGetRS232Baud                =13     , // get baud speed of serial line (exact value)
  FNCNumberDoGetRS232Buffer                =14     , // get received bytes from FIFO RS232 buffer
  FNCNumberDoSetRS232DataBits           =15     , // set number of data bits of serial line
  FNCNumberDoGetRS232DataBits           =16     , // get number of data bits of serial line
  FNCNumberDoSetRS232Parity             =17     , // set parity of serial line
  FNCNumberDoGetRS232Parity             =18     , // get parity of serial line
  FNCNumberDoSetRS232StopBits           =19     , // set stopbits of serial line
  FNCNumberDoGetRS232StopBits           =20     , // get stopbits of serial line
};

const double usecPoll = 150000.;
  // 70 msec is so fast that the measurements are pure noise.
  // 300 msec causes wraparound past 127.
// Timing intervals overlap/interleave for x and y.
double usecDuration = usecPoll;
double usecDurationPrev = usecPoll;
double usecDurationPrew = usecPoll;

#ifdef AR_USE_WIN_32

HANDLE SerializationEvent = 0;
const int SerializationEventTimeout = 3000; // 3 seconds to wait to end previous device access
const int OutputDataLength = 256;
char OutputData[OutputDataLength]; // data to USB
DWORD OutLength = 0; // length of data to USB
const char* DrvName = "\\\\.\\AVR309USB_0";
HANDLE DrvHnd = INVALID_HANDLE_VALUE;
bool UARTx2Mode = false;
HANDLE RS232MutexHandle = 0; // serialize access to the AVR USB chip
const char* RS232BufferMutexName = "IgorPlugUSBRS232MutexHandleName";

const char* RS232BufferFileMappedName = "IgorPlugUSBRS232FileMappedHandleName";
const char* SerializationEventName = "IgorPlugDLLEvent";
HANDLE RS232BufferFileMappedHND = 0;

const int cbCG = 10000;
char bufCG[cbCG];
int RS232BufferRead = 0; // read position
int RS232BufferWrite = 0; // write position

DWORD RS232BufferThreadId = 0; // read thread
UINT DoGetRS232BufferTimer = 0; // timer generating event: reading to RS232 from device FIFO in intervals
int RS232BufferTimerInterval = 10; // reading from device FIFO into RS232 buffer, was 10 msec
HANDLE RS232BufferThreadHND = 0; // read thread
HANDLE RS232BufferGetEvent = 0; // signal for read from device FIFO
HANDLE RS232BufferEndEvent = 0; // stop cyclic reading RS232 buffer thread
HANDLE LocalThreadRunningEvent = 0; // end local RS232 buffer thread

inline bool OpenDriver() { // open device driver
  DrvHnd = CreateFile(DrvName, GENERIC_WRITE | GENERIC_READ,
    FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
  return DrvHnd != INVALID_HANDLE_VALUE;
}

inline void CloseDriver() {
  if (DrvHnd != INVALID_HANDLE_VALUE)
    CloseHandle(DrvHnd);
}

void Tick() {
  static bool enableTick = true;
  if (!enableTick) {
    usecDurationPrew = usecDurationPrev;
    usecDurationPrev = usecDuration;
    usecDuration = usecPoll;
    return;
  }

  // Measure duration between now and last time we were here.
  LARGE_INTEGER value; // uninitializable
  static LARGE_INTEGER valuePrev; // uninitializable
  static double scale = 0.;
  static bool fInited = false;
  if (!fInited) {
    LARGE_INTEGER Hz; // uninitializable
    if (!QueryPerformanceFrequency(&Hz)) {
      ar_log_error() << "arUSBdriver: QueryPerformanceFrequency() failed.\n";
      enableTick = false;
      return;
    }
    // Hz is clock ticks per second
    scale = 1000000. / double(Hz.QuadPart); // usec per tick
    fInited = true;
  }
  if (fInited) {
    if (!QueryPerformanceCounter(&value)) {
      ar_log_error() << "arUSBdriver: QueryPerformanceCounter() failed.\n";
      enableTick = false;
      return;
    }
    usecDurationPrew = usecDurationPrev;
    usecDurationPrev = usecDuration;
    usecDuration = double(value.QuadPart - valuePrev.QuadPart) * scale;
    valuePrev = value;
  }
}

bool SendToDriver(int FunctionNumber, int Param1, int Param2, char* pb, DWORD& cb) {
  if (!OpenDriver()) {
    cb = 0;
    return false;
  }
  char bufIn[5] = {
    FunctionNumber,
    LoByte(Param1), HiByte(Param1),
    LoByte(Param2), HiByte(Param2) };
  int OutLengthMax = /* cb<0 ? OutputDataLength : */ cb;
  if (OutLengthMax > 255)
    OutLengthMax = 255;

  if (FunctionNumber == FNCNumberDoRS232Send)
    Tick();

  // Original source code (Delphi 7) repeats 3 times and re-calls OpenDriver.
  const int ok = DeviceIoControl(DrvHnd, 0x808, bufIn, sizeof(bufIn),
    pb, OutLengthMax, &cb, NULL);
  GetLastError();
  CloseDriver();
  return ok && (cb > 0);
}

#define LOCK \
  if (WaitForSingleObject(SerializationEvent, SerializationEventTimeout)==WAIT_TIMEOUT) { \
    ar_log_error() << "arUSBDriver failed to lock mutex.\n"; \
    return DEVICE_NOT_PRESENT; \
  }

#define LOCKBLAH(statement) \
  if (WaitForSingleObject(SerializationEvent, SerializationEventTimeout)==WAIT_TIMEOUT) { \
    ar_log_error() << "arUSBDriver failed to lock mutex.\n"; \
    statement; \
    return DEVICE_NOT_PRESENT; \
  }

#define UNLOCK \
  SetEvent(SerializationEvent);

// Set direction of data bits.  0=in, 1=out.
int DoSetDataPortDirections(char portb, char portc, char portd, char UsedPorts) {
  LOCK
  bool ok = SendToDriver(FNCNumberDoSetDataPortDirection,
      (portc<<8) | portb, (UsedPorts<<8) | portd, OutputData, OutLength);
  UNLOCK
  return ok ? USB_NO_ERROR : DEVICE_NOT_PRESENT;
}

// Set direction of data bits, all ports the same
int DoSetDataPortDirection(char portz) {
  return DoSetDataPortDirections(portz, portz, portz, 0xff);
}

// Get direction of data bits
int DoGetDataPortDirections(char& portb, char& /*portc*/, char& /*portd*/, char& UsedPorts) {
  LOCK
  OutLength = 3;
  if (!SendToDriver(FNCNumberDoGetDataPortDirection, 0, 0, OutputData, OutLength)) {
    UNLOCK
    return DEVICE_NOT_PRESENT;
  }
  UsedPorts = 0;
  if (OutLength > 0) {
    portb = OutputData[0];
    UsedPorts |= 1;
  }
  if (OutLength > 1) {
    portb = OutputData[1];
    UsedPorts |= 2;
  }
  if (OutLength > 2) {
    portb = OutputData[2];
    UsedPorts |= 4;
  }
  UNLOCK
  return USB_NO_ERROR;
}

// Get direction of data bits for Port B
int DoGetDataPortDirection(char& portb) {
  char dummy = 0;
  return DoGetDataPortDirections(portb, dummy, dummy, dummy);
}

// Set output pins' values;  enable input pins' pull-up resistors.
int DoSetOutDataPorts(char portb, char portc, char portd, char UsedPorts) {
  LOCK
  OutLength = 1;
  const bool ok = SendToDriver(FNCNumberDoSetOutDataPort,
    (portc << 8) | portb, (UsedPorts << 8) | portd, OutputData, OutLength);
  UNLOCK
  return ok ? USB_NO_ERROR : DEVICE_NOT_PRESENT;
}

// Set pins, same for all ports
int DoSetOutDataPort(char portz) {
  return DoSetOutDataPorts(portz, portz, portz, 0xff);
}

int DoGetOutDataPorts(char& portb, char& portc, char& portd, char& UsedPorts) {
  LOCK
  OutLength = 3;
  if (!SendToDriver(FNCNumberDoGetOutDataPort, 0, 0, OutputData, OutLength)) {
    UNLOCK
    return DEVICE_NOT_PRESENT;
  }
  UsedPorts = 0;
  if (OutLength > 0) {
    portb = OutputData[0];
    UsedPorts |= 1;
  }
  if (OutLength > 1) {
    portc = OutputData[1];
    UsedPorts |= 2;
  }
  if (OutLength > 2) {
    portd = OutputData[2];
    UsedPorts |= 4;
  }
  UNLOCK
  return USB_NO_ERROR;
}

// get data bits from Port B
int DoGetOutDataPort(char& portb) {
  char dummy = 0;
  return DoGetOutDataPorts(portb, dummy, dummy, dummy);
}

// read all ports
int DoGetInDataPorts(char& portb, char& portc, char& portd, char& UsedPorts) {
  LOCK
  if (!SendToDriver(FNCNumberDoGetInDataPort, 0, 0, OutputData, OutLength)) {
    UNLOCK
    return DEVICE_NOT_PRESENT;
  }
  UsedPorts = 0;
  if (OutLength > 0) {
    portb = OutputData[0];
    UsedPorts |= 1;
  }
  if (OutLength > 1) {
    portc = OutputData[1];
    UsedPorts |= 2;
  }
  if (OutLength > 2) {
    portd = OutputData[2];
    UsedPorts |= 4;
  }
  UNLOCK
  return USB_NO_ERROR;
}

// read port B
int DoGetInDataPort(char& portb) {
  char dummy = 0;
  return DoGetInDataPorts(portb, dummy, dummy, dummy);
}

//////////////////////////////////// RS232 ///////////////////////////////////

int DoRS232Send(char c) {
  LOCK
  OutLength = 1;
  const bool ok = SendToDriver(FNCNumberDoRS232Send, c, 0, OutputData, OutLength);
  UNLOCK
  return ok ? USB_NO_ERROR : DEVICE_NOT_PRESENT;
}

// External function for sending bytes!
int DoRS232BufferSend(const char* rgb, int& cb) {
  bool ok = true;
  int i;
  for (i=0; i<cb; ++i)
    if (DoRS232Send(rgb[i]) == DEVICE_NOT_PRESENT) {
      ok = false;
      break;
    }
  cb = i;
  return ok ? USB_NO_ERROR : DEVICE_NOT_PRESENT;
}

int DoRS232Read(unsigned char& c) {
  LOCK
  OutLength = 3;
  if (!SendToDriver(FNCNumberDoRS232Read, 0, 0, OutputData, OutLength)) {
    UNLOCK
    return DEVICE_NOT_PRESENT;
  }
  if (OutLength == 2) {
    UNLOCK
    return NO_DATA_AVAILABLE;
  }
  bool ok = OutLength != 3;
  c = *OutputData;
  UNLOCK
  return ok ? USB_NO_ERROR : OVERRUN_ERROR;
}

int DoGetRS232Baud(int* BaudRate) {
  LOCK
  OutLength = 2;
  if (!SendToDriver(FNCNumberDoGetRS232Baud, 0, 0, OutputData, OutLength)) {
    UNLOCK
    return DEVICE_NOT_PRESENT;
  }
  UARTx2Mode = OutLength > 1;
  if (!UARTx2Mode)
    OutputData[1] = 0; // ATmega returns 2 byte answer, others 1 byte
  else
    ar_log_error() << "arUSBDriver expected AT90S2313 not ATmega microcontroller.\n";
  *BaudRate = int(12e6/(16*(255.0*OutputData[1]+OutputData[0]+1.0)));
  if (UARTx2Mode)
    *BaudRate *= 2; // ATmega has x2 mode on UART
  UNLOCK
  return USB_NO_ERROR;
}

int DoSetRS232Baud(int BaudRate) {
  int BaudRateByte = 0;
  double BaudRateDouble = 0.;
  int MaxBaudRateByte = 0;
  DoGetRS232Baud(&MaxBaudRateByte); // no-op since MaxBaudRateByte gets clobbered below?
  if (UARTx2Mode) {
    BaudRate /= 2;
    MaxBaudRateByte = 4095;
  }
  else {
    MaxBaudRateByte = 255;
  }

  const double Error = 0.04; // 4% max error
  const double MaxError = 1 + Error;
  const double MinError = 1 - Error;
  if (BaudRate >= 12e6/16*MaxError)
    return INVALID_BAUDRATE;
  if (BaudRate <= 12e6/(16*(MaxBaudRateByte+1))*MinError)
    return INVALID_BAUDRATE;

  BaudRateDouble = 12e6/(16*BaudRate) - 1;
  if (BaudRateDouble<0.)
    BaudRateDouble = 0.;
  if (BaudRateDouble>MaxBaudRateByte)
    BaudRateDouble = MaxBaudRateByte;
  const double BaudError = 12e6/(16*(BaudRateDouble+1))/BaudRate;
  if (BaudError>MaxError || BaudError<MinError)
    return INVALID_BAUDRATE;

  BaudRateByte = int(BaudRateDouble);
  LOCK
  OutLength = 1;
  const bool ok = SendToDriver(FNCNumberDoSetRS232Baud, LoByte(BaudRateByte), HiByte(BaudRateByte), OutputData, OutLength);
  UNLOCK
  return ok ? USB_NO_ERROR : DEVICE_NOT_PRESENT;
}

// External function for getting the bytes!
int DoGetRS232Buffer(char* RS232Buffer, int& RS232BufferLength) {
  int i;
  for (i=0; i<RS232BufferLength; ++i) {
    if (RS232BufferRead == RS232BufferWrite)
      // underflow:  read index caught up with write index
      break;
    RS232Buffer[i] = bufCG[RS232BufferRead++];
    RS232BufferRead %= cbCG;
  }
  RS232BufferLength = i;
  return NO_ERROR;
}

int DoGetRS232BufferLocal(char* RS232Buffer, int& RS232BufferLength) {
  if (RS232BufferLength<=0) {
    RS232BufferLength = 0;
    return USB_NO_ERROR;
  }
  LOCKBLAH(RS232BufferLength = 0);
  int i = 0;
  const int HeaderLength = UARTx2Mode ? 2 : 1;
  do {
    OutLength = RS232BufferLength + HeaderLength - i;
    if (OutLength > OutputDataLength)
      OutLength = OutputDataLength;
    if (!SendToDriver(FNCNumberDoGetRS232Buffer, 0, 0, OutputData, OutLength)) {
      RS232BufferLength = 0;
      UNLOCK
      return DEVICE_NOT_PRESENT;
    }
    if (OutLength <= 1)
      break;
    for (int j = i; j < i+OutLength-HeaderLength; ++j)
      RS232Buffer[j] = OutputData[j-i+HeaderLength];
    i += OutLength-HeaderLength;
  } while (i < RS232BufferLength);
  RS232BufferLength = i;
  UNLOCK
  return USB_NO_ERROR;
}

bool fThreadDied = false;

// system-unique thread for reading device RS232 FIFO into bufCG
DWORD WINAPI DoGetRS232BufferThreadProc(LPVOID /*Parameter*/) {
  LocalThreadRunningEvent = CreateEvent(NULL, false, false, NULL);

  const HANDLE Handles[2] = { RS232BufferGetEvent, RS232BufferEndEvent };
  DoGetRS232BufferTimer = timeSetEvent(RS232BufferTimerInterval, 1,
    (LPTIMECALLBACK)RS232BufferGetEvent, 0,
    TIME_PERIODIC | TIME_CALLBACK_EVENT_SET);
  for (;;) {
    // every RS232BufferTimerInterval msec
    switch (WaitForMultipleObjects(2, Handles, false, INFINITE)) {
    case WAIT_OBJECT_0:
      do {
          // read to end of bufCG
        int BufferLength = cbCG - RS232BufferWrite;

        if (DoGetRS232BufferLocal(bufCG + RS232BufferWrite, BufferLength) != USB_NO_ERROR) {
          ar_log_error() << "arUSBDriver: rs232 problem\n";
          // wait 2 seconds if no answer, to save bandwidth for non-rs232 devices
          if (WaitForSingleObject(RS232BufferEndEvent, 2000) != WAIT_TIMEOUT)
            goto LDone;
        }
        // now BufferLength is how many bytes were actually read from the USB hardware
        RS232BufferWrite += BufferLength;
        if (RS232BufferWrite >= cbCG)
          RS232BufferWrite = 0;
      } while (false); // I don't think this loop ever needs to run more than once
      break;
    case WAIT_OBJECT_0+1:
      // RS232BufferEndEvent
      goto LDone;
    }
  }
LDone:
  if (DoGetRS232BufferTimer) {
    timeKillEvent(DoGetRS232BufferTimer);
    DoGetRS232BufferTimer = 0;
  }
  SetEvent(LocalThreadRunningEvent);
  fThreadDied = true;
  return 0;
}

// Return false on error.
bool InitLowLevel() {
  // Create buffers, threads, events and synchronization object.
  RS232MutexHandle = CreateMutex(NULL, false, RS232BufferMutexName);
  if (RS232MutexHandle && (GetLastError() == ERROR_ALREADY_EXISTS))
    return false;
  SerializationEvent = CreateEvent(NULL, false, true, SerializationEventName);
    // Auto-reset, i.e. resets to nonsignaled (after SetEvent), once thread is released.
    // Initially signalled.
  RS232BufferGetEvent = CreateEvent(NULL, false, false, NULL); // start reading device fifo
    // Auto-reset.  Initially nonsignalled.
  RS232BufferEndEvent = CreateEvent(NULL, false, false, NULL); // finish fifo reading thread
    // Auto-reset.  Initially nonsignalled.

  // (securityattr, stacksize, pfn, pparam, flags, &id)
  fThreadDied = false;
  RS232BufferThreadHND = CreateThread(NULL, 0, DoGetRS232BufferThreadProc, NULL, CREATE_SUSPENDED, &RS232BufferThreadId); // create FIFO reading thread (suspended)
  SetThreadPriority(RS232BufferThreadHND, THREAD_PRIORITY_TIME_CRITICAL); // highest priority for FIFO reading thread
  ResumeThread(RS232BufferThreadHND); // start FIFO reading thread
  ar_usleep(150000); // give thread a chance to abort
  return !fThreadDied;
}

void StopLowLevel() {
  if (RS232BufferThreadHND) {
    if (DoGetRS232BufferTimer) {
      timeKillEvent(DoGetRS232BufferTimer);
      DoGetRS232BufferTimer = 0;
    }
    SetEvent(RS232BufferEndEvent);
    if (LocalThreadRunningEvent) {
      WaitForSingleObject(LocalThreadRunningEvent, 5000);
      WaitForSingleObject(RS232BufferThreadHND, 10);
      LocalThreadRunningEvent = 0;
    } else {
      WaitForSingleObject(RS232BufferThreadHND, 400);
    }
    CloseHandle(RS232BufferThreadHND);
  }
  if (LocalThreadRunningEvent) {
    CloseHandle(LocalThreadRunningEvent);
    LocalThreadRunningEvent = NULL;
  }
  CloseHandle(RS232BufferGetEvent);
  CloseHandle(RS232BufferEndEvent);
  CloseHandle(SerializationEvent);
  ReleaseMutex(RS232MutexHandle);
  CloseHandle(RS232MutexHandle);
}

#else
int DoRS232BufferSend(const char*, int&) { return -1; }
int DoGetRS232Buffer(char*, int&) { return -1; }
int DoSetOutDataPort(char) { return -1; }
int DoSetDataPortDirection(char) { return -1; }
int DoGetInDataPort(char&) { return -1; }
#endif

// End of AVR309 application note

arUSBDriver::arUSBDriver() :
  _inited( false ),
  _stopped( true ),
  _eventThreadRunning( false )
{
}

arUSBDriver::~arUSBDriver() {
}

bool arUSBDriver::init(arSZGClient&) {
#ifndef AR_USE_WIN_32
  ar_log_error() << "arUSBDriver requres Windows, sorry.\n";
  return false;
#else
  if (!InitLowLevel()) {
    return false;
  }
  const int baudSet = 57692; // close enough to 57600
  int baud = -1;
  if (DoSetRS232Baud(baudSet) != USB_NO_ERROR || DoGetRS232Baud(&baud) != USB_NO_ERROR) {
    ar_log_error() << "arUSBDriver failed to set+get baud rate.\n";
    return false;
  }
  if (baud != baudSet) {
    ar_log_error() << "arUSBDriver expected " << baudSet << " baud, not " << baud << ".\n";
    return false;
  }
  _inited = true;
  _setDeviceElements(3, 2, 0);
  return true;
#endif
}

void ar_USBDriverDataTask(void* pv) {
  ((arUSBDriver*)pv)->_dataThread();
}

bool arUSBDriver::start() {
#ifndef AR_USE_WIN_32
  ar_log_error() << "arUSBDriver requres Windows, sorry.\n";
  return false;
#else
  if (!_inited) {
    ar_log_error() << "arUSBDriver can't start before init.\n";
    return false;
  }
  return _eventThread.beginThread(ar_USBDriverDataTask, this);
#endif
}

bool arUSBDriver::stop() {
  if (_stopped)
    return true;
  _stopped = true;
#ifdef AR_USE_WIN_32
  StopLowLevel();
  arSleepBackoff a(8, 20, 1.2);
  while (_eventThreadRunning)
    a.sleep();
#endif
  return true;
}

void BlinkLED(bool on = true) {
    DoSetOutDataPort(on ? 0xfb : 0xff); // LED is on D2; no other outputs.  Keep inputs' pull-ups enabled.
}

void arUSBDriver::_dataThread() {
  _stopped = false;
  _eventThreadRunning = true;
  DoSetDataPortDirection(0x04); // D2 is the only output pin (activity/heartbeat LED).

  // Initial default values: buttons up, joystick centered.
  queueButton(0, 0);
  queueButton(1, 0);
  queueButton(2, 0);
  queueAxis(0, 0.);
  queueAxis(1, 0.);
  sendQueue();

  BlinkLED(); // enable all input pull-ups.
  while (!_stopped && _eventThreadRunning) {
    // Throttle.
    ar_log_remark() << "arUSBDriver: ar_usleeping " << int(usecPoll)/1000 << " msec.\n";
    ar_usleep(int(usecPoll));

    int cb = 2;
    if (0 != DoRS232BufferSend("aa", cb)) {
      ar_log_error() << "arUSBDriver failed to poll joystick.\n";
      continue;
    }
    if (cb != 2) {
      ar_log_error() << "arUSBDriver polled joystick with not 2 but " << cb << " bytes.\n";
      continue;
    }
    char buf[4];
    cb = 2;
    if (0 != DoGetRS232Buffer(buf, cb)) {
      ar_log_error() << "arUSBDriver: no response from joystick.\n";
      continue;
    }
    if (cb != 2) {
      ar_log_error() << "arUSBDriver: joystick responded with not 2 but " << cb << " bytes.\n";
      continue;
    }

    // Could be off-by-one out of sync buf[0] and buf[1].
    // But it's always x which has the high bit set.
    const int x  =  int(buf[1] & ~0xffffff80);
    const int y  =  int(buf[0]);
    const int x_ =  int(buf[0] & ~0xffffff80);
    const int y_ =  int(buf[1]);

    if (x == 0 && y == 0 && x_ == 0 && y_ == 0) {
      ar_log_error() << "arUSBDriver: joystick responded with zeros.\n";
      continue;
    }

    // At least one byte should have the high bit set.
    if (x == 0 && y == 0 && x_ == 0 && y_ == 0) {
      ar_log_error() << "arUSBDriver: joystick responded with no high bit set.\n";
ar_log_debug() << "raw xy: " << x << ", " << y << ";    " << x_ << ", " << y << "\n";;;;
      continue;
    }

ar_log_debug() << "raw xy: " << x << ", " << y << ";    " << x_ << ", " << y << "\n";;;;

    // Use the pair with one negative value.
    // Correct, approximately, for the measured sleep duration (not just ar_usleep(usecPoll).
    float xUse = 0., yUse = 0.;
    static float xUsePrev = 0., yUsePrev = 0.;
    if (x*y <= 0) {
      xUse = x * usecPoll / (usecDuration + usecDurationPrev);
      yUse = y * usecPoll / (usecDurationPrev + usecDurationPrew);
    }
    else if (x_*y_ <= 0) {
      xUse = x_ * usecPoll / (usecDurationPrev + usecDurationPrew);
      yUse = y_ * usecPoll / (usecDuration + usecDurationPrev);
    }
    else {
      // At least one byte should have the high bit set.
      ar_log_error() << "arUSBDriver: bogus data from MCU: " << ar_hex << buf[0] << buf[1] << "\n";
      break;
    }

    // Autocalibrate: measure the mean x and y over the first 3 seconds.
    const int cInit = int(3000000. / usecPoll);
    static int iInit = cInit;
    static float xAvg = 0., yAvg = 0., xAvgPrev = 0., yAvgPrev = 0.;
    static float xMin = 0., xMax = 0., yMin = 0., yMax = 0.;
    if (iInit > 0) {
      if (iInit == cInit)
        ar_log_critical() << "arUSBDriver calibrating.  Don't wiggle joystick yet.\n";
      xAvg += xUse;
      yAvg += yUse;
      if (--iInit > 0)
        continue;
      ar_log_critical() << "arUSBDriver calibrated.  OK to wiggle joystick now.\n";
      xAvg /= cInit;
      yAvg /= cInit;
ar_log_debug() << "\txy average = " << xAvg << ", " << yAvg << "\n";;;;
      xAvgPrev = xAvg;
      yAvgPrev = yAvg;
      // These offsets avoid divide by zero.
      // Offsets not too big though, for this initial estimate.
      // 7 is experimentally determined.
      xMin = xAvg - 7.;
      xMax = xAvg + 7.;
      yMin = yAvg - 7.;
      yMax = yAvg + 7.;
      xUsePrev = xUse;
      yUsePrev = yUse;
    }
    BlinkLED(false);

    // Reduce noise with moving-average filter.
    xUse = xUse * .3 + xUsePrev * .7;
    yUse = yUse * .3 + yUsePrev * .7;
    xUsePrev = xUse;
    yUsePrev = yUse;

    // Update xMin xMax yMin yMax
    if (xUse < xMin)
      xMin = xUse;
    if (xUse > xMax)
      xMax = xUse;
    if (yUse < yMin)
      yMin = yUse;
    if (yUse > yMax)
      yMax = yUse;
ar_log_remark() << "use xy: " << xUse << ", " << yUse << "\n";

    // Scale to [-1, 1] by lerping xUse w.r.t. xMin xMax xAvg.
    // Though an exponential curve fit would better model the 555 timing circuit.
    float xx = (xUse <= xAvg) ?
      (xUse - xMin) / (xAvg - xMin) - 1. :
      (xUse - xAvg) / (xMax - xAvg);
    float yy = (yUse <= yAvg) ?
      (yUse - yMin) / (yAvg - yMin) - 1. :
      (yUse - yAvg) / (yMax - yAvg);
    const float xxAbs = fabs(xx);
    const float yyAbs = fabs(yy);

ar_log_remark() << "Nrm xy: " << xx << ", " << yy << "\n";
    // Update xAvg yAvg, to correct for long-term thermal drift and cpu load.
    // But not when the joystick is likely near an extreme.
    if (xxAbs < .5) {
      xAvg = xUse * .02 + xAvgPrev * .98;
      xAvgPrev = xAvg;
    }
    if (yyAbs < .5) {
      yAvg = yUse * .02 + yAvgPrev * .98;
      yAvgPrev = yAvg;
    }

    // dead zone, to reduce noise easily seen when joystick's at rest
    if (xxAbs < .25)
      xx = 0.;
    if (yyAbs < .25)
      yy = 0.;

    if (xxAbs > .7*.6 || yyAbs > .55*.6) {
      //printf("xy  %.2f   %4.2f\n", xxAbs, yyAbs);
      BlinkLED();
    }

//        printf("X %.1f %.1f %.1f   %5.0f = %5.2f           Y %.1f %.1f %.1f  %5.0f = %5.2f\n",
//          xMin, xAvg, xMax, xUse, xx,
//          yMin, yAvg, yMax, yUse, yy);

        printf("\t\t\tmsec: %6.1f %6.1f %6.1f\n", usecDuration/1000., usecDurationPrev/1000., usecDurationPrew/1000.);
        // ar_usleep failing?  Why 4.0 or 13.5 msec, +- .7 ?  Expected 150.

    queueAxis(0, -xx);
    queueAxis(1, yy);

    // Poll wand's buttons.
    // todo: poll buttons much faster than RS232-based joystick.
    static bool fButtonDown[3] = {0};
    char cButtons = 0;
    if (DoGetInDataPort(cButtons) != USB_NO_ERROR) {
      ar_log_error() << "arUSBDriver failed to read MCU's input pins.\n";
    }
    else {
      for (int i=0; i<3; ++i) {
        // Read pins D3 D4 D5.
        const bool fDown = !(cButtons & (1 << (i+3))); // 1=pulled up to 5V, 0=switched to ground.
        // Send only state changes.
        if (fDown != fButtonDown[i]) {
          queueButton(i, fDown);
          fButtonDown[i] = fDown;
          BlinkLED();
        }
      }
    }

    sendQueue();
  }
  _eventThreadRunning = false;
}
