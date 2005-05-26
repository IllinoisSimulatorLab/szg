//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// Driver for Precision Navigation Inc.'s TCM2-50 compass+tiltsensor module.
#include "RS232Server.h"

static float pitchExport = 0.; // in radians
static float rollExport = 0.; // in radians
static float aziExport = 0.; // in radians

#ifdef AR_USE_WIN_32

static void ResetHead(HANDLE hCommPort, bool fVerbose)
{
  cout << "Reinitializing head sensor.\n";
  // Start continuous reporting of data.
  char* foo = "go\n";
  DWORD cbWritten = -1;
  int iStatus = WriteFile(hCommPort, foo, 3, &cbWritten, NULL);
  if (!iStatus)
    {
    if (fVerbose)
	    cerr << "WriteFile failed" << GetLastError() << endl;
    return;
    }

  // Sleep(20);

  // we could send more commands here too, to be fancy.
}

static void ReadHead(HANDLE hCommPort)
{
  static char buf[500];

  {
  DWORD dwCount = 0;
  char str[60];
  int rc = ReadFile((HANDLE)hCommPort, str, 50, &dwCount, NULL);
  //if (!rc)
  //  MessageBox(NULL, "ReadFile failed!", "foo", MB_OK);
  if (dwCount <= 0)
    return;
  strncat(buf, str, dwCount);
  //cout << "TCM2 sent bytes: " << dwCount << endl;
  }

  static int iTimeout = 0;
  {
  char* pch = NULL;
  char* pch2 = NULL;
  char sz[60];
  if (++iTimeout > 50)
    goto LTimeout;
  pch = strstr(buf, "$C");
  if (!pch || !*buf)
    return; // not yet a complete message
  iTimeout = 0;
  pch2 = strchr(pch+=2, '*'); // beginning of checksum, which we ignore
  if (!pch2 || pch2-pch>59)
    return; // not yet a complete message
  //cout << "\t\"" << buf << "\"  received!\n";
  strncpy(sz, pch, pch2-pch);
  sz[pch2-pch] = '\0';
  // String sz has the form "$C93.3P-4.2R12.8*7E\n", with 7E possibly truncated.

  pch = strstr(sz, "E002");
  if (pch)
    // Truncate this error message which comes after the R12.8,
    // so it doesn't get parsed as an exponent by atof().
    *pch = '\0';

  float azi = float(atof(sz));
  pch = strchr(sz, 'P') + 1;
  float pitch = -float(atof(pch)) * 3.1415927/180.;
  pch = strchr(pch, 'R') + 1;
  float roll = -float(atof(pch)) * 3.1415927/180.;

  azi -= 90.; // convert 0-is-north to 0-is-positiveXaxis-is-east
  if (azi < 0.)
    azi += 360.;
  azi *= 3.1415927/180.;

  // moving-average filters to reduce back-and-forth jitter

  static float aziExportPrev = -2000.;
  if (aziExportPrev < -1000.)
    aziExport = aziExportPrev = azi;
  else
    {
    // Smoothing by moving average,
    // but don't average across the 0--360 singularity!
    aziExport = fabs(aziExportPrev - azi) > 90.*3.1415927/180 ?
	    azi : aziExport = azi*.4f + aziExportPrev*.6f;
    aziExportPrev = azi;
    }

  static float pitchExportPrev = -2000.;
  if (pitchExportPrev < -1000.)
    pitchExport = pitchExportPrev = pitch;
  else
    {
    pitchExport = pitch*.3f + pitchExportPrev*.7f;
    pitchExportPrev = pitch;
    }

  static float rollExportPrev = -2000.;
  if (rollExportPrev < -1000.)
    rollExport = rollExportPrev = roll;
  else
    {
    // Smoothing by moving average,
    // but don't average across the 0--360 singularity!
    // (Not likely: you'd need a 360-degree roll sensor,
    // and have your head upside down.)
    rollExport = fabs(rollExportPrev - roll) > 90.*3.1415927/180 ?
	    roll : rollExport = roll*.3f + rollExportPrev*.7f;
    rollExportPrev = roll;
    }

  // No smoothing on these guys, though we may need it eventually.
  pitchExport = pitch;
  rollExport = roll;

#if 0
  cout <<   "TCM2   azi " << int(aziExport *  180./3.1415927)
       << "\n     pitch " << int(pitchExport * 180./3.1415927)
       << "\n      roll " << int(rollExport * 180./3.1415927)
       << endl;
#endif

  *buf = '\0';
  return;
  }

LTimeout:
  iTimeout = 0;
  aziExport = 0.;
  pitchExport = 0.;
  rollExport = 0.;
  cerr << "HeadServer warning: TCM2-50 module timed out.\n";
  *buf = '\0';
}

#endif // AR_USE_WIN_32

void rs232Task(void* pv){
#ifdef AR_USE_WIN_32
  ResetHead(HANDLE(pv), true);
  while (true) {
    ar_usleep(25000);
    ReadHead(HANDLE(pv));
  }
#endif
}

static void simTask(void*){
  static float vAzi = 0.;
  static float vPitch = 0.;
  static float vRoll = 0.;
  while (true) {

    vAzi += .04 * (float(rand())/RAND_MAX-.5);
    if (vAzi < -.2)
      vAzi = -.2;
    if (vAzi > .2)
      vAzi = .2;
    aziExport += vAzi;
    if (aziExport > 6.2)
      { aziExport = 6.2; vAzi = 0.; }
    if (aziExport < .01)
      { aziExport = .01; vAzi = 0.; }

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
  return ar_rotationMatrix('y', aziExport) *
         ar_rotationMatrix('z', pitchExport) *
         ar_rotationMatrix('x', rollExport);
}

static const float* exportAxes()
{
  static float val[3];
  val[0] = aziExport;
  val[1] = pitchExport;
  val[2] = rollExport;
  return val;
}

int main(int argc, char** argv)
{
  const RS232Port rs232 = {"COM5", 20, true, 9600};
  return ar_RS232main(argc, argv, "HeadServer", "head", "SZG_HEAD",
    25000, exportMatrix, simTask, rs232Task, rs232,
    3, exportAxes);
}
