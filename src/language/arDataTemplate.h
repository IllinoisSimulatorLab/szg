//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DATA_TEMPLATE
#define AR_DATA_TEMPLATE

#include "arDataType.h"
#include "arDataUtilities.h"
#include <iostream>
#include <string>
#include <map>
using namespace std;

class arTemplateDictionary;

//;;;; doxygen fails
/// This map is from string to arDataPair, not merely to arDataType,
/// so that add()'s return value (which is arDataPair.second)
/// does not change as a result of future add()'s.

typedef pair< arDataType, int > arDataPair;

//;;;; doxygen fails
/// Something stored by the szgserver.

typedef map< string,arDataPair,less<string> > arAttribute;
typedef arAttribute::iterator arAttributeIterator;

/// Contains a list of arAttribute objects (string and arDataType).

class SZG_CALL arDataTemplate{
 public:
   friend class arTemplateDictionary;

   arDataTemplate();
   arDataTemplate(const string&, int templateID = -1);
   arDataTemplate(const arDataTemplate&);
   arDataTemplate& operator=( const arDataTemplate& dataTemplate );
   ~arDataTemplate(){}

   int add(const string&, arDataType);
   void addAttribute(const string& s, arDataType d); // backwards-compatible
   void setName(const string&);
   string getName() const
     { return _templateName; }
   int getID() const
     { return _templateID; }
   void setID(int ID){ _templateID = ID; }
   int getAttributeID(const string&);
   arDataType getAttributeType(const string&);
   int getNumberAttributes() const
     { return _numberAttributes; }
   void dump();
   int translate(ARchar*,ARchar*,arStreamConfig);

   arAttributeIterator attributeBegin()
     { return _attributeContainer.begin(); }
   arAttributeIterator attributeEnd()
     { return _attributeContainer.end(); }
   
 private:
   string _templateName;
   int    _templateID;                 // will be set by the owning dictionary
   int    _numberAttributes;
   arAttribute _attributeContainer;
};  

bool ar_addAttributesFromString( arDataTemplate& t,
                                 string nameString, string typeString );

#endif
