//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arDataTemplate.h"

#include <vector>
using namespace std;

arDataTemplate::arDataTemplate():
  _templateName("NULL"),
  _templateID(-1),
  _numberAttributes(0){
}

arDataTemplate::arDataTemplate(const string& name, int templateID):
  _templateName(name),
  _templateID(templateID),
  _numberAttributes(0)
{
}

// Sometimes we need to be able to make copies of a data template.
arDataTemplate::arDataTemplate(const arDataTemplate& dataTemplate){
  _templateName = dataTemplate._templateName;
  _templateID = dataTemplate._templateID;
  _numberAttributes = dataTemplate._numberAttributes;
  arAttribute::const_iterator i;
  for (i = dataTemplate._attributeContainer.begin();
       i != dataTemplate._attributeContainer.end();
       i++){
    _attributeContainer.insert(arAttribute::value_type(i->first,
						       i->second));
  }
}

arDataTemplate& arDataTemplate::operator=( const arDataTemplate& dataTemplate ) {
  if (&dataTemplate == this) {
    return *this;
  }
  _templateName = dataTemplate._templateName;
  _templateID = dataTemplate._templateID;
  _numberAttributes = dataTemplate._numberAttributes;
  _attributeContainer.clear();
  arAttribute::const_iterator i;
  for (i = dataTemplate._attributeContainer.begin();
       i != dataTemplate._attributeContainer.end();
       ++i) {
    _attributeContainer.insert(arAttribute::value_type(i->first,
						       i->second));
  }
  return *this;
}

int arDataTemplate::add(const string& attributeName, arDataType attributeType){
  _attributeContainer.insert( 
    arAttribute::value_type
      (attributeName, make_pair(attributeType, _numberAttributes)));
  ++_numberAttributes;

  if (getAttributeID(attributeName) != _numberAttributes-1)
    cerr << "\n\narDataTemplate internal error while adding "
         << attributeName
         << ",\npossibly because this attribute name "
         << "is duplicated in the language.\n";

  return _numberAttributes-1;
}

void arDataTemplate::addAttribute(const string& s, arDataType d){
  (void)add(s, d);
}

void arDataTemplate::setName(const string& name){
  _templateName = name;
}

int arDataTemplate::getAttributeID(const string& attributeName) const {
  for (arAttribute::const_iterator iter(_attributeContainer.begin());
       iter != _attributeContainer.end(); ++iter){
    if (iter->first == attributeName)
      return iter->second.second;
      // This is the value of _numberAttributes when this field was added.
    }
  return -1;
}

arDataType arDataTemplate::getAttributeType(const string& attributeName) const {
  for (arAttribute::const_iterator iter(_attributeContainer.begin());
       iter != _attributeContainer.end(); ++iter){
    if (iter->first == attributeName)
      return iter->second.first;
      // This is the value of _numberAttributes when this field was added.
    }
  return AR_GARBAGE;
}

void arDataTemplate::dump() const {
  cout << "arDataTemplate: \"" << _templateName << "\"\n";
  for (arAttribute::const_iterator iter(_attributeContainer.begin());
       iter != _attributeContainer.end(); ++iter){
    cout << "  \"" << iter->first << "\":" << iter->second.second << ":"
	 << arDataTypeName(iter->second.first) << "\n";
  }
}

// Return the number of translated bytes written into memory, or -1 on failure.
// Ugly: this does some data formatting, and in a second location vis-a-vis arStructuredData.

int arDataTemplate::translate(ARchar* dest, ARchar* src, 
                              arStreamConfig streamConfig){
  // This doesn't use arDataTemplate yet, because the wire
  // data format includes so much info about the data
  // that arDataTemplate is almost redundant.

  ARint positionDest; // = 0; // Irix compiler produces bad code with "= 0" here. 
  ARint positionSrc;  // = 0; // Irix compiler produces bad code with "= 0" here. 
  positionDest = 0;
  positionSrc = 0;

  // Translate the header.
  const ARint size = ar_translateInt(dest,positionDest,
                                     src,positionSrc,streamConfig);
  (void) /*const ARint ID = */ ar_translateInt(dest,positionDest,
                                               src,positionSrc,streamConfig);
  const ARint numberFields = 
    ar_translateInt(dest,positionDest,src,positionSrc,streamConfig);

  // Translate each field.
  ARint iField = 0;
  for (iField=0; positionSrc<size && iField<numberFields; ++iField){
    const ARint dim =
      ar_translateInt(dest,positionDest,src,positionSrc,streamConfig);
    const ARint type = 
      ar_translateInt(dest,positionDest,src,positionSrc,streamConfig);
    ar_translateField(dest,positionDest,src,positionSrc,(arDataType)type,
		      dim,streamConfig);
  }
  if (iField != numberFields)
    return -1;

  // Records are 8-byte-aligned.
  return positionDest + ar_fieldOffset(AR_DOUBLE, positionDest);
}

bool ar_addAttributesFromString( arDataTemplate& t,
    const string& nameString, const string& typeString ) {
  std::vector<std::string> names;
  if (!ar_getTokenList( nameString, names, '|' )) { // vertical bar, not slash
    cerr << "ar_addAttributesFromString error: failed to parse name string.\n";
    return false;
  }

  std::vector<std::string> types;
  if (!ar_getTokenList( typeString, types, '|' )) { // vertical bar, not slash
    cerr << "ar_addAttributesFromString error: failed to parse type string.\n";
    return false;
  }

  if (names.size() != types.size()) {
    cerr << "ar_addAttributesFromString error: different field count in name string '"
         << nameString << "'\n" << "  and type string '" << typeString << "'.\n";
    return false;
  }

  for (unsigned int i=0; i<names.size(); i++) {
    arDataType dataType = arDataNameType( types[i].c_str() );
    t.addAttribute( names[i], dataType );
  }
  return true;
}
