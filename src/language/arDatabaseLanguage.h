//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATABASE_LANGUAGE
#define AR_DATABASE_LANGUAGE
#include "arLanguage.h"
#include "arLanguageCalling.h"

// Also want to include the built-in IDs for the pure database nodes here.
// NOTE: THERE IS A VERY WICKED KLUDGE GOING ON HERE! SPECIFICALLY, TO
// AVOID COLLISIONS I AM JUST MANUALLY MOVING STUFF AROUND!

enum{
  AR_D_NAME_NODE = 100
};

// Generic language for an arDatabase.

class SZG_CALL arDatabaseLanguage: public arLanguage{
 public:
  arDatabaseLanguage();
  ~arDatabaseLanguage() {};

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
  int AR_NAME_INFO;

  int AR_INSERT;
  int AR_INSERT_PARENT_ID;
  int AR_INSERT_CHILD_ID;
  int AR_INSERT_ID;
  int AR_INSERT_NAME;
  int AR_INSERT_TYPE;

  int AR_CUT;
  int AR_CUT_ID;

  int AR_PERMUTE;
  int AR_PERMUTE_PARENT_ID;
  int AR_PERMUTE_CHILD_IDS;

 protected:
  arDataTemplate _erase;
  arDataTemplate _makeNode;
  arDataTemplate _nameNode;
  arDataTemplate _insertNode;
  arDataTemplate _cut;
  arDataTemplate _permute;
};

#endif
