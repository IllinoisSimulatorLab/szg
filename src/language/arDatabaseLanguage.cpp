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
_nameNode("name_node"),
_insertNode("insert_node"),
_cut("cut"),
_permute("permute"){
  AR_ERASE_ID = _erase.add("ID", AR_INT);
  AR_ERASE = _dictionary.add(&_erase);

  AR_MAKE_NODE_PARENT_ID = _makeNode.add("parent_ID", AR_INT);
  AR_MAKE_NODE_ID = _makeNode.add("ID", AR_INT);
  AR_MAKE_NODE_NAME = _makeNode.add("name", AR_CHAR);
  AR_MAKE_NODE_TYPE = _makeNode.add("type", AR_CHAR);
  AR_MAKE_NODE = _dictionary.add(&_makeNode);

  AR_NAME_ID = _nameNode.add("ID", AR_INT);
  AR_NAME_NAME = _nameNode.add("name", AR_CHAR);
  AR_NAME_INFO = _nameNode.add("info", AR_CHAR);
  AR_NAME = _dictionary.add(&_nameNode);

  AR_INSERT_PARENT_ID = _insertNode.add("parent_ID", AR_INT);
  AR_INSERT_CHILD_ID = _insertNode.add("child_ID", AR_INT);
  AR_INSERT_ID = _insertNode.add("ID", AR_INT);
  AR_INSERT_NAME = _insertNode.add("name", AR_CHAR);
  AR_INSERT_TYPE = _insertNode.add("type", AR_CHAR);
  AR_INSERT = _dictionary.add(&_insertNode);

  AR_CUT_ID = _cut.add("ID", AR_INT);
  AR_CUT = _dictionary.add(&_cut);

  AR_PERMUTE_PARENT_ID = _permute.add("parent_ID", AR_INT);
  AR_PERMUTE_CHILD_IDS = _permute.add("child_IDs", AR_INT);
  AR_PERMUTE = _dictionary.add(&_permute);
}
