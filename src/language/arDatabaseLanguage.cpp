//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDatabaseLanguage.h"

arDatabaseLanguage::arDatabaseLanguage():
_erase("erase"),
_makeNode("make node"),
_nameNode("name_node"){
  AR_ERASE_ID = _erase.add("ID",AR_INT);
  AR_ERASE = _dictionary.add(&_erase);

  AR_MAKE_NODE_PARENT_ID = _makeNode.add("parent_ID",AR_INT);
  AR_MAKE_NODE_ID = _makeNode.add("ID",AR_INT);
  AR_MAKE_NODE_NAME = _makeNode.add("name",AR_CHAR);
  AR_MAKE_NODE_TYPE = _makeNode.add("type",AR_CHAR);
  AR_MAKE_NODE = _dictionary.add(&_makeNode);

  AR_NAME_ID = _nameNode.add("ID",AR_INT);
  AR_NAME_NAME = _nameNode.add("name",AR_CHAR);
  AR_NAME = _dictionary.add(&_nameNode);
}
