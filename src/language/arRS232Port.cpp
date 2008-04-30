//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arRS232Port.h"
#include "arDataUtilities.h"
#include "arLogStream.h"

#ifdef AR_USE_LINUX
#include <unistd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <sys/time.h>
#include <fcntl.h>
#include <sys/signal.h>
#endif

#ifdef AR_USE_WIN_32
#include <time.h>
#endif

#include <stdio.h>
#include <memory.h>
#include <math.h>
#include <iostream>
#include <string>

/*
 Read timeouts: ar_read() blocks until either the requested
 number of characters is received or the timeout interval elapses.
 This is trivial in Win32, but not in Posix.

 Some reference material:

 Win32 Timeout Info (From MSDN):

 ReadIntervalTimeout 
  Specifies the maximum time, in milliseconds, allowed to elapse between
  the arrival of two characters on the communications line. During a
  ReadFile operation, the time period begins when the first character is
  received. If the interval between the arrival of any two characters
  exceeds this amount, the ReadFile operation is completed and any
  buffered data is returned. A value of zero indicates that interval
  time-outs are not used.  A value of MAXDWORD, combined with zero values
  for both the ReadTotalTimeoutConstant and ReadTotalTimeoutMultiplier
  members, specifies that the read operation is to return immediately with
  the characters that have already been received, even if no characters
  have been received.

 ReadTotalTimeoutMultiplier 
  Specifies the multiplier, in milliseconds, used to calculate the
  total time-out period for read operations. For each read operation,
  this value is multiplied by the requested number of bytes to be read.

 ReadTotalTimeoutConstant 
  Specifies the constant, in milliseconds, used to calculate the
  total time-out period for read operations. For each read operation,
  this value is added to the product of the ReadTotalTimeoutMultiplier
  member and the requested number of bytes.  A value of zero for both
  the ReadTotalTimeoutMultiplier and ReadTotalTimeoutConstant members
  indicates that total time-outs are not used for read operations.

 WriteTotalTimeoutMultiplier 

  Specifies the multiplier, in milliseconds, used to calculate the
  total time-out period for write operations. For each write operation,
  this value is multiplied by the number of bytes to be written.
  WriteTotalTimeoutConstant Specifies the constant, in milliseconds,
  used to calculate the total time-out period for write operations. For
  each write operation, this value is added to the product of the
  WriteTotalTimeoutMultiplier member and the number of bytes to be
  written.  A value of zero for both the WriteTotalTimeoutMultiplier
  and WriteTotalTimeoutConstant members indicates that total time-outs
  are not used for write operations.

 Remarks
  If an application sets ReadIntervalTimeout and
  ReadTotalTimeoutMultiplier to MAXDWORD and sets ReadTotalTimeoutConstant
  to a value greater than zero and less than MAXDWORD, one of the
  following occurs when the ReadFile function is called: If there are any
  characters in the input buffer, ReadFile returns immediately with the
  characters in the buffer.  If there are no characters in the input
  buffer, ReadFile waits until a character arrives and then returns
  immediately.  If no character arrives within the time specified by
  ReadTotalTimeoutConstant, ReadFile times out.

 From The Serial Programming Guide for POSIX Operating Systems by Michael R. Sweet
 
 Setting Read Timeouts

  UNIX serial interface drivers provide the ability to specify character
  and packet timeouts. Two elements of the c_cc array are used for
  timeouts: VMIN and VTIME. Timeouts are ignored in canonical input mode
  or when the NDELAY option is set on the file via open or fcntl.

  VMIN specifies the minimum number of characters to read. If it is
  set to 0, then the VTIME value specifies the time to wait for every
  character read.  This does not mean that a read call for N bytes will
  wait for N characters to come in. Rather, the timeout will apply to the
  first character and the read call will return the number of characters
  immediately available (up to the number you request).

  If VMIN is non-zero, VTIME specifies the time to wait for the first
  character read. If a character is read within the time given, any read
  will block (wait) until all VMIN characters are read. That is, once the
  first character is read, the serial interface driver expects to receive
  an entire packet of characters (VMIN bytes total). If no character is
  read within the time allowed, then the call to read returns 0. This
  method allows you to tell the serial driver you need exactly N bytes
  and any read call will return 0 or N bytes. However, the timeout only
  applies to the first character read, so if for some reason the driver
  misses one character inside the N byte packet then the read call could
  block forever waiting for additional input characters.

  VTIME specifies the amount of time to wait for incoming characters
  in tenths of seconds. If VTIME is set to 0 (the default), reads will
  block indefinitely unless the NDELAY option is set on the port with
  open or fcntl.

*/

static bool nyi() {
  ar_log_error() << "arRS232Port: implemented only for Win32 and Linux.\n";
  return false;
}

arRS232Port::arRS232Port() :
  _isOpen(false),
  _readTimeoutTenths(10)
{
#ifdef AR_USE_WIN_32
  _timeoutStruct.ReadIntervalTimeout = 0; 
  _timeoutStruct.ReadTotalTimeoutMultiplier = 0; 
  _timeoutStruct.ReadTotalTimeoutConstant = 1000;
  _timeoutStruct.WriteTotalTimeoutMultiplier = 0;
  _timeoutStruct.WriteTotalTimeoutConstant = 100;
#endif
}

arRS232Port::~arRS232Port() {
  if (_isOpen)
    ar_close();
}

static inline bool fltcomp(float a, float b) {
  return fabs(a-b) < 1.0e-6;
}

bool arRS232Port::ar_open( const unsigned port, const unsigned long baud,
                      const unsigned dBits, const float stBits,
                      const std::string& par ) {
#if defined( AR_USE_WIN_32 )
  const unsigned portMin = 1;
#elif defined( AR_USE_LINUX )
  //unused const unsigned portMin = 0;
#else
  return nyi();
#endif

#if defined( AR_USE_WIN_32 )
  // This check is meaningful only when portMin>0, since portMin is unsigned.
  if (port < portMin) {
    ar_log_error() << "arRS232Port: port numbers are 1-based.\n";
    return false;
  }
#endif

  if (_isOpen) {
    ar_log_error() << "arRS232Port: port already in use.\n";
    return false;
  }

#ifdef AR_USE_WIN_32
  DWORD baudRate = 0;
  switch (baud) {
    case 110:
      baudRate = CBR_110;
      break;
    case 300:
      baudRate = CBR_300;
      break;
    case 600:
      baudRate = CBR_600;
      break;
    case 1200:
      baudRate = CBR_1200;
      break;
    case 2400:
      baudRate = CBR_2400;
      break;
    case 4800:
      baudRate = CBR_4800;
      break;
    case 9600:
      baudRate = CBR_9600;
      break;
    case 14400:
      baudRate = CBR_14400;
      break;
    case 19200:
      baudRate = CBR_19200;
      break;
    case 38400:
      baudRate = CBR_38400;
      break;
    case 57600:
      baudRate = CBR_57600;
      break;
    case 115200:
      baudRate = CBR_115200;
      break;
    case 128000:
      baudRate = CBR_128000;
      break;
    case 256000:
      baudRate = CBR_256000;
      break;
    default:
      ar_log_error() << "arRS232Port: baud rate must be one of the following:\n"
        << "\t110\n\t300\n\t600\n\t1200\n\t2400\n\t4800\n\t9600\n"
        << "\t14400\n\t19200\n\t38400\n\t57600\n\t115200\n\t128000\n\t256000\n";
      return false;
  }
  if (dBits < 4 || dBits > 8) {
    ar_log_error() << "arRS232Port: data bits must be one of 4,5,6,7,8.\n";
    return false;
  }
  const BYTE byteSize = static_cast<BYTE>( dBits );
  BYTE stopBits = 0;
  if (fltcomp( stBits, 1 ))
    stopBits = 0;
  else if (fltcomp( stBits, 1.5 ))
    stopBits = 1;
  else if (fltcomp( stBits, 2 ))
    stopBits = 2;
  else {
    ar_log_error() << "arRS232Port: stop bits must be one of 1, 1.5, 2.\n";
    return false;
  }
  BYTE parity = 0;
  if (par == "none")
    parity = 0;
  else if (par == "odd")
    parity = 1;
  else if (par == "even")
    parity = 2;
  else if (par == "mark")
    parity = 3;
  else if (par == "space")
    parity = 4;
  else {
    ar_log_error() << "arRS232Port: parity must be one of none, odd, even, mark, space.\n";
    return false;
  }

  char portString[8] = "COM";
  // Windows port #s are 1-based..
  sprintf( portString+3, "%d", port );

  _portHandle = CreateFile( portString, GENERIC_READ | GENERIC_WRITE,
                            0, // exclusive access
                            0, // no security attributes.
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

  if (_portHandle == INVALID_HANDLE_VALUE) {
    ar_log_error() << "arRS232Port failed to open " << portString << ".\n";
    return false;
  }  
  ar_log_remark() << "arRS232Port opened " << portString << ".\n";
  
  DCB dcb;
  GetCommState( _portHandle, &dcb );

  // These two are needed by the Flock of Birds, should probably be optional. 
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fRtsControl = RTS_CONTROL_DISABLE;

  dcb.BaudRate = baudRate;
  dcb.ByteSize = byteSize;
  dcb.Parity = parity;
  dcb.StopBits = stopBits;

#if 0
  cerr << "DCB Fields:\n";
  cerr << "fParity: " << dcb.fParity << endl;
  cerr << "fOutxCtsFlow: " << dcb.fOutxCtsFlow << endl;
  cerr << "fOutxDsrFlow: " << dcb.fOutxDsrFlow << endl;
  cerr << "fDtrControl: " << dcb.fDtrControl << endl;
  cerr << "fOutX: " << dcb.fOutX << endl;
  cerr << "fInX: " << dcb.fInX << endl;
  cerr << "fErrorChar: " << dcb.fErrorChar << endl;
  cerr << "fRtsControl: " << dcb.fRtsControl << endl;
#endif

  if (SetCommState( _portHandle, &dcb ) != TRUE) {
    ar_log_error() << "arRS232Port failed to set communication parameters.\n";
    return false;
  }
#if 0
  else
    ar_log_remark() << "arRS232Port baud rate is " << baudRate << endl;
#endif

#if 0
  COMMPROP comProp;
  if (GetCommProperties( _portHandle, &comProp ) != TRUE) {
    ar_log_error() << "arRS232Port failed to get com-port properties.\n";
    return false;
  }
  ar_log_debug() << "System input buffer size = " << comProp.dwCurrentRxQueue << ", max " << comProp.dwMaxRxQueue << ar_endl;
#endif

  _isOpen = true;
  if (!setReadTimeout( _readTimeoutTenths )) {
    return false;
  } 

  if (!flushInput())
    ar_log_error() << "arRS232Port: flushInput() failed.\n";
  if (!flushOutput())
    ar_log_error() << "arRS232Port: flushOutput() failed.\n";
  return true;
#endif

#ifdef AR_USE_LINUX
  tcflag_t baudRate;
  switch (baud) {
    case 9600:
      baudRate = B9600;
      break;
    case 19200:
      baudRate = B19200;
      break;
    case 38400:
      baudRate = B38400;
      break;
    case 57600:
      baudRate = B57600;
      break;
    case 115200:
      baudRate = B115200;
      break;
    default:
      ar_log_error() << "arRS232Port: baud rate must be one of 9600, 19200, 38400, 57600, 115200.\n";
      return false;
  }
  char portString[64];
  sprintf( portString, "/dev/ttyS%d", port-1 ); // port numbers are 0-based
  
  // Open the port.
  _fileDescriptor = open( portString, O_RDWR | O_NOCTTY );
  if (_fileDescriptor < 0) {
    ar_log_error() << "arRS232Port failed to open " << portString << ".\n"
         << "     (Did you give non-root users write permission?)\n";
    return false;
  }
  ar_log_remark() << "arRS232Port opened '" << portString << "'.\n";
  
  if (tcgetattr( _fileDescriptor, &_oldConfig ) < 0) // save port settings
    perror("arRS232Port error");
  memcpy( &_newConfig, &_oldConfig, sizeof(_newConfig) );

  // Set baud rate.
  cfsetispeed( &_newConfig, baudRate );
  cfsetospeed( &_newConfig, baudRate );
  
  _newConfig.c_cflag &= ~CSIZE;
  switch (dBits) {
    case 5:
      _newConfig.c_cflag |= CS5;
      break;
    case 6:
      _newConfig.c_cflag |= CS6;
      break;
    case 7:
      _newConfig.c_cflag |= CS7;
      break;
    case 8:
      _newConfig.c_cflag |= CS8;
      break;
    default:
      ar_log_error() << "arRS232Port: data bits must be one of 5,6,7,8.\n";
      return false;
  }
  if (fltcomp( stBits, 1 )) {
    _newConfig.c_cflag &= ~CSTOPB;
  }
  else if (fltcomp( stBits, 2 ))
    _newConfig.c_cflag |= CSTOPB;
  else {
    ar_log_error() << "arRS232Port: stop bits must be one of 1 or 2.\n";
    return false;
  }
  if ((par == "none")||(par == "space")) {
    _newConfig.c_cflag &= ~PARENB;
  }
  else if (par == "odd")
    _newConfig.c_cflag |= PARENB | PARODD;
  else if (par == "even") {
    _newConfig.c_cflag |= PARENB;
    _newConfig.c_cflag &= ~PARODD;
  }
  else {
    ar_log_error() << "arRS232Port: parity must be one of none, odd, even, or space.\n";
    return false;
  }

  // Set flags.
  
  _newConfig.c_cflag |= CLOCAL | CREAD;
  // _newConfig.c_cflag |= CRTSCTS;
#if 0
  _newConfig.c_cflag |= HUPCL;
#endif
  
  // change old flag: _newConfig.c_iflag |= (INPCK | ISTRIP); to the flag
  // below to make the FOB work (see SERKNR.C from the ascension driver or
  // fob.cpp from libfob)
  _newConfig.c_iflag = IXOFF;
  _newConfig.c_oflag &= ~OPOST;
  
  // set input mode (non-canonical, no echo, ...)
  // _newConfig.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
  _newConfig.c_lflag = 0;

  // These settings control timeout behavior, as follows:
  // (1) If VMIN = 0, then a read blocks until either one or more characters arrive
  // or VTIME elapses. Then it returns all available characters,
  // up to the number requested.
  // (2) If VMIN > 0, then it waits VTIME/10 seconds for the first character. If no
  // character appears by then, the read returns 0. If a character appears, the read
  // blocks indefinitely until VMIN characters arrive.
  // (3) In either case, if VTIME = 0, it blocks indefinitely until the appropriate
  // number of characters shows up.
  //
  // For non-blocking reads, either open the port with O_NDELAY or
  // call fcntl( _fileDescriptor, F_SETFL, FNDELAY ).
  //
  _newConfig.c_cc[VTIME]    = 1;   // inter-character timer
  _newConfig.c_cc[VMIN]     = 0;   // blocking read until XX characters received
  _isOpen = true;
  if (!flushInput())
    ar_log_error() << "arRS232Port: flushInput() failed.\n";
  if (!flushOutput())
    ar_log_error() << "arRS232Port: flushOutput() failed.\n";
    
  // Reconfigure port.
  if (tcsetattr( _fileDescriptor, TCSANOW, &_newConfig ) < 0)
    perror("arRS232Port error");
  
  // Set some flags needed by FOB, will likely break other stuff.
  int status = 0;
  ioctl( _fileDescriptor, TIOCMGET, &status);
  status |= TIOCM_DTR;
  status &= ~TIOCM_RTS;
  ioctl( _fileDescriptor, TIOCMSET, &status );
#if 0
  // Another way of doing the previous 5 lines.  Identical in Nov 2005.
  int f = TIOCM_DTR;
  ioctl(_fileDescriptor, TIOCMBIS, &f);
  f = TIOCM_RTS;
  ioctl(_fileDescriptor, TIOCMBIC, &f);
#endif

#undef PARANOID
#ifdef PARANOID
  // test it again.
  status = 0xffffffff;
  ioctl(_fileDescriptor, TIOCMGET, &status);
  if ((status & TIOCM_LE))
    ar_log_error() << "arRS232Port: argh LE-DSR alarm.\n";
  if ((status & TIOCM_DSR))
    ar_log_error() << "arRS232Port: argh DSR.\n";
  if ((status & TIOCM_CTS))
    ar_log_error() << "arRS232Port: argh CTS.\n";
  if (!(status & TIOCM_DTR))
    ar_log_error() << "arRS232Port failed to set DTR.\n";
  if (status & TIOCM_RTS)
    ar_log_error() << "arRS232Port failed to clear RTS.\n";

  {
  struct termios tmp;
  tcgetattr(_fileDescriptor, &tmp);
  if (cfgetispeed(&tmp)!=B115200 || cfgetospeed(&tmp)!=B115200)
    ar_log_error() << "arRS232Port FoB: actual baud rates aren't 115200.\n";
  }
  ar_log_debug() << "\tFoB works in winXP: parity 0, cts 0, dsr 0, dtr 1, rts 0, baud 115200, buffer 4k.\n";
#endif

  return setReadTimeout( _readTimeoutTenths );
#endif
}

bool arRS232Port::ar_close() {
  if (!_isOpen)
    return true;
  
  if (!flushInput())
    ar_log_error() << "arRS232Port: flushInput() failed.\n";
  if (!flushOutput())
    ar_log_error() << "arRS232Port: flushOutput() failed.\n";
  _isOpen = false;

#ifdef AR_USE_WIN_32
  return CloseHandle( _portHandle ) == TRUE;
#endif

#ifdef AR_USE_LINUX
  // restore port's state
  if (tcsetattr( _fileDescriptor, TCSANOW, &_oldConfig ) < 0)
    perror("arRS232Port error");
  return close(_fileDescriptor) == 0;
#endif

  return true;
}

int arRS232Port::ar_read(char* buf, const unsigned numBytes, const unsigned maxBytes) {
  if (!_isOpen) {
    ar_log_error() << "arRS232Port can't read from a closed port.\n";
    return -1;
  }
#ifdef AR_USE_WIN_32
  DWORD bytesThisTime = 0; // unsigned
  unsigned numBytesAvailable = getNumberBytes();
  unsigned numToRead = numBytes;
  if ((numBytesAvailable <= maxBytes) && (numBytesAvailable > numBytes))
    numToRead = numBytesAvailable; 

  // Do one blocking read with a total timeout.
  if (!ReadFile(_portHandle, buf, numToRead, &bytesThisTime, NULL)) {
    ar_log_error() << "ar_read() failed:\n  "
                   << ar_getLastWin32ErrorString() << ar_endl;
    return -1;
  }

#if 0
  static int myLimit = 20;
  if (myLimit-- > 0) {
    cout << "\t\t\tRD: ";
    for (int i=0; i<numBytes; i++)
      cout << hex << (unsigned)buf[i] << " ";
    cout << dec << "\n\n";
  }
#endif

  return int(bytesThisTime);
#endif

#ifdef AR_USE_LINUX
  struct timeval tStruct;
  struct timezone zStruct;
  if (gettimeofday(&tStruct, &zStruct)) {
    ar_log_error() << "arRS232Port: gettimeofday failed.\n";
    return -1;
  }

  const unsigned numBytesAvailable = getNumberBytes();
  unsigned numToRead = numBytes;
  int numRead = 0;

  if (_readTimeoutTenths > 0) {
    // Blocking read.  Wait for at least numBytes bytes,
    // but take more if they're immediately available (up to maxBytes).
    // This simulates one blocking read with a total timeout.

    const long startTimeSecs = tStruct.tv_sec;
    const long startTimeMicSecs = tStruct.tv_usec;
    if (numBytesAvailable <= maxBytes && numBytesAvailable > numBytes)
      numToRead = numBytesAvailable; 
    unsigned long timeElapsedTenths = 0;

    while (static_cast<unsigned>(numRead) < numToRead &&
           timeElapsedTenths < _readTimeoutTenths ) {
      const int stat = read( _fileDescriptor, buf+numRead, numToRead-numRead );
      // Note: if we decide we want to get at actual error codes, we'll need to
      // figure out how to do it in a Win32-compatible way.
      if (stat < 0) {
        ar_log_error() << "arRS232Port: read() failed.\n";
        return -1;
      }
      numRead += stat;
      if (static_cast<unsigned>(numRead) < numToRead) {
        if (gettimeofday( &tStruct, &zStruct )!=0) {
          ar_log_error() << "arRS232Port: gettimeofday failed during read.\n";
          return -1;
        }      
        timeElapsedTenths = static_cast<unsigned long>( static_cast<unsigned long>(
	  10 * (tStruct.tv_sec-startTimeSecs) +
	  static_cast<long>(floor((tStruct.tv_usec-startTimeMicSecs)/1.0e5)) ) );
      }
    } 

#if 0
  static int myLimit = 20;
  if (myLimit-- > 0) {
    cout << "\t\t\tRD: ";
    for (int i=0; i<numRead; i++)
      cout << hex << (unsigned)buf[i] << " ";
    cout << dec << "\n\n";
  }
#endif

  }
  else {
    // Return immediately with whatever's there, since timeout==0.

    if (numBytesAvailable == 0)
      return 0;
    if (numBytesAvailable < numToRead)
      numToRead = numBytesAvailable; 
    numRead = read( _fileDescriptor, buf, numToRead );
    if (numRead < 0) {
      ar_log_error() << "arRS232Port: read() failed.\n";
      return -1;
    }
  }
  return numRead;
#endif

  ar_log_error() << "arRS232Port: only Win32 and Linux implemented.\n";
  return -1;
}

int arRS232Port::readAll( char**bufAdd, unsigned& currentBufferSize ) {
  if (!_isOpen) {
    ar_log_error() << "arRS232Port: can't read from a closed port.\n";
    return -1;
  }
  if (!bufAdd) {
    ar_log_error() << "arRS232Port: readAll got NULL buffer.\n";
    return false;
  }
  const unsigned numBytes = getNumberBytes();
  if (numBytes == 0)
    return 0;
  if (numBytes > currentBufferSize && *bufAdd) {
    delete[] *bufAdd;
    *bufAdd = 0;
    currentBufferSize = 0;
  }
  if (!*bufAdd) {
    // Not null-terminated!
    *bufAdd = new char[numBytes];
    if (!*bufAdd) {
      ar_log_error() << "arRS232Port: readAll() out of memory.\n";
      return 0; 
    }
    currentBufferSize = numBytes;
  }
  return ar_read( *bufAdd, numBytes );
}

int arRS232Port::ar_write( const char* buf, const unsigned numBytes ) {
  if (!_isOpen) {
    ar_log_error() << "arRS232Port: can't write to a closed port.\n";
    return -1;
  }

#if 0
  static int myLimit = 6;
  if (myLimit-- > 0) {
    cout << "\t\t\t\tWR: ";
    for (int i=0; i<numBytes; i++)
      cout << hex << (int)buf[i] << " ";
    cout << dec << "\n";
  }
#endif

#ifdef AR_USE_WIN_32
  DWORD numWrit = 0; // unsigned
  if (!WriteFile( _portHandle, buf, numBytes, &numWrit, NULL )) {
    ar_log_error() << "arRS232Port: WriteFile() failed.\n";
    return -1;
  }
  if (numWrit != numBytes)
    ar_log_error() << "arRS232Port: only " << numWrit << " bytes written.\n";

  return static_cast<int>(numWrit);
#endif

#ifdef AR_USE_LINUX
  int numWrit = write( _fileDescriptor, buf, numBytes );
  if (numWrit < 0) {
    ar_log_error() << "arRS232Port: write() failed.\n";
    return -1;
  } else if (static_cast<unsigned>(numWrit) != numBytes)
    ar_log_error() << "arRS232Port: only " << numWrit << " bytes written.\n";
  return numWrit;
#endif

  nyi();
  return -1;
}

int arRS232Port::ar_write( const char* buf ) {
  return ar_write( buf, strlen(buf) );
}

bool arRS232Port::flushInput() {
#if !defined( AR_USE_WIN_32 ) && !defined( AR_USE_LINUX )
  nyi();
  return false;
#endif 

  if (!_isOpen) {
    ar_log_error() << "arRS232Port: can't flush a closed port.\n";
    return false;
  }
#ifdef AR_USE_WIN_32
  return PurgeComm( _portHandle, PURGE_RXCLEAR ) == TRUE;
#endif
#ifdef AR_USE_LINUX
  return tcflush( _fileDescriptor, TCIFLUSH ) == 0;
#endif
}

bool arRS232Port::flushOutput() {
#if !defined( AR_USE_WIN_32 ) && !defined( AR_USE_LINUX )
  nyi();
  return false;
#endif 

  if (!_isOpen) {
    ar_log_error() << "arRS232Port: can't flush a closed port.\n";
    return false;
  }
#ifdef AR_USE_WIN_32
  return PurgeComm( _portHandle, PURGE_TXCLEAR ) == TRUE;
#endif
#ifdef AR_USE_LINUX
  return tcflush( _fileDescriptor, TCOFLUSH ) == 0;
#endif
}

bool arRS232Port::setReadTimeout( const unsigned timeoutTenths ) {
#if !defined( AR_USE_WIN_32 ) && !defined( AR_USE_LINUX )
  nyi();
  return false;
#endif 
  if (!_isOpen) {
    ar_log_error() << "arRS232Port: port closed\n";
    return false;
  }
  bool ok = true;
#ifdef AR_USE_WIN_32
  if (timeoutTenths != _readTimeoutTenths) {
    _timeoutStruct.ReadTotalTimeoutConstant = 100*timeoutTenths;
    ok = SetCommTimeouts( _portHandle, &_timeoutStruct );
  }
#endif
  _readTimeoutTenths = timeoutTenths;
  return ok;
}

bool arRS232Port::setReadBufferSize( const unsigned numBytes ) {
#ifdef AR_USE_WIN_32
  if (!_isOpen) {
    ar_log_error() << "arRS232Port: port closed\n";
    return false;
  }
  COMMPROP comProp;
  if (GetCommProperties( _portHandle, &comProp ) != TRUE) {
    ar_log_error() << "arRS232Port: failed to get com-port properties in setReadBufferSize().\n";
    return false;
  }
  return SetupComm( _portHandle, static_cast<DWORD>(numBytes), comProp.dwCurrentTxQueue );
#else
  (void)numBytes; // avoid compiler warning
  return true;
#endif
}

unsigned arRS232Port::getNumberBytes() {
#ifdef AR_USE_WIN_32
  DWORD errorFlags = 0;
  COMSTAT comState;
  ClearCommError( _portHandle, &errorFlags, &comState ) ;
  return static_cast<unsigned>(comState.cbInQue);
#endif
#ifdef AR_USE_LINUX
  int numBytes = 0;
  ioctl( _fileDescriptor, FIONREAD, &numBytes );
  return static_cast<unsigned>( numBytes );
#endif
  nyi();
  return 0;
}
