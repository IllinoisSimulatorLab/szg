//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#include "RS232Server.h"

static float northExport = 900.;
static float westExport = 680.;

#ifdef AR_USE_WIN_32

static void ReadGPS(HANDLE hCommPort)
{
  static char buf[10000];

  {
  DWORD dwCount = 0;
  char str[400];
  int rc = ReadFile((HANDLE)hCommPort, str, 300, &dwCount, NULL);
  //if (!rc)
  //      MessageBox(NULL, "ReadFile failed!", "foo", MB_OK);
  if (dwCount <= 0)
    return;

  //cout << "GPS sent bytes: " << dwCount << endl;
  strncat(buf, str, dwCount);
  }

  static int iTimeout = 0;
  char sz[60];
  char* pch = NULL;
  char* pchComma = NULL;
  float raw = 0.;

  // 9 * 300 msec = 2.7 sec
  if (++iTimeout > 9)
	  goto LTimeout;
  pch = strstr(buf, "GPRTE,");
  if (!pch || !*buf)
    return;
  iTimeout = 0;

  pch = strstr(buf, "GPGLL,");
  if (!pch)
    return;
  pch += 6;
  //cout << "rawgps: " << pch << endl;
  pchComma = strchr(pch, ',');
  if (!pchComma || pchComma-pch>59)
    return;
  strncpy(sz, pch, pchComma-pch);
  sz[pchComma-pch] = '\0';
  raw = atof(sz); // e.g., "4006.9230"
  if (raw == 0.)
    goto LTimeout;
  northExport = (float)((raw - 4006.000) * 1000.);
  pch = pchComma + 3; // past "N,"
  pchComma = strchr(pch, ',');
//      // extra field?! from etrex
//      pch = pchComma + 2;
//      pchComma = strchr(pch, ','); // past "W,"
  strncpy(sz, pch, pchComma-pch);
  sz[pchComma-pch] = '\0';
  raw = atof(sz); // e.g., "8813.6270"
  if (raw == 0.)
    goto LTimeout;
  westExport = (float)((raw - 8813.000) * 1000.);
  //cout << "NW!   "; //cout << "\t\tN+W is " << northExport << "   " << westExport << endl;
  *buf = '\0';
  return;

LTimeout:
  northExport = 900.;
  westExport = 680.;
  *buf = '\0';
}
#endif

void rs232Task(void* pv){
#ifdef AR_USE_WIN_32
  while (true) {
    ar_usleep(400000); // worked with 1400000 and 700000
    ReadGPS(HANDLE(pv));
  }
#endif
}

void simTask(void*){
  static float vNorth = 0.;
  static float vWest = 0.;
  while (true) {
  
    const float latMax = 940.;
    const float latMin = 850.;
    const float lonMax = 730.;
    const float lonMin = 610.;

    vNorth += .3 * (float(rand())/RAND_MAX-.5);
    if (vNorth < -3.)
      vNorth = -3.;
    if (vNorth > 3.)
      vNorth = 3.;
    northExport += 4*vNorth;

    vWest += .3 * (float(rand())/RAND_MAX-.5);
    if (vWest < -3.)
      vWest = -3.;
    if (vWest > 3.)
      vWest = 3.;
    westExport += 4*vWest;

    if (northExport > latMax)
      { northExport = latMax;  vNorth = -2.; }
    if (northExport < latMin)
      { northExport = latMin;  vNorth = 2.; }
    if (westExport > lonMax)
      { westExport = lonMax;  vWest = -2.; }
    if (westExport < lonMin)
      { westExport = lonMin;  vWest = 2.; }

    // cerr << "\tsimTask NW: " << northExport << "  " << westExport << endl;
    ar_usleep(1000000);
  }
}

static arMatrix4 exportMatrix()
{
  // cout << "GPS reports " << int(northExport) << ", " << int(westExport) << endl;;;;
  // cout << "GPS reports \n" << ar_translationMatrix(westExport, 4., northExport) << endl;;;;
  return ar_translationMatrix(westExport, 4., northExport);
}

int main(int argc, char** argv){
  const RS232Port rs232 = {"COM1", 50, false, 4800};
  return ar_RS232main(argc, argv, "GPSServer", "gps", "SZG_GPS",
    50000, exportMatrix, simTask, rs232Task, rs232);
}
