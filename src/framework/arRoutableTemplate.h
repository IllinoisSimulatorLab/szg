//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_ROUTABLE_TEMPLATE
#define AR_ROUTABLE_TEMPLATE

#include "arDataTemplate.h"
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arFrameworkCalling.h"

class SZG_CALL arRoutableTemplate: public arDataTemplate{
 public:
  arRoutableTemplate(const string&);
  ~arRoutableTemplate();
};

#endif
