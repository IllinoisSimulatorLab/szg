//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arStructuredData.h"
#include "arDatabaseLanguage.h" // for dataInMakenode()
#include "arDatabaseNode.h"     // for dataInMakenode()

static const char* debugTypename(int i){
  const char* internalTypeNames[6] = {
    "AR_GARBAGE", "AR_CHAR", "AR_INT", "AR_LONG", "AR_FLOAT", "AR_DOUBLE" };
  if (i>=0 && i<6)
    return internalTypeNames[i];

  static int j = 0; // feeble attempt to support multiple calls from one printf
  const int jMax = 10;
  static char buf[jMax][20];
  ++j %= jMax;
  sprintf(buf[j], "[unknown: %d]", i);
  return buf[j];
}

//**************************************************************************
// there's a mysterious problem with this constructor: given arDataTemplate
// myTemplate, simultaneous calls to arStructuredData data1(&myTemplate)
// and arStructuredData data2(&myTemplate) in seperate threads will
// segfault in a free call in the STL
//**************************************************************************

arStructuredData::arStructuredData(arDataTemplate* theTemplate) :
  _fValid(false) {
  _construct(theTemplate);
}

void arStructuredData::_construct(arDataTemplate* theTemplate){
  if (!theTemplate) {
    cerr << "arStructuredData warning: constructor got NULL template.\n";
    return;
    }
  
  _dataID = theTemplate->getID();
  _numberDataItems = theTemplate->getNumberAttributes();
  
  _name = theTemplate->getName();
  
  if (_numberDataItems < 0 || _numberDataItems > 100000){
    cerr << "arStructuredData error: constructor's template \""
      << _name << "\" has "
      << _numberDataItems << " _numberDataItems;  truncating to 1.\n";
    _numberDataItems = 1;
  }

  _fValid = true;
  if (_numberDataItems == 0)
    return;
  _dataPtr = new void*[_numberDataItems];
  _owned = new bool[_numberDataItems];
  _dataDimension = new ARint[_numberDataItems];
  _storageDimension = new ARint[_numberDataItems];
  _dataName = new string[_numberDataItems];
  _dataType = new arDataType[_numberDataItems];
  for (arAttributeIterator iAttribute(theTemplate->attributeBegin());
       iAttribute != theTemplate->attributeEnd();
       ++iAttribute){
    const int i = iAttribute->second.second;
    _dataPtr[i] = NULL;
    _owned[i] = false;
    _dataDimension[i] = 0;
    _storageDimension[i] = 0;
    _dataName[i] = iAttribute->first;
    _dataType[i] = iAttribute->second.first;
    _dataNameMap.insert(arNameMap::value_type
    		        (iAttribute->first,i));
    if (_dataType[i] == AR_GARBAGE){
      cerr << "arStructuredData warning: constructor's template \""
      << _name << "\" has not-yet-typed attribute (# "
      << i << ", name \"" << _dataName[i] << "\").\n";
    }
  }
}

arStructuredData::arStructuredData(arTemplateDictionary* d, const char* name) :
  _fValid(false) {
  arDataTemplate* t = d->find(name);
  if (!t) {
    cerr << "arStructuredData error: no \"" << name << "\" in dictionary.\n";
    return;
  }
  _construct(t);
}

const char* arDataTypeName(arDataType theType){
  static const char* names[6] = {
    "ARgarbage", "ARchar", "ARint", "ARlong", "ARfloat", "ARdouble" };
  return names[int(theType)];
}

arDataType arDataNameType(const char* const theName) {
  static const char* names[6] = {
    "ARgarbage", "ARchar", "ARint", "ARlong", "ARfloat", "ARdouble" };
  for (int i=0; i<6; i++)
    if (strcmp(theName,names[i])==0)
      return arDataType(i);
  return AR_GARBAGE;
}

int arDataTypeSize(arDataType theType){
  // Faster than a switch(), but crashes with bad input data.
  static const int sizes[6] = {
    AR_GARBAGE_SIZE,
    AR_CHAR_SIZE,
    AR_INT_SIZE,
    AR_LONG_SIZE,
    AR_FLOAT_SIZE,
    AR_DOUBLE_SIZE};
  const int t = int(theType);
  if (t<0 || t>AR_DOUBLE) {
    cerr << "syzygy warning: unknown arDataType " << theType << endl;
    return -1;
  }
  return sizes[t];
}

void arStructuredData::dump(bool verbosity)
{
  // This really needs a class-variable mutex so multiple threads' dump-output
  // don't interleave.

  cout << "----arStructuredData----------------------------\n"
       << "dataID = " << _dataID << endl
       << "numberDataItems = " << _numberDataItems << endl
       << "name = \"" << _name << "\"" << endl
       << "size = " << size() << "\n";

  for (int i=0; i<_numberDataItems; i++){
    // readable
    cout << "[" << i << "]: "
	 << (_owned[i] ? "owned " : "      ")
	 << arDataTypeName(_dataType[i]) << " \""
	 << _dataName[i] << "\"["
	 << _dataDimension[i] << " of "
	 << _storageDimension[i] << "]\n";
    if (verbosity){
      cout << "  ";
      for (int j=0; j<_dataDimension[i]; j++){
	switch (_dataType[i]) {
	case AR_CHAR:
	  cout << ((ARchar*)_dataPtr[i])[j] << " ";
	  break;
	case AR_INT:
	  cout << ((ARint*)_dataPtr[i])[j] << " ";
	  break;
	case AR_FLOAT:
	  cout << ((ARfloat*)_dataPtr[i])[j] << " ";
	  break;
	case AR_DOUBLE:
	  cout << ((ARdouble*)_dataPtr[i])[j] << " ";
	  break;
	case AR_LONG:
	  cout << ((ARlong*)_dataPtr[i])[j] << " ";
	  break;
	}
      }
      cout << "\n";
    }
  }

  cout << "------------------------------------------------\n";
}

void arStructuredData::copy(arStructuredData const& rhs){
  _dataID = rhs._dataID;
  _numberDataItems = rhs._numberDataItems;
  _name = rhs._name;
  if (_numberDataItems == 0)
    return;

  _dataPtr = new void*[_numberDataItems];
  _owned = new bool[_numberDataItems];
  _dataDimension = new ARint[_numberDataItems];
  _storageDimension = new ARint[_numberDataItems];
  _dataName = new string[_numberDataItems];
  _dataType = new arDataType[_numberDataItems];
  _dataNameMap = rhs._dataNameMap;
  for (int i=0; i<_numberDataItems; i++){
    _dataDimension[i] = rhs._dataDimension[i];
    _storageDimension[i] = rhs._storageDimension[i];
    _dataName[i] = rhs._dataName[i];
    _dataType[i] = rhs._dataType[i];
    _owned[i] = rhs._owned[i];
    if (_owned[i]){
      // Internally managed memory: make a local copy.
      _dataPtr[i] =
        new ARchar[_storageDimension[i] * arDataTypeSize(_dataType[i])];
      memcpy((char*)_dataPtr[i], (const char*)rhs._dataPtr[i],
        _dataDimension[i] * arDataTypeSize(_dataType[i]));
    }
    else{
      // Externally managed memory: just get the pointer.
      _dataPtr[i] = rhs._dataPtr[i];
    }
  }
}

void arStructuredData::destroy()
{
  if (_numberDataItems == 0)
    return;
  for (int i=0; i<_numberDataItems; i++){
    if (_dataPtr[i] && _owned[i]){
      delete [] (ARchar*) _dataPtr[i];
    }
  }
  delete [] _dataPtr;
  delete [] _owned;
  delete [] _dataDimension;
  delete [] _storageDimension;
  delete [] _dataName;
  delete [] _dataType;
}

arStructuredData const& arStructuredData::operator=(
        arStructuredData const& rhs){
  if (this != &rhs) {
    destroy();
    copy(rhs);
  }
  return *this;
}

arStructuredData::arStructuredData(arStructuredData const& rhs){
  copy(rhs);
}

arStructuredData::~arStructuredData(){
  destroy();
}

void* arStructuredData::getDataPtr(int field, arDataType type){
  if (_numberDataItems <= 0){
    cerr << "arStructuredData warning: ignoring getDataPtr() on empty arDataTemplate for \"" << _name << "\".\n";
    return NULL;
    }
  if (field<0 || field>=_numberDataItems){
    cerr << "arStructuredData warning: getDataPtr() failed with field " << field
         << " out of range [0," << _numberDataItems-1 << "] for \"" << _name << "\".\n";
    return NULL;
  }
  if (_dataType[field] == AR_GARBAGE){
    cerr << "arStructuredData warning: ignoring getDataPtr() on not-yet-typed field "
         << field << " (name is \""
         << _name << "/" << _dataName[field] << "\").\n";
    return NULL;
  } 
  if (type != _dataType[field]){
    cerr << "arStructuredData warning: getDataPtr() failed with type mismatch(\n"
         << arDataTypeName(type) << ", expected "
	 << arDataTypeName(_dataType[field]) << ") for \"" << _name << "\".\n";
    return NULL;
  }
  return _dataPtr[field];
}

int arStructuredData::getDataDimension(int field){
  if (_numberDataItems <= 0){
    cerr << "arStructuredData warning: ignoring getDataDimension() on\n"
         << "  empty arDataTemplate for \"" << _name << "\".\n";
    return 0;
    }
  if (field<0 || field>=_numberDataItems){
    cerr << "arStructuredData warning: getDataDimension() failed with field "
         << field << " out of range [0," << _numberDataItems-1 << "]. for \""
	 << _name << "\".\n";
    return 0;
  }
  if (_dataType[field] == AR_GARBAGE){
    cerr << "arStructuredData warning: getDataDimension() failed on\n"
         << "  not-yet-typed field " << field << " in \"" << _name << "\".\n";
    return 0;
  } 
  const int dim = _dataDimension[field];
  if (dim < 0){
    cerr << "arStructuredData warning: negative dimension "
         << dim << " for field "
         << field << " (\"" << _name << "/" << _dataName[field] << "\").\n";
    return 0;
    }
  return dim;
}

string arStructuredData::getDataString(int field){
  const char* pch = (char*)getDataPtr(field, AR_CHAR);
  if (!pch)
    return string(""); // Don't complain; getDataPtr already did.
  return string(pch, getDataDimension(field));
}

string arStructuredData::getDataString(const string& fieldName){
  const arNameMap::iterator i(_dataNameMap.find(fieldName));
  if (i == _dataNameMap.end()){
    cerr << "arStructuredData warning: no field \"" << fieldName << "\".\n";
    return string("NULL");
  }
  return getDataString(i->second);
}

bool arStructuredData::setDataDimension(int field, int dim){
  if (_numberDataItems <= 0){
    cerr << "arStructuredData warning: ignoring setDataDimension() on empty arDataTemplate. for \"" << _name << "\".\n";
    return false;
    }
  if (field<0 || field>=_numberDataItems){
    cerr << "arStructuredData warning: setDataDimension() failed with field " << field
         << " out of range [0," << _numberDataItems-1 << "]. for \"" << _name << "\".\n";
    return false;
  }
  if (dim<0){
    cerr << "arStructuredData warning: setDataDimension() failed with negative dimension "
         << dim << " for \"" << _name << "\".\n";
    return false;
  }
  if (_dataType[field] == AR_GARBAGE){
    cerr << "arStructuredData warning: setDataDimension() failed on not-yet-typed field "
         << field << ".\n";
    return false;
  } 

  if (dim>_storageDimension[field]){
    int copySize = _storageDimension[field]*arDataTypeSize(_dataType[field]);
    const int totalSize = dim*arDataTypeSize(_dataType[field]);
    if (_dataPtr[field]){
      // we need to allocate a new memory chunk and copy the old data
      ARchar* source = (ARchar*) _dataPtr[field];
      ARchar* dest = new ARchar[totalSize];
      memcpy(dest,source,copySize);
      _dataPtr[field] = dest;
      _storageDimension[field] = dim;
      if (_owned[field]){
        // only delete if this was previosuly owned
	delete [] source;
      }
    }
    else{
      // need a new memory chunk... but we weren't managing an old one
      _dataPtr[field] = new ARchar[totalSize];
      _storageDimension[field] = dim;
    }
    _owned[field] = true;
  }
  _dataDimension[field] = dim;
  return true;
}

int arStructuredData::getStorageDimension(int field){
  if (_numberDataItems <= 0){
    cerr << "arStructuredData warning: ignoring getStorageDimension() for empty \""
         << _name << "\".\n";
    return -1;
    }
  if (field<0 || field>=_numberDataItems){
    cerr << "arStructuredData warning: getStorageDimension() failed with field " << field
         << " out of range [0," << _numberDataItems-1 << "] for \""
	 << _name << "\".\n";
    return -1;
  }
  return _storageDimension[field];
}

arDataType arStructuredData::getDataType(int fieldIndex) const {
  if (_numberDataItems <= 0){
    cerr << "arStructuredData warning: ignoring getDataType() for empty \""
         << _name << "\".\n";
    return AR_GARBAGE;
    }
  if (fieldIndex<0 || fieldIndex>=_numberDataItems){
    cerr << "arStructuredData warning: getDataType() failed with field " << fieldIndex
         << " out of range [0," << _numberDataItems-1 << "] for \""
	 << _name << "\".\n";
    return AR_GARBAGE;
  }
  return _dataType[fieldIndex];
}


// This does *not* update _dataDimension, so if you have a pile of data
// already which you're adding to, then call
// setDataDimension() instead of setStorageDimension().
//
bool arStructuredData::setStorageDimension(int field, int dim){
  if (_numberDataItems <= 0){
    cerr << "arStructuredData warning: ignoring setStorageDimension() on empty arDataTemplate for \""
         << _name << "\".\n";
    return false;
  }
  if (field<0 || field>=_numberDataItems){
    cerr << "arStructuredData warning: setStorageDimension() failed with field " << field
         << " out of range [0," << _numberDataItems-1 << "] for \""
	 << _name << "\".\n";
    return false;
  }
  if (dim<=0){
    cerr << "arStructuredData warning: setStorageDimension() failed with negative dimension "
         << dim << " for \"" << _name << "\".\n";
    return false;
  }
  if (dim < _dataDimension[field]){
    cerr << "arStructuredData warning: setStorageDimension() failed with dimension "
         << dim << " less than field size "
	 << _dataDimension[field] << " for \"" << _name << "\".\n";
    return false;
  } 
  if (_dataType[field] == AR_GARBAGE){
    cerr << "arStructuredData warning: setStorageDimension() failed on not-yet-typed field "
         << field << ".\n";
    return false;
  } 
  
  const int totalSize = dim * arDataTypeSize(_dataType[field]);

  if (_dataPtr[field]){
    // Allocate a new chunk of memory and copy old data into it.
    ARchar* source = (ARchar*) _dataPtr[field];
    ARchar* dest = new ARchar[totalSize];
    memcpy(dest, source,
           _dataDimension[field] * arDataTypeSize(_dataType[field]));
    _dataPtr[field] = dest;
    if (_owned[field]){
      // Delete the old chunk of memory.
      delete [] source;
    }
  }
  else{
    // Allocate a completely new chunk of memory.
    _dataDimension[field] = 0;
    _dataPtr[field] = new ARchar[totalSize];
  }
  _owned[field] = true;
  _storageDimension[field] = dim;
  return true;
}

void* arStructuredData::getDataPtr(const string& fieldName, arDataType theType){
  const arNameMap::iterator i(_dataNameMap.find(fieldName));
  if (i == _dataNameMap.end()){
    cerr << "arStructuredData warning: no field \"" << fieldName << "\".\n";
    return NULL;
  }
  return getDataPtr(i->second, theType);
}

int arStructuredData::getDataDimension(const string& fieldName){
  const arNameMap::iterator i(_dataNameMap.find(fieldName));
  if (i == _dataNameMap.end()){
    cerr << "arStructuredData warning: no field \"" << fieldName << "\".\n";
    return 0;
  }
  return getDataDimension(i->second);
}

arDataType arStructuredData::getDataType(const string& fieldName) {
  const arNameMap::iterator i(_dataNameMap.find(fieldName));
  if (i == _dataNameMap.end()){
    cerr << "arStructuredData warning: no field \"" << fieldName << "\".\n";
    return AR_GARBAGE;
  }
  return getDataType(i->second);
}

bool arStructuredData::setDataDimension(const string& fieldName, int theSize){
 const arNameMap::iterator i(_dataNameMap.find(fieldName));
  if (i == _dataNameMap.end()){
    cerr << "arStructuredData warning: no field \"" << fieldName << "\".\n";
    return false;
  }
  return setDataDimension(i->second, theSize);
}

bool arStructuredData::dataIn(int field, const void* data,
                               arDataType type, int dim){
/*
  cerr << "arStructuredData::dataIn _numberDataItems==" << _numberDataItems
       << ", type is " << type
       << ", dim is " << dim
       << endl;;;; 
*/
  if (_numberDataItems <= 0){
    cerr << "arStructuredData warning: ignoring dataIn() on "
         << "empty arDataTemplate for \"" << _name << "\".\n";
    return false;
    }
  if (field<0 || field >= _numberDataItems){
    cerr << "arStructuredData warning: ignoring dataIn() with field " << field
         << " out of range [0," << _numberDataItems-1 << "].\n";
    return false;
  }
  if (_dataType[field] == AR_GARBAGE){
    cerr << "arStructuredData warning: ignoring dataIn() on "
         << "not-yet-typed field "
         << field << " for \"" << _name << "\".\n";
    return false;
  } 
  if (type != _dataType[field]){
    cerr << "arStructuredData warning: ignoring dataIn() with type mismatch ("
         << arDataTypeName(type) << ", expected "
	 << arDataTypeName(_dataType[field]) << ") for \"" << _name << "\".\n";
    return false;
  } 
  if (dim < 0){
    cerr << "arStructuredData warning: ignoring dataIn() with negative "
	 << "dimension " << dim
         << " for \"" << _name << "\".\n";
    return false;
  }
  if (!data){
    cerr << "arStructuredData warning: ignoring dataIn() with null data "
	 << "source "
         << " for \"" << _name << "\".\n";
    return false;
  }
  const int cb = dim*arDataTypeSize(_dataType[field]);
  if (dim>_storageDimension[field] || !_owned[field]){
    if (_dataPtr[field] && _owned[field]){
      delete [] (ARchar*) _dataPtr[field];
    }

    _dataPtr[field] = new ARchar[cb];
    // sometimes more bytes are read from this block than were allocated!

    _storageDimension[field] = dim; 
    _owned[field] = true;
  }
  _dataDimension[field] = dim;
  if (cb != 0)
    memcpy(_dataPtr[field], data, cb);
  return true;
}

bool arStructuredData::dataOut(int field, void* destination,
                               arDataType type, int dim){
  if (_numberDataItems <= 0){
    cerr << "arStructuredData warning: ignoring dataOut() on empty arDataTemplate for \"" << _name << "\".\n";
    return false;
    }
  if (field<0 || field >= _numberDataItems){
    cerr << "arStructuredData error: \""
      << _name << "\", dataOut field " << field
      << " is out of range [0, "
      << _numberDataItems-1 << "]\n";
    return false;
  }
  if (_dataType[field] == AR_GARBAGE){
    cerr << "arStructuredData warning: ignoring dataOut() on not-yet-typed field "
         << field << " for \"" << _name << "\".\n";
    return false;
  } 
  if (type != _dataType[field]){
    cerr << "arStructuredData error: \""
      << _name << "\", dataOut field " << field
      << " (" << _dataName[field] << ") has type "
      << debugTypename(type) << ";  expected "
      << debugTypename(_dataType[field]) << " instead.\n";

#if 0
    cout << "\n\t___DUMP " << _name << "___\n\n";
    for (int i=0; i<_numberDataItems; i++){
      cout << "\tname: "
      << _dataName[i]
      << "\t  type: "
      << int(_dataType[i])
      << "  dim: "
      << _dataDimension[i]
      << endl;
      }
    cout << "\n\n";
    cout << "coredump:  " << *(int*)0 << endl;
    exit(-42);
#endif

    return false;
  }
  if (dim == 0){
    // nothing to do!
    return true;
  }
  if (dim < 0){
    cerr << "arStructuredData error: \""
      << _name << "\", dataOut field "
      << field << " (\"" << _name << "/" << _dataName[field] << "\")"
      << " has negative dimension "
      << dim << ".\n";
    return false;
  }
  if (_dataDimension[field] < 0){
    cerr << "arStructuredData error: \""
      << _name << "\", dataOut field "
      << field << " (\"" << _name << "/" << _dataName[field] << "\")"
      << " initialized with negative dimension "
      << _dataDimension[field] << ".\n";
    return false;
  }
  if (dim > _dataDimension[field]){
    cerr << "arStructuredData error: \""
      << _name << "\", dataOut request for field "
      << field << " (\"" << _name << "/" << _dataName[field] << "\")"
      << " has dimension "
      << dim << " which exceeds " << _dataDimension[field] << ".\n";
    return false;
  }
  if (!_dataPtr[field]){
    cerr << "arStructuredData error: \""
      << _name << "\", dataOut field "
      << field << " (\"" << _name << "/" << _dataName[field] << "\")"
      << " has a NULL data pointer.\n";
    return false;
  }
  memcpy(destination, _dataPtr[field], dim * arDataTypeSize(type));
  return true;
}

bool arStructuredData::dataIn(const string& fieldName, const void* data,
			      arDataType theType, int dimension){
  if (data == 0 || dimension <= 0) {
    // Convenient abbreviation.
    return setDataDimension(fieldName, 0);
  }

  const arNameMap::iterator i(_dataNameMap.find(fieldName));
  if (i == _dataNameMap.end()){
    cerr << "arStructuredData warning: no field \"" << fieldName << "\".\n";
    return false;
  }
  return dataIn(i->second, data, theType, dimension);
}

int arStructuredData::getDataFieldIndex( const string& fieldName ) {
  const arNameMap::iterator i(_dataNameMap.find(fieldName));
  if (i == _dataNameMap.end()){
    cerr << "arStructuredData warning: no field \"" << fieldName << "\".\n";
    return -1;
  }
  return i->second;
}

bool arStructuredData::getDataFieldName( int index, string& fieldName ) {
  arNameMap::iterator iter;
  for (iter = _dataNameMap.begin(); iter != _dataNameMap.end(); iter++) {
    if (iter->second == index) {
      fieldName = iter->first;
      return true;
    }
  }
  return false;
}

void arStructuredData::getFieldNames( std::vector< std::string >& names ) const {
  arNameMap::const_iterator iter;
  for (iter = _dataNameMap.begin(); iter!= _dataNameMap.end(); iter++)
    names.push_back( iter->first );
}

bool arStructuredData::dataOut(const string& fieldName, void* data,
			       arDataType theType, int dimension){
  const arNameMap::iterator i(_dataNameMap.find(fieldName));
  if (i == _dataNameMap.end()){
    cerr << "arStructuredData warning: no field \"" << fieldName << "\".\n";
    return false;
  }
  return dataOut(i->second, data, theType, dimension);
}

bool arStructuredData::ptrIn(int field, void* ptr, int dim){
  if (_numberDataItems <= 0){
    cerr << "arStructuredData warning: ptrIn failed on empty \""
         << _name << "\".\n";
    return false;
  }
  if (field < 0 || field >= _numberDataItems){
    cerr << "arStructuredData warning: ptrIn failed because field "
         << field << " in \"" << _name << "\" is out of range [0, "
	 << _numberDataItems << "].\n";
    return false;
  }
  if (dim < 0){
    cerr << "arStructuredData warning: ptrIn failed on field "
         << field << " in \"" << _name << "\" because dimension "
	 << dim << " is negative.\n";
    return false;
  }
  if (_owned[field] && _dataPtr[field]){
    delete [] (ARchar*) _dataPtr[field];
  }
  _dataPtr[field] = ptr;
  _dataDimension[field] = dim;
  _storageDimension[field] = dim;
  _owned[field] = false;
  return true;
}

#ifdef UNUSED
void* arStructuredData::ptrOut(int field){
  if (field < 0 || field >= _numberDataItems){
    cerr << "arStructuredData warning: ptrOut failed because field "
         << field << " in \"" << _name << "\" is out of range [0, "
	 << _numberDataItems << "].\n";
    return NULL;
  }
  return _dataPtr[field];
}
#endif

int arStructuredData::size() const {
  // 3 ARint entries for the data header
  // 2 Arint entries per field header
  int total = 3*AR_INT_SIZE;
  for (int i=0; i<_numberDataItems; i++){
    total += 2*AR_INT_SIZE +
             ar_fieldSize(_dataType[i],_dataDimension[i]) +
             ar_fieldOffset(_dataType[i],total);
  }
  // records are aligned at 8 byte boundaries
  return total + ar_fieldOffset(AR_DOUBLE,total);
}

void arStructuredData::pack(ARchar* destination){
  const ARint theSize = size();
  ARint offset = 0;
  ar_packData(destination + offset, &theSize, AR_INT, 1);
  offset += AR_INT_SIZE;

  ar_packData(destination + offset, &_dataID, AR_INT, 1);
  offset += AR_INT_SIZE;

  ar_packData(destination + offset, &_numberDataItems, AR_INT, 1);
  offset += AR_INT_SIZE;

  for (int i=0; i<_numberDataItems; i++){
    ar_packData(destination + offset, &_dataDimension[i], AR_INT, 1);
    offset += AR_INT_SIZE;

    const arDataType type = _dataType[i];
    const ARint dim = _dataDimension[i];
    if (type == AR_GARBAGE)
      cerr << "arStructuredData::pack warning: type AR_GARBAGE for field "
	   << i << " (\"" << _name << "/" << _dataName[i] << "\").\n";
    if (dim < 0)
      cerr << "arStructuredData::pack warning: negative dimension "
           << dim << " for field "
	   << i << " (\"" << _name << "/" << _dataName[i] << "\").\n";

    ar_packData(destination + offset, &type, AR_INT, 1);
    offset += AR_INT_SIZE;
    offset += ar_fieldOffset(_dataType[i],offset);

    ar_packData(destination + offset, _dataPtr[i], type, dim);
    offset += ar_fieldSize(_dataType[i],_dataDimension[i]);
  }
}

bool arStructuredData::unpack(ARchar* source){
  ARint theSize = -1;
  ARint offset = 0;
  ar_unpackData(source,&theSize,AR_INT,1);
  offset += AR_INT_SIZE;
  if (theSize <= 0){
    cerr << "arStructuredData::unpack error: size is " << theSize << endl;
    return false;
  }
  ar_unpackData(source+offset,&_dataID,AR_INT,1);
  offset += AR_INT_SIZE;
  ARint numberFields = -1;
  ar_unpackData(source+offset,&numberFields,AR_INT,1);
  offset += AR_INT_SIZE;
  if (numberFields != _numberDataItems){
    cerr << "arStructuredData::unpack error: found " << numberFields
      << " instead of " << _numberDataItems << " data fields.\n";
    cerr << "\tfyi, overall size is " << theSize << " bytes, and dataID is "
      << _dataID << ".\n";
    return false;
  }
  for (int i=0; i<_numberDataItems; i++){
    ARint dim, type;
    ar_unpackData(source+offset,&dim,AR_INT,1);
    offset += AR_INT_SIZE;
    ar_unpackData(source+offset,&type,AR_INT,1);
    if (type == AR_GARBAGE)
      cerr << "arStructuredData::unpack warning: got an AR_GARBAGE type.\n";
    offset += AR_INT_SIZE;
    if (!setDataDimension(i,dim)) {
      cerr << "arStructuredData error: setDataDimension failed.\n";
      return false;
    }
    // remember that we align AR_DOUBLE fields on an eight byte boundary
    offset += ar_fieldOffset((arDataType)type,offset);
    ar_unpackData(source+offset, _dataPtr[i], (arDataType)type, dim);
    offset += ar_fieldSize((arDataType)type,dim);
  }
  return true;
}

bool arStructuredData::parse(ARchar* source){
  ARint theSize = -1;
  ARint offset = 0;
  ar_unpackData(source,&theSize,AR_INT,1);
  offset += AR_INT_SIZE;
  if (theSize <= 0){
    cerr << "arStructuredData::parse error: size is " << theSize << endl;
    return false;
  }
  ar_unpackData(source+offset,&_dataID,AR_INT,1);
  offset += AR_INT_SIZE;
  ARint numberFields = -1;
  ar_unpackData(source+offset,&numberFields,AR_INT,1);
  offset += AR_INT_SIZE;
  if (numberFields != _numberDataItems){
    cerr << "arStructuredData::parse error: found " << numberFields
      << " instead of " << _numberDataItems << " data fields.\n";
    cerr << "\tfyi, overall size is " << theSize << " bytes, and dataID is "
      << _dataID << ".\n";
    return false;
  }

  for (int i=0; i<_numberDataItems; i++){
    ARint dim = -1, type = -1;
    ar_unpackData(source+offset,&dim,AR_INT,1);
    if (dim < 0){
      cerr << "arStructuredData::parse error: got a negative dimension "
           << dim << " in "
           << i+1 << " of " << _numberDataItems << " records.\n";
      return false;
    }
    offset += AR_INT_SIZE;
    ar_unpackData(source+offset,&type,AR_INT,1);
    if (type == AR_GARBAGE){
      cerr << "arStructuredData::parse error: got an AR_GARBAGE type in "
           << i+1 << " of " << _numberDataItems << " "
	   << "records.  (dim == "
	   << dim << ".)\n";
      return false;
    }
    offset += AR_INT_SIZE;
    // remember that we align AR_DOUBLE fields on an eight byte boundary
    offset += ar_fieldOffset((arDataType)type,offset);
    if (!ptrIn(i, source+offset, dim)){
      return false;
    }
    offset += ar_fieldSize((arDataType)type,dim);
  }
  return true;
}

void arStructuredData::print(){
  print(stdout);
}

void arStructuredData::print(ostream& s){
  const int numbersInRow = 8;
  s << "<" << _name << ">\n";
  for (int i=0; i<_numberDataItems; i++){
    int j;
    s << "  <" << _dataName[i] << ">";
    if (_dataType[i] == AR_CHAR){
      for (j=0; j<_dataDimension[i]; j++){
        s << ((ARchar*)_dataPtr[i])[j];
      }
    }
    else{
      s << "\n    ";
      for (j=0; j<_dataDimension[i]; j++){
        switch (_dataType[i]) {
	case AR_INT:
	  s << ((ARint*)_dataPtr[i])[j] << " ";
	  break;
	case AR_FLOAT:
	  s << ((ARfloat*)_dataPtr[i])[j] << " ";
	  break;
	case AR_DOUBLE:
	  s << ((ARdouble*)_dataPtr[i])[j] << " ";
	  break;
	case AR_LONG:
	  s << ((ARlong*)_dataPtr[i])[j] << " ";
	  break;
	}
	if (j%numbersInRow == numbersInRow-1)
	  s << "\n    ";
      }
      if (_dataDimension[i] % numbersInRow != 0)
	s << "\n";
      s << "  ";
    }
    s << "</" << _dataName[i] << ">\n";
  }
  s << "</" << _name << ">\n";
}

void arStructuredData::print(FILE* theFile){
  // some *very* rudimentary error checking
  if (!theFile){
    cerr << "arStructuredData error: print to file failed.\n";
    return;
  }
  const int numbersInRow = 8;
  fprintf(theFile, "<%s>\n", _name.c_str());
  for (int i=0; i<_numberDataItems; i++){
    int j;
    fprintf(theFile, "  <%s>", _dataName[i].c_str());
    if (_dataType[i] == AR_CHAR){
      for (j=0; j<_dataDimension[i]; j++){
	fprintf(theFile, "%c", ((ARchar*)_dataPtr[i])[j]);
      }
    }
    else{
      fprintf(theFile, "\n    ");
      for (j=0; j<_dataDimension[i]; j++){
	// the following format choices are almost certainly not good
	// enough! fix them as problems arise!
        switch (_dataType[i]) {
	case AR_INT:
	  fprintf(theFile, "%i ", ((ARint*)_dataPtr[i])[j]);
	  break;
	case AR_FLOAT:
	  fprintf(theFile, "%f ", ((ARfloat*)_dataPtr[i])[j]);
	  break;
	case AR_DOUBLE:
	  fprintf(theFile, "%lg ", ((ARdouble*)_dataPtr[i])[j]);
	  break;
	case AR_LONG:
	  fprintf(theFile, "%ld ", ((ARlong*)_dataPtr[i])[j]);
	  break;
	}
	if (j%numbersInRow == numbersInRow-1)
	  fprintf(theFile, "\n    ");
      }
      if (_dataDimension[i] % numbersInRow != 0)
	fprintf(theFile, "\n");
      fprintf(theFile, "  ");
    }
    fprintf(theFile, "</%s>\n", _dataName[i].c_str());
  }
  fprintf(theFile, "</%s>\n\n", _name.c_str());
}


bool ar_getStructuredDataStringField( arStructuredData* dataPtr,
                        const string& name,
                        string& value ) {
  const char* charPtr = (char*)dataPtr->getDataPtr(name,AR_CHAR);
  if (!charPtr) {
    cerr << "ar_getStringField error: no pointer to field " << name << endl;
    return false;
  }
  const int dataSize = dataPtr->getDataDimension(name);
  if (dataSize==0) {
    cerr << "ar_getStringField error: empty field " << name << endl;
    return false; // we'll take an empty field as a failure here
  }
  char *stringPtr = new char[dataSize+1];
  if (!stringPtr) {
    cerr << "ar_getStringField error: out of memory.\n";
    return false;
  }
  memcpy( stringPtr, charPtr, dataSize );
  stringPtr[dataSize] = '\0';
  value = string( stringPtr );
  delete [] stringPtr;
  return true;
}

ostream& operator<<(ostream&s, const arStructuredData& d ) { 
  const_cast<arStructuredData&>(d).print(s); return s; 
}
