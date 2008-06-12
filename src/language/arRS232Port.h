//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_RS232_PORT_H
#define AR_RS232_PORT_H

#ifdef AR_USE_WIN_32
#include <wtypes.h>
#endif
#ifdef AR_USE_LINUX
#include <termios.h>
#endif
#include <string>
using namespace std;

#include "arLanguageCalling.h"

// An RS-232 port.

class SZG_CALL arRS232Port {
  public:
    arRS232Port();
    virtual ~arRS232Port();

    // Port numbers are 1-based on all platforms.
    // Legal values for other parameters vary across platforms, see the
    // .cpp file
    bool ar_open( const unsigned int portNumber,
                  const unsigned long baudRate,
                  const unsigned int dataBits, const float stopBits,
                  const std::string& parity );
    bool ar_close();

    // These three return a signed int that is the number of bytes
    // transferred >= 0, while -1 indicates a system error.
    //
    // The _optional_ maxBytes argument to ar_read() indicates the maximum
    // number of bytes to read in a blocking read (i.e. timeout > 0).
    // The behavior is as follows: If fewer than bytesToRead characters are
    // available in the system serial buffer when the read is called,
    // the routine will block until either the timeout interval passes or it
    // receives bytesToRead characters, at which point it returns. If more
    // than bytesToRead characters are available when the read is called and
    // maxBytes > bytesToRead, then all available characters up to maxBytes
    // will be returned immediately. This option is provided to help on the
    // off-chance that repeatedly doing a blocking read for the
    // minimum amount of data does not allow you to keep up with streaming
    // input. You specify the minimum amount you require and the maximum you
    // can handle.

    int ar_read( char* buf, const unsigned int bytesToRead,
                 const unsigned int maxBytes=0 );

    // Read all characters available, growing buffer if necessary.
    // Returns number of bytes, puts current buffer size in currentBufferSize.
    // IMPORTANT: pointer pointed to by bufAdd must either (1) be 0,
    // in which case it will be new[]ed); (2) have been allocated using
    // new[]; or (3) have been allocated by a previous call to
    // readAll (really same as 2). You are responsible for delete[]ing
    // it when you're done. If there are no bytes waiting to
    // be read, call returns immediately without doing anything
    // (so if you've passed the address of a NULL pointer, it will still be
    // NULL). Caller must remove the contents of
    // the buffer between readAll()s, as they will be discarded if the buffer
    // has to be grown.
    int readAll( char**bufAdd, unsigned int& currentBufferSize );

    int ar_write( const char* buf, const unsigned int bytesToRead );
    int ar_write( const char* buf );

    bool flushInput();
    bool flushOutput();

    // Set read timeout in tenths of a second. Write timeout is currently
    // fixed at 1/10 sec for Windows. Posix does not implement write timeouts.
    bool setReadTimeout( const unsigned int timeoutTenths );
    // Set the system input buffer size (Win32 only, no effect under Posix).
    // Size defaults to 4096 on my Win2k system...
    bool setReadBufferSize( const unsigned int numBytes );

    // Get the number of bytes available to be read (yahoo!).
    unsigned int getNumberBytes();

  private:
    bool _isOpen;
    unsigned int _readTimeoutTenths;
#ifdef AR_USE_WIN_32
    HANDLE _portHandle;
    COMMTIMEOUTS _timeoutStruct;
#endif
#ifdef AR_USE_LINUX
    int _fileDescriptor;
    struct termios _oldConfig, _newConfig;
#endif
};

#endif
