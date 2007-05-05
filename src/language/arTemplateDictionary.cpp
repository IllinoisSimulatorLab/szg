//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arTemplateDictionary.h"
#include "arLogStream.h"

arTemplateDictionary::arTemplateDictionary() :
  _numberTemplates(0)
{
}

arTemplateDictionary::arTemplateDictionary(arDataTemplate* t) :
  _numberTemplates(0)
{
  (void)add(t);
}

arTemplateDictionary::~arTemplateDictionary(){
  for (arOwnerType::iterator i = _ownershipContainer.begin();
       i != _ownershipContainer.end(); ++i){
    if (i->second){
      // Template was created by the dictionary (via unpack)
      delete _templateContainer.find(i->first)->second;
    }
  }
}

int arTemplateDictionary::add(arDataTemplate* theTemplate){
  if (!theTemplate){
    cerr << "arTemplateDictionary::add error: NULL template.\n";
    return -1;
  }
  const string& templateName = theTemplate->getName();
  if (_templateContainer.count(templateName) != 0){
    // This inevitably occurs when a fooServer dies and restarts,
    // because the templates the old one added are not removed from
    // _templateContainer -- only in the destructor is anything ever
    // removed from _templateContainer.

    cout << "arTemplateDictionary::add warning: ignoring duplicate template name \""
         << templateName << "\".\n";
    return theTemplate->getID();
  }

  // Set the ID of the template.
  theTemplate->_templateID = _numberTemplates++;
  _templateContainer.insert(arTemplateType::value_type
    (templateName, theTemplate));
  _templateIDContainer.insert(arTemplat2Type::value_type
    (theTemplate->_templateID, theTemplate));
  _ownershipContainer.insert(arOwnerType::value_type
    (templateName, false));
  return theTemplate->getID();
}

bool arTemplateDictionary::_addNoSetID(arDataTemplate* theTemplate){
  if (!theTemplate){
    cerr << "arTemplateDictionary::_addNoSetID error: NULL template.\n";
    return false;
  }
  const string& templateName = theTemplate->getName();
  if (_templateContainer.count(templateName) != 0){
#if 0
    cerr << "arTemplateDictionary::_addNoSetID error: duplicate template name \""
         << templateName << "\".\n";
    return false;
#else
    // This inevitably occurs when a fooServer dies and restarts,
    // because the templates the old one added are not removed from
    // _templateContainer -- only in the destructor is anything ever
    // removed from _templateContainer.
    // So just pretend that we added it, and don't complain.
    return true;
#endif
  }
  _templateContainer.insert(arTemplateType::value_type
    (templateName, theTemplate));
  _templateIDContainer.insert(arTemplat2Type::value_type
    (theTemplate->_templateID, theTemplate));
  _ownershipContainer.insert(arOwnerType::value_type
    (templateName, true));
  return true;
}

arDataTemplate* arTemplateDictionary::find(const string& name){
  arTemplateType::iterator iTemplate = _templateContainer.find(name);
  if (iTemplate == _templateContainer.end()){
    if (_templateContainer.empty()){
      cerr << "arTemplateDictionary error: empty, so find(\""
	   << name << "\") failed.\n";
    }
    else {
      cerr << "arTemplateDictionary error: no entry \"" << name << "\".\n"
           << "  (dictionary has these: ";
      for (iTemplate = _templateContainer.begin();
	   iTemplate != _templateContainer.end();
	   ++iTemplate) {
	cerr << iTemplate->first << "; ";
      }
      cerr << ")\n";
    }
    return NULL;
  }
  return iTemplate->second;
}

arDataTemplate* arTemplateDictionary::find(int ID){
  arTemplat2Type::iterator iter(
    _templateIDContainer.find(ID));
  return (iter == _templateIDContainer.end()) ? NULL :
    iter->second;
}

// For merging dictionaries (see arMasterSlaveDataRouter.)
void arTemplateDictionary::renumber(){
  _templateIDContainer.clear();
  arTemplateType::iterator i;
  for (i=_templateContainer.begin(); i != _templateContainer.end(); i++){
    _templateIDContainer.insert(
      arTemplat2Type::value_type(i->second->_templateID, i->second));
  }
}

int arTemplateDictionary::size() const {
  int total = 3*AR_INT_SIZE; // header
  for (arTemplateType::const_iterator iTemplate = _templateContainer.begin();
       iTemplate != _templateContainer.end(); ++iTemplate){
    // length of template's name
    total += ar_fieldSize(AR_CHAR,iTemplate->first.length());
    // 2 ARint for string field header, 3 for following AR_GARBAGE field, 3 for ID.
    total += 8*AR_INT_SIZE;
    const arDataTemplate* theTemplate = iTemplate->second;
    for (arAttribute::const_iterator iAttr(theTemplate->attributeConstBegin());
         iAttr != theTemplate->attributeConstEnd(); ++iAttr){
      total += ar_fieldSize(AR_CHAR, iAttr->first.length()) + 5*AR_INT_SIZE;
    }
  }
  // Pad record to an 8-byte boundary.
  total += ar_fieldOffset(AR_DOUBLE,total);
  return total; 
}

// Helper function for pack().
// @param position is modified in the caller.
static void ar_packDataField(
  ARchar* dest, ARint& position,
  arDataType type, ARint length,
  const void* data){
  ar_packData(dest+position, &length, AR_INT,1);
  position += AR_INT_SIZE;
  ar_packData(dest+position, &type, AR_INT,1);
  position += AR_INT_SIZE; 
  // AR_DOUBLE fields are 8-byte aligned.
  position += ar_fieldOffset(type,position);
  ar_packData(dest+position, data, type, length);
  // Pad char data.
  position += ar_fieldSize(type,length);
}

void arTemplateDictionary::pack(ARchar* dest) const {
  // (Fill in the first three fields later.)
  ARint position = 3*AR_INT_SIZE;
  ARint recordID = 0; // ID we'll assign to this record
  ARint numFields = 0;
  for (arTemplateType::const_iterator iTemplate = _templateContainer.begin();
       iTemplate != _templateContainer.end(); ++iTemplate){
    const string& templateName = iTemplate->first;

    // Pack the name field.
    ar_packDataField(dest, position, AR_CHAR,
      templateName.length(), templateName.data());

    // Pack the field indicating that this is a template name.
    // and not a data field
    ARint data = (ARint) AR_GARBAGE;
    ar_packDataField(dest, position, AR_INT, 1, &data);

    // Pack the field that holds the template ID.
    data = (ARint) iTemplate->second->getID();
    ar_packDataField(dest, position, AR_INT, 1, &data);

    numFields += 3;

    // Pack the template fields.
    arDataTemplate* theTemplate = iTemplate->second;

    // Quadratic instead of linear in getNumberAttributes(), sigh.
    for (int i = 0; i < theTemplate->getNumberAttributes(); ++i){

      // Pack the iAttr whose iAttr->second.second == i, so packing
      // has the same order as arStructuredData::arStructuredData().

      for (arAttribute::const_iterator iAttr(theTemplate->attributeBegin());
           iAttr != theTemplate->attributeEnd(); ++iAttr){
	if (iAttr->second.second != i)
	  continue;

	// Pack the field name.
	string attributeName = iAttr->first;
	ar_packDataField(dest, position, AR_CHAR,
	  attributeName.length(), attributeName.data());

	// Pack the data type.
	ARint attributeType = iAttr->second.first;
	ar_packDataField(dest, position, AR_INT, 1, &attributeType);

	numFields += 2;
	break;
	}
    }
  }

  // All sizes are 8-byte aligned.
  position += ar_fieldOffset(AR_DOUBLE,position);

  // Go back and fill in the first 3 fields.
  ar_packData(dest,                            &position,  AR_INT,1);
  ar_packData(dest + arDataTypeSize(AR_INT),   &recordID,  AR_INT,1);
  ar_packData(dest + arDataTypeSize(AR_INT)*2, &numFields, AR_INT,1);
}

static void ar_unpackDataField(
  const ARchar* src, ARint& position,
  void* data, arDataType type, int length)
{
  position += ar_fieldOffset(type,position);
  ar_unpackData(src+position, data, type, length);
  position += ar_fieldSize(type,length);
}

bool arTemplateDictionary::unpack(ARchar* source,arStreamConfig streamConfig){
  //unused    ARint dictionaryLength = ar_translateInt(source,streamConfig);
  const ARint numFields = ar_translateInt(source+2*AR_INT_SIZE,streamConfig);
  ARint position = 3*AR_INT_SIZE;
  arDataTemplate* theTemplate = NULL;
  int iField = 0;
  while (iField < numFields){

    // The dictionary record alternates name and type fields.
    // The names are either template names or field names.

    // Unpack the name field.
    ARint nameLength = ar_translateInt(source+position,streamConfig);
    position += AR_INT_SIZE;
    ARint nameType = ar_translateInt(source+position,streamConfig);
    position += AR_INT_SIZE;
    if (nameType != AR_CHAR || nameLength >= 1023){
      cout << "arTemplateDictionary unpack error: name type of " 
	   << arDataTypeName((arDataType)nameType) << " or name length of "
	   << nameLength << " are illegal\n";
      return false;
    }

    ARchar nameBuffer[1024];
    ar_unpackDataField(source, position, nameBuffer, AR_CHAR, nameLength);
    nameBuffer[nameLength]='\0';
    string theName(nameBuffer);
      
    // Unpack the type field.
    nameLength = ar_translateInt(source+position,streamConfig);
    position += AR_INT_SIZE;
    nameType = ar_translateInt(source+position,streamConfig);
    position += AR_INT_SIZE;
    if (nameType != AR_INT || nameLength != 1){
      cout << "arTemplateDictionary unpack error: field type of " 
	   << arDataTypeName((arDataType)nameType) << " or field length of "
	   << nameLength << " are illegal.\n";
      return false;
    }
    nameType = ar_translateInt(source+position,streamConfig);
    position += AR_INT_SIZE;

    // We assume that nameType==AR_GARBAGE is the FIRST thing hit
    // in this loop.  Otherwise theTemplate will be NULL when it's ->'d.

    if (nameType==AR_GARBAGE) {
      // Add a new template, because AR_GARBAGE is special.

      // Unpack the template ID.
      const ARint dataLength = ar_translateInt(source+position,streamConfig);
      position += AR_INT_SIZE;
      const ARint dataType = ar_translateInt(source+position,streamConfig);
      position += AR_INT_SIZE;
      if (dataLength!=1 || dataType!=AR_INT){
        cout << "arTemplateDictionary unpack error: illegal field type/length \"" 
	   << arDataTypeName((arDataType)nameType) << "\"/"
	   << nameLength << ".\n";
        return false;
      }
      const ARint templateID = ar_translateInt(source+position,streamConfig);
      position += AR_INT_SIZE;
      theTemplate = new arDataTemplate(theName); // deleted in destructor
      theTemplate->_templateID = templateID;
      _addNoSetID(theTemplate);
      iField += 3;
    }
    else{
      // we add a field to an existing template
      if (theTemplate){
        theTemplate->addAttribute(theName, (arDataType)nameType);
        iField += 2;
      }
      else{
	// Hey!  nameType==AR_GARBAGE wasn't the first case!  Abort.
        cout << "arTemplateDictionary unpack error: field name incorrect.\n";
        return false;
      }
    }
  }
  return true;
}

void arTemplateDictionary::dump() {
  cout << "----arTemplateDictionary: size = " << size() << "\n";
  for (arTemplateType::iterator i(_templateContainer.begin());
       i != _templateContainer.end();
       ++i){
    cout << "name = \"" << i->first << "\"\n";
    i->second->dump(); 
  }
  cout << "-------------------------\n";
}
