//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#include "RS232Server.h"

static float aziExport = 0.; // in radians

#ifdef AR_USE_WIN_32

void ReadCompass(HANDLE hCommPort)
{
  static char buf[10000];

  {
  DWORD dwCount = 0;
  char str[110];
  int rc = ReadFile((HANDLE)hCommPort, str, 100, &dwCount, NULL);
  //if (!rc)
  //  MessageBox(NULL, "ReadFile failed!", "foo", MB_OK);
  if (dwCount <= 0)
    return;
  strncat(buf, str, dwCount);
  //cout << "Azi sent bytes: " << dwCount << endl;
  }

  static int iTimeout = 0;
  char* pch = NULL;
  char* pchComma = NULL;
  char sz[30];
  if (++iTimeout > 50)
    goto LTimeout;
  pch = strstr(buf, "HCHDM,");
  if (!pch || !*buf)
    return;
  iTimeout = 0;
  pch += 6;
  pchComma = strchr(pch, ',');
  if (!pchComma || pchComma-pch>29)
    return;
  strncpy(sz, pch, pchComma-pch);
  sz[pchComma-pch] = '\0';

  {
  // moving-average filter to reduce back-and-forth jitter
  float azi = (float)atof(sz);
  if (azi == 800.)
    {
    cerr << "CompassServer error:  battery may be low.\n";
    aziExport = 0.;
    *buf = '\0';
    return;
    }

  // subtract 90 degrees, to account for compass's orientation in its box
  azi -= 90.;
  azi -= 90.; // do it a SECOND time, to convert 0-is-north to 0-is-positiveXaxis-is-east
  if (azi < 0.)
    azi += 360.;
  azi *= 3.1415927/180.;

  static float aziExportPrev = -1.;
  if (aziExportPrev < 0.)
    aziExport = aziExportPrev = azi;
  else
    {
    // moving average, but don't average across the 0--360 singularity!
    aziExport = fabs(aziExportPrev - azi) > 90.*3.1415927/180 ?
	    azi : aziExport = azi*.5f + aziExportPrev*.5f;
    aziExportPrev = azi;
    }
  *buf = '\0';
  }

LTimeout:
  iTimeout = 0;
  aziExport = 0.;
  cerr << "CompassServer warning: compass timed out.\n";
  *buf = '\0';
}

#endif // AR_USE_WIN_32

void rs232Task(void* pv){
#ifdef AR_USE_WIN_32
  while (true) {
    ar_usleep(50000);
    ReadCompass(HANDLE(pv));
  }
#endif
}

void simTask(void*){
  static float vAzi = 0.;
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

    ar_usleep(250000);
  }
}

static arMatrix4 exportMatrix()
{
  return ar_rotationMatrix('y', aziExport);
}

static const float* exportAxes()
{
  static float val = aziExport;
  return &val;
}

int main(int argc, char** argv)
{
  const RS232Port rs232 = {"COM5", 50, true, 4800};
  return ar_RS232main(argc, argv, "CompassServer", "compass", "SZG_COMPASS",
    25000, exportMatrix, simTask, rs232Task, rs232,
    1, exportAxes);
}
