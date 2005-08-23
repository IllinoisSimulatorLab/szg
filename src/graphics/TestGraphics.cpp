//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
// MUST come before other szg includes. See arCallingConventions.h for details.
#define SZG_DO_NOT_EXPORT
#include "arGraphicsContext.cpp"

int main(int, char**){
  ar_timeval time1 = ar_time();
  int i;
  int num = 100000;
  for (i=0; i<num; i++){
    arGraphicsContext* g = new arGraphicsContext();
    delete g;
  }
  double t = ar_difftime(ar_time(), time1);
  cout << "Average time to create/delete graphics context = "
       << t/num << " microseconds.\n";
}
