//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "RS232Server.h"

static float pitchExport = 0.; // in radians
static float rollExport = 0.; // in radians

#ifdef AR_USE_WIN_32

static void ResetTilt(HANDLE hCommPort, bool fVerbose)
{
  cout << "Reinitializing tilt sensor.\n";
  char* foo = "R";
  DWORD cbWritten = -1;
  int iStatus = WriteFile(hCommPort, foo, 1, &cbWritten, NULL);
  if (!iStatus)
    {
    if (fVerbose)
	    cerr << "WriteFile failed" << GetLastError() << endl;
    return;
    }

  Sleep(20);
  char bar[8] = {0};
  DWORD cbRead = -1;
  static bool fComplained = false;
  iStatus = ReadFile(hCommPort, bar, 4, &cbRead, NULL);  // SetCommTimeouts controls how this times out.
  if (!iStatus)
    {
    if (fVerbose && !fComplained)
      {
      fComplained = true;
      cerr << "tilt init, ReadFile failed" << GetLastError() << endl;
      }
    return;
    }
  //if (bar[0] != 'H' && fVerbose && !fComplained)
  //    MessageBox("urp, tiltsensor didn't respond to reset command.");
  if (bar[0] != 'H')
    {
    pitchExport = 0;
    rollExport = 0.0;
    return;
    }

  char* zop = "N\006"; // 3: .08 sec;  5: .32 sec;  6: .67 sec // was 003 just before EOH
  iStatus = WriteFile(hCommPort, zop, 2, &cbWritten, NULL);
  if (!iStatus)
    {
    if (fVerbose)
      cerr << "lowlat WriteFile failed" << GetLastError() << endl;
    return;
    }

  Sleep(20);
  char* zip = "G";   // pre-request an angle packet
  iStatus = WriteFile(hCommPort, zip, 1, &cbWritten, NULL);
  if (!iStatus)
    {
    if (fVerbose)
      cerr << "2nd WriteFile failed" << GetLastError() << endl;
    }
}

static void ReadTilt(HANDLE hCommPort)
{
  // get the packet we previously requested

  static char sz[500];
  DWORD cbRead = -1;
  int iStatus = ReadFile(hCommPort, sz, 498, &cbRead, NULL); // read more than the 6 bytes we need, to get back in sync if we got out of sync
  if (!iStatus)
    { 
    static bool fComplained = false;
    if (!fComplained)
      { 
      cerr << "tilt G-response ReadFile failed: " << GetLastError() << endl;
      fComplained = true;
      } 
    return; 
    }
  
  static int cError = 0;
  if (sz[0] != -1/*255*/ || cbRead < 6)
    { 
    //cerr << "tilt G-response 255 failed, or we read an incomplete packet";
    if (++cError > 30)
      { 
      cError = 0;
      // tilt sensor died (serial cable or power supply).  check if it came back.
      ResetTilt(hCommPort, false);
      } 
    return;
    } 
  // might be start of a packet
  const int a = int(sz[1]); 
  const int b = int(sz[2]); 
  const int c = int(sz[3]);
  const int d = int(sz[4]);
  const BYTE checksumReported = a+b+c+d;
  const unsigned char checksum = sz[5];
  if (checksum == checksumReported)
    { 
    // it was a packet
    const float pitchOffset = -20.0*3.1415927/180.0;
    // compensates for the sensor's nonhorizontal mounting on user's head

    pitchExport = -90.-90.* float(a*256+b - 32768) * float(1./32767.) + pitchOffset - 0.4;
    rollExport = 180-90.+90.* float(c*256+d - 32768) * float(1./32767.) + 0.15;
//cout << "QQQ pitch " << pitchExport << "     roll " << rollExport << endl;
    // The -0.4 and +0.15 are approximate corrections derived from this particular sensor's measured error. 
    pitchExport *= 3.1415927/180.;
    rollExport *= 3.1415927/180.;
    } 
  else
    cerr << "TiltServer error: G-response checksum failed\n";
          
  // request the next packet.
  char* zip = "G";
  unsigned long cbWritten = 0;
  iStatus = WriteFile(hCommPort, zip, 1, &cbWritten, NULL);
  if (!iStatus)
    {
    cerr << "continuing G failed: " << GetLastError() << endl;
    return;
    }
}

#endif // AR_USE_WIN_32

void rs232Task(void* pv){
#ifdef AR_USE_WIN_32
  ResetTilt(HANDLE(pv), true);
  while (true) {
    ar_usleep(200000);
    ReadTilt(HANDLE(pv));
  }
#endif
}

static void simTask(void*){
  static float vPitch = 0.;
  static float vRoll = 0.;
  while (true) {

    vPitch += .04 * (float(rand())/RAND_MAX-.5);
    if (vPitch < -.1)
      vPitch = -.1;
    if (vPitch > .1)
      vPitch = .1;
    pitchExport += vPitch;
    if (pitchExport > .4)
      { pitchExport = .4; vPitch = 0.; }
    if (pitchExport < -.4)
      { pitchExport = -.4; vPitch = 0.; }

    vRoll += .04 * (float(rand())/RAND_MAX-.5);
    if (vRoll < -.1)
      vRoll = -.1;
    if (vRoll > .1)
      vRoll = .1;
    rollExport += vRoll;
    if (rollExport > .6)
      { rollExport = .6; vRoll = 0.; }
    if (rollExport < -.6)
      { rollExport = -.6; vRoll = 0.; }

    ar_usleep(250000);
  }
}

static arMatrix4 exportMatrix()
{
  return ar_rotationMatrix('z', pitchExport) *
         ar_rotationMatrix('x', rollExport);
}

static const float* exportAxes()
{
  static float val[2];
  val[0] = pitchExport;
  val[1] = rollExport;
  return val;
}

int main(int argc, char** argv)
{
  const RS232Port rs232 = {"COM6", 20, true, 9600};
  return ar_RS232main(argc, argv, "TiltServer", "tilt", "SZG_TILT",
    25000, exportMatrix, simTask, rs232Task, rs232,
    2, exportAxes);
}
