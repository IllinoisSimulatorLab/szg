//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATABASE_LANGUAGE
#define AR_DATABASE_LANGUAGE
#include "arLanguage.h"

// Also want to include the built-in IDs for the pure database nodes here.
// NOTE: THERE IS A VERY WICKED KLUDGE GOING ON HERE! SPECIFICALLY, TO
// AVOID COLLISIONS I AM JUST MANUALLY MOVING STUFF AROUND!

enum{
  AR_D_NAME_NODE = 100
};

/// Generic language for an arDatabase.

class arDatabaseLanguage: public arLanguage{
 public:
  arDatabaseLanguage();
  ~arDatabaseLanguage(){};

  // These values must be public. This is how applications
  // are able to use high-speed, ID-based access to data fields.

  int AR_ERASE;
  int AR_ERASE_ID;

  int AR_MAKE_NODE;
  int AR_MAKE_NODE_PARENT_ID;
  int AR_MAKE_NODE_ID;
  int AR_MAKE_NODE_NAME;
  int AR_MAKE_NODE_TYPE;

  int AR_NAME;
  int AR_NAME_ID;
  int AR_NAME_NAME;

 protected:
  arDataTemplate _erase;
  arDataTemplate _makeNode;
  arDataTemplate _nameNode;
};

#endif
