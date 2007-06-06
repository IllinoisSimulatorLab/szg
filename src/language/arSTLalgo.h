#ifndef ARSTLALGO_H
#define ARSTLALGO_H

#if (defined(__GNUC__)&&(__GNUC__<3))
  #include <algo.h>
#else
  #include <algorithm>
#endif
#if (defined(__GNUC__)&&(__GNUC__>=4))
  // For gcc 4.1.2 "accumulator"
  #include <numeric>
#endif

#endif
