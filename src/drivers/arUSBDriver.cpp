//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include <string>
#include <iostream>
#include <sstream>
#include "arDataUtilities.h"
#include "arUSBDriver.h"

// Methods used by the dynamic library mappers. 
extern "C"{
  SZG_CALL void* factory(){
    return new arUSBDriver();
  }

  SZG_CALL void baseType(char* buffer, int size){
    ar_stringToBuffer("arInputSource", buffer, size);
  }
}

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
  FNCNumberDoSetInfraBufferEmpty	=1	, // restart of infra reading (if was stopped by RAM reading)
  FNCNumberDoGetInfraCode		=2	, // transmit of receved infra code (if some code in infra buffer)
  FNCNumberDoSetDataPortDirection	=3	, // set direction of data bits
  FNCNumberDoGetDataPortDirection	=4	, // get direction of data bits
  FNCNumberDoSetOutDataPort		=5	, // set data bits (if are bits as input, then set theirs pull-ups)
  FNCNumberDoGetOutDataPort		=6	, // get data bits (if are bits as input, then get theirs pull-ups)
  FNCNumberDoGetInDataPort		=7	, // get data bits - pin reading
  FNCNumberDoEEPROMRead			=8	, // read EEPROM from given address
  FNCNumberDoEEPROMWrite		=9	, // write data byte to EEPROM to given address
  FNCNumberDoRS232Send  		=10	, // send one data byte to RS232 line
  FNCNumberDoRS232Read			=11	, // read one data byte from serial line (only when FIFO not implemented in device)
  FNCNumberDoSetRS232Baud		=12	, // set baud speed of serial line
  FNCNumberDoGetRS232Baud		=13     , // get baud speed of serial line (exact value)
  FNCNumberDoGetRS232Buffer		=14     , // get received bytes from FIFO RS232 buffer
  FNCNumberDoSetRS232DataBits           =15     , // set number of data bits of serial line
  FNCNumberDoGetRS232DataBits           =16     , // get number of data bits of serial line
  FNCNumberDoSetRS232Parity             =17     , // set parity of serial line
  FNCNumberDoGetRS232Parity             =18     , // get parity of serial line
  FNCNumberDoSetRS232StopBits           =19     , // set stopbits of serial line
  FNCNumberDoGetRS232StopBits           =20     , // get stopbits of serial line
};

#ifdef AR_USE_WIN_32

HANDLE SerializationEvent = 0;
const int SerializationEventTimeout = 5000; // 5 seconds to wait to end previous device access
const int OutputDataLength = 256;
char OutputData[OutputDataLength]; // data to USB
DWORD OutLength = 0; // length of data to USB
const char* DrvName = "\\\\.\\AVR309USB_0";
HANDLE DrvHnd = INVALID_HANDLE_VALUE;
bool UARTx2Mode = false;
bool IsRS232PrevInst = true;
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
int RS232BufferTimerInterval = 35; // reading from device FIFO into RS232 buffer, was 10 msec
HANDLE RS232BufferThreadHND = 0; // read thread
HANDLE RS232BufferGetEvent = 0; // signal for read from device FIFO
HANDLE RS232BufferEndEvent = 0; // stop cyclic reading RS232 buffer thread
HANDLE LocalThreadRunningEvent = 0; // end local RS232 buffer thread

void ShowThreadErrorMessage()
{
  //;;;; called in only one place?
  cerr << "arUSBDriver error: failed to acquire mutex lock.\n";
}

inline bool OpenDriver() { // open device driver
  DrvHnd = CreateFile(DrvName, GENERIC_WRITE | GENERIC_READ,
    FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0);
  return DrvHnd != INVALID_HANDLE_VALUE;
}

inline void CloseDriver() {
  if (DrvHnd != INVALID_HANDLE_VALUE)
    CloseHandle(DrvHnd);
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
  int OutLengthMax = cb<0 ? OutputDataLength : cb;
  if (OutLengthMax > 255)
    OutLengthMax = 255;
  // Original source code (Delphi 7) repeats 3 times and re-calls OpenDriver.
  const int ok = DeviceIoControl(DrvHnd, 0x808, bufIn, sizeof(bufIn),
    pb, OutLengthMax, &cb, NULL);
  GetLastError();
  CloseDriver();
  return ok && (cb > 0);
}

#define LOCK \
  if (WaitForSingleObject(SerializationEvent, SerializationEventTimeout)==WAIT_TIMEOUT) { \
    ShowThreadErrorMessage(); \
    return DEVICE_NOT_PRESENT; \
  }

#define LOCKBLAH(statement) \
  if (WaitForSingleObject(SerializationEvent, SerializationEventTimeout)==WAIT_TIMEOUT) { \
    ShowThreadErrorMessage(); \
    statement; \
    return DEVICE_NOT_PRESENT; \
  }

#define UNLOCK \
  SetEvent(SerializationEvent);

// Set direction of data bits
int DoSetDataPortDirections(char portb, char portc, char portd, char UsedPorts) {
  short Param1, Param2;
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
int DoGetDataPortDirections(char& portb, char& portc, char& portd, char& UsedPorts) {
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
  char dummy;
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
  for (int i=0; i<cb; ++i)
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
  if (!SendToDriver(FNCNumberDoGetRS232Baud,0,0,OutputData,OutLength)) {
    UNLOCK
    return DEVICE_NOT_PRESENT;
  }
  UARTx2Mode = OutLength > 1;
  if (!UARTx2Mode)
    OutputData[1] = 0; // ATmega returns 2 byte answer, others 1 byte
  else
    cerr << "warning: expected AT90S2313 not ATmega chip.\n";
  *BaudRate = int(12e6/(16*(255.0*OutputData[1]+OutputData[0]+1.0)));
  if (UARTx2Mode)
    *BaudRate *= 2; // ATmega has x2 mode on UART
  UNLOCK
  return USB_NO_ERROR;
}

int DoSetRS232Baud(int BaudRate) {
  int BaudRateByte;
  double BaudRateDouble;
  int MaxBaudRateByte;
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
  const bool ok = SendToDriver(FNCNumberDoSetRS232Baud,LoByte(BaudRateByte),HiByte(BaudRateByte),OutputData,OutLength);
  UNLOCK
  return ok ? USB_NO_ERROR : DEVICE_NOT_PRESENT;
}

// init system-unique RS232 buffer (memory mapped file)
void InitRS232ExclusiveSystemBuffer() {
  RS232MutexHandle = CreateMutex(NULL, false, RS232BufferMutexName);
  IsRS232PrevInst = RS232MutexHandle && (GetLastError() == ERROR_ALREADY_EXISTS);
  if (IsRS232PrevInst) {
    // Get value from where previous instance stuffed it in buf itself.
    cerr << "bad things will happen.  should abort.\n";;;;
  }
}

// External function for getting the bytes!
int DoGetRS232Buffer(char* RS232Buffer, int& RS232BufferLength) {
  int BufferLength = 0;
  for (int i=0; i<RS232BufferLength; ++i) {
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

// system-unique thread for reading device RS232 FIFO into bufCG
DWORD WINAPI DoGetRS232BufferThreadProc(LPVOID Parameter) {
  const HANDLE MutexHandles[2] = { RS232MutexHandle, RS232BufferEndEvent };
  LocalThreadRunningEvent = CreateEvent(NULL, false, false, NULL);
  if (IsRS232PrevInst) {
    SetEvent(LocalThreadRunningEvent);
    return 42;
  }

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
	//;;;; get ACCURATE timing between now and last time.  queryperformancecounter ?
	if (DoGetRS232BufferLocal(bufCG + RS232BufferWrite, BufferLength) != USB_NO_ERROR) {
          cerr << "arUSBDriver warning: rs232 problem\n";
	  // wait 2 seconds if no answer, to save bandwidth for non-rs232 devices
	  if (WaitForSingleObject(RS232BufferEndEvent,2000) != WAIT_TIMEOUT)
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
}

void InitLowLevel() {
  // Create buffers, threads, events and synchronization object.
  InitRS232ExclusiveSystemBuffer();
  SerializationEvent = CreateEvent(NULL, false, true, SerializationEventName);
    // Auto-reset, i.e. resets to nonsignaled (after SetEvent), once thread is released.
    // Initially signalled.
  RS232BufferGetEvent = CreateEvent(NULL, false, false, NULL); // start reading device fifo
    // Auto-reset.  Initially nonsignalled.
  RS232BufferEndEvent = CreateEvent(NULL, false, false, NULL); // finish fifo reading thread
    // Auto-reset.  Initially nonsignalled.

  // (securityattr, stacksize, pfn, pparam, flags, &id)
  RS232BufferThreadHND = CreateThread(NULL, 0, DoGetRS232BufferThreadProc, NULL, CREATE_SUSPENDED, &RS232BufferThreadId); // create FIFO reading thread (suspended)
  SetThreadPriority(RS232BufferThreadHND, THREAD_PRIORITY_TIME_CRITICAL); // highest priority for FIFO reading thread, ;; originally THREAD_PRIORITY_TIME_CRITICAL not THREAD_PRIORITY_HIGHEST
  ResumeThread(RS232BufferThreadHND); // start FIFO reading thread
}

void CloseRS232ExclusiveSystemBuffer() {
  if (!IsRS232PrevInst)
    ReleaseMutex(RS232MutexHandle);
  CloseHandle(RS232MutexHandle);
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
  CloseRS232ExclusiveSystemBuffer();
}

#else
int DoRS232BufferSend(const char*, int&) { return -1; }
int DoGetRS232Buffer(char*, int&) { return -1; }
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
  cerr << "arUSBDriver error: Windows-only.\n";
  return false;
#else
  InitLowLevel();
  const int baudSet = 57692; // close enough to 57600
  int baud = -1;
  if (DoSetRS232Baud(baudSet) != USB_NO_ERROR || DoGetRS232Baud(&baud) != USB_NO_ERROR){
    cerr << "arUSBDriver error: failed to set+get baud rate.\n";
    return false;
  }
  if (baud != baudSet) {
    cerr << "arUSBDriver error: expected " << baudSet << " baud, not "
         << baud << ".\n";
    return false;
  }
  _inited = true;
  _setDeviceElements(3, 2, 0);
  return true;
#endif
}

bool arUSBDriver::start() {
#ifndef AR_USE_WIN_32
  cerr << "arUSBDriver error: Windows-only.\n";
  return false;
#else
  if (!_inited) {
    cerr << "arUSBDriver::start() error: Not inited yet.\n";
    return false;
  }
  return _eventThread.beginThread(ar_USBDriverDataTask,this);
#endif
}

bool arUSBDriver::stop() {
  if (_stopped)
    return true;
  _stopped = true;
#ifdef AR_USE_WIN_32
  StopLowLevel();
  while (_eventThreadRunning)
    ar_usleep(10000);
#endif
  return true;
}

void ar_USBDriverDataTask(void* pv) {
  ((arUSBDriver*)pv)->_dataThread();
}

void arUSBDriver::_dataThread() {
  cerr << "todo: set data directions, poll pins / queueButton(), blinkenlight activity (see syzdot)\n";
  _stopped = false;
  _eventThreadRunning = true;
  while (!_stopped && _eventThreadRunning) {
    ar_usleep(10000); // throttle

    int cb = 2;
    if (0 != DoRS232BufferSend("aa", cb)) {
      cerr << "arUSBDriver warning: failed to poll joystick.\n";
      continue;
    }
    if (cb != 2) {
      cerr << "arUSBDriver warning: polled joystick with not 2 but " << cb << " bytes.\n";
      continue;
    }
    char buf[4];
    cb = 2;
    if (0 != DoGetRS232Buffer(buf, cb))
      cerr << "arUSBDriver warning: joystick failed to respond to poll.\n";
    if (cb != 2)
      cerr << "arUSBDriver warning: joystick responded with not 2 but " << cb << " bytes.\n";

    const int x = int(buf[1]); // 35 neutral, 55 left, 27 right
    const int y = -int(buf[0]); // 90 neutral, 63 forward, 100 backward
    // scale to [-1,1]
    float xx = (35-x) / 15.;
//  if (xx < 0.1)
//    xx *= 3.;
    float yy = (90-y) / 25.;
//  if (yy > 0.1)
//    yy *= 3.;

    cout << "XY: " << x << " ," << y << endl;
    queueAxis(0, xx);
    queueAxis(1, yy);

    // todo: queueButton()
    sendQueue();

    ar_usleep(100000);
  }
  _eventThreadRunning = false;
}

bool arUSBDriver::restart() {
  return stop() && start();
}
