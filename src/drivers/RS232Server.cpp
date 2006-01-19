//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "RS232Server.h"
#include "InputServer.h"

#ifdef AR_USE_WIN_32
static HANDLE hCommPort = INVALID_HANDLE_VALUE;
#endif

static void cleanup()
{
#ifdef AR_USE_WIN_32
  if (hCommPort != INVALID_HANDLE_VALUE)
    CloseHandle(hCommPort);
#endif
}

static void messageTask(void* pClient){
  arSZGClient* cli = (arSZGClient*)pClient;
  string messageType, messageBody;
  while (true) {
    cli->receiveMessage(&messageType, &messageBody);
    if (messageType=="quit"){
      cleanup();
      ar_usleep(100000); // let other threads end gracefully
      exit(0);
    }
  }
}

int ar_RS232main(int argc, char** argv, const char* label, const char* /*dictName*/,
  const char* /*fieldName*/, int usecSleep, arMatrix4 getMatrix(void),
  void simTask(void*),
#ifdef AR_USE_WIN_32
  void rs232Task(void*), const RS232Port& rs232,
#else
  void (void*), const RS232Port&,
  // unused args
#endif
  int numAxes,
  const float* getAxes(void)
  )
{
  if (argc>1 && !strcmp(argv[1], "--help"))
    {
    cerr << "usage: " << argv[0] << " [-sim]\n";
    return -1;
    }
  arSZGClient szgClient(1);
  if (!szgClient)
    return -1;

  const int fSim = argc>1 && !strcmp(argv[1], "-sim"); // simulate a real one

  // init the data server object
  // This isn't quite right!!!!!
  arInputServer inputServer;
  inputServer.setSignature(0,3,1);
  if (!inputServer.configure(&szgClient, label) || !inputServer.start())
    return 1;

  arThread dummy1(messageTask, &szgClient);

  if (!fSim)
    {
#ifdef AR_USE_WIN_32
    hCommPort = CreateFile(rs232.port,
      rs232.rw ? GENERIC_READ|GENERIC_WRITE : GENERIC_READ,
      0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (hCommPort == INVALID_HANDLE_VALUE)
      {
      cerr << argv[0] << " error: CreateFile failed.  (Is a copy already running?)\n";
      return -1;
      }
    {
    COMMTIMEOUTS ctmoCommPort;
    ctmoCommPort.ReadIntervalTimeout = MAXDWORD;
    ctmoCommPort.ReadTotalTimeoutMultiplier = MAXDWORD;
    ctmoCommPort.ReadTotalTimeoutConstant = rs232.timeout;
    ctmoCommPort.WriteTotalTimeoutMultiplier = 0;
    ctmoCommPort.WriteTotalTimeoutConstant = 0;
    SetCommTimeouts(hCommPort, &ctmoCommPort);
    }
    {
    DCB dcbCommPort;
    dcbCommPort.DCBlength = sizeof(DCB);
    GetCommState(hCommPort, &dcbCommPort);
    dcbCommPort.BaudRate = rs232.baud;
    dcbCommPort.ByteSize = 8;
    dcbCommPort.Parity = NOPARITY;
    dcbCommPort.StopBits = ONESTOPBIT;
    dcbCommPort.fRtsControl = RTS_CONTROL_DISABLE;
    if (!SetCommState(hCommPort, &dcbCommPort))
      {
      cerr << argv[0] << "error: SetCommState failed\n";
      CloseHandle(hCommPort);
      hCommPort = INVALID_HANDLE_VALUE;
      return -1;
      }
    }
    arThread dummy2(rs232Task, hCommPort);
#else
    // This could be joined by Unix serial port code, eventually.
    cerr << argv[0] << " warning: " << label
         << " implemented only for Win32.\n";
    goto LSim;
#endif
    }
  else
    {
LSim:
    cerr << argv[0] << ": simulating a physical sensor.\n";
    arThread dummy3(simTask);
    }

  // Start the data flowing.
  arMatrix4 matrixHeadPrev(ar_identityMatrix());
  float* axesPrev = NULL;
  float* axes = NULL;
  if (numAxes > 0)
    {
    axesPrev = new float[numAxes];
    axes = new float[numAxes];
    // memory leak, not really -- the while-loop never terminates.
    }
  int usec = 0;
  const int usecRefresh = 500000;
  while (true) {
    arMatrix4 matrixHead((*getMatrix)());
    if (numAxes > 0)
      memcpy(axes, getAxes(), numAxes * sizeof(float));

    // Only send data if the data has changed!  Conserve network bandwidth.
    // "if (theData != theDataPrev)" would be more general, but slower.
    //
    // But limit the maximum pause to usecRefresh microseconds,
    // so late-joining clients don't wait a long time for this data.

    if (matrixHead != matrixHeadPrev || usec > usecRefresh)
      {
      inputServer.sendMatrix(matrixHead);
      matrixHeadPrev = matrixHead;
      }
    if ((numAxes > 0 && memcmp(axes, axesPrev, numAxes * sizeof(float))) ||
        usec > usecRefresh)
      {
      for (int i=0; i<numAxes; i++)
        inputServer.sendAxis(i, axes[i]);
      memcpy(axesPrev, axes, numAxes * sizeof(float));
      }
    if (usec > usecRefresh)
      usec = 0;
    ar_usleep(usecSleep);
    usec += usecSleep;
  }
  return 0;
}
