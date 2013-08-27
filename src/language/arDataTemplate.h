//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATA_TEMPLATE
#define AR_DATA_TEMPLATE

#include "arDataType.h"
#include "arDataUtilities.h"
#include "arLanguageCalling.h"
#include <iostream>
#include <string>
#include <map>
using namespace std;

class arTemplateDictionary;

typedef pair< arDataType, int > arDataPair;

// Something stored by the szgserver.
// This map is to arDataPair, not merely to arDataType,
// so add()'s return value, arDataPair.second,
// does not change as a result of future add()'s.

typedef map< string, arDataPair, less<string> > arAttribute;

// Contains a list of arAttribute objects (string and arDataType).

class SZG_CALL arDataTemplate{
 public:
  friend class arTemplateDictionary;

  arDataTemplate();
  arDataTemplate(const string&, int templateID = -1);
  arDataTemplate(const arDataTemplate&);
  arDataTemplate& operator=( const arDataTemplate& dataTemplate );
  ~arDataTemplate() {}

  int add(const string&, arDataType);
  void addAttribute(const string& s, arDataType d); // backwards-compatible
  void setName(const string&);
  string getName() const
    { return _templateName; }
  int getID() const
    { return _templateID; }
  void setID(int ID)
    { _templateID = ID; }
  int getAttributeID(const string&) const;
  arDataType getAttributeType(const string&) const;
  int getNumberAttributes() const
    { return _numberAttributes; }
  void dump() const;
  int translate(ARchar*, ARchar*, arStreamConfig);

  arAttribute::iterator attributeBegin()
    { return _attributeContainer.begin(); }
  arAttribute::iterator attributeEnd()
    { return _attributeContainer.end(); }
  arAttribute::const_iterator attributeConstBegin() const
    { return _attributeContainer.begin(); }
  arAttribute::const_iterator attributeConstEnd() const
    { return _attributeContainer.end(); }

 private:
  string _templateName;
  int    _templateID; // set by owning dictionary
  int    _numberAttributes;
  arAttribute _attributeContainer;
};

SZG_CALL bool ar_addAttributesFromString( arDataTemplate& t,
  const string& nameString, const string& typeString );

#endif
