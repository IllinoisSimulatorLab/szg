//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_ROUTABLE_TEMPLATE
#define AR_ROUTABLE_TEMPLATE

#include "arDataTemplate.h"
#include "arFrameworkCalling.h"

class SZG_CALL arRoutableTemplate: public arDataTemplate{
 public:
  arRoutableTemplate(const string&);
  ~arRoutableTemplate();
};

#endif
