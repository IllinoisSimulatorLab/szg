//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_TEMPLATE_DICTIONARY
#define AR_TEMPLATE_DICTIONARY

#include "arDataTemplate.h"
#include "arDataUtilities.h"
#include "arLanguageCalling.h"
#include <map>
#include <string>
#include <iostream>
using namespace std;

typedef map<string, arDataTemplate*, less<string> > arTemplateType;

// Collection of arDataTemplate objects.

class SZG_CALL arTemplateDictionary{
 public:
   arTemplateDictionary();
   // for the common case of adding a single template
   arTemplateDictionary(arDataTemplate*);
   ~arTemplateDictionary();

   int add(arDataTemplate*); // Returns the ID which gets assigned to it.
   arDataTemplate* find(const string&);
   arDataTemplate* find(int);
   void renumber();

   // Read the template list.
   int numberTemplates() const
     { return _templateContainer.size(); }
   arTemplateType::iterator begin()
     { return _templateContainer.begin(); }
   arTemplateType::iterator end()
     { return _templateContainer.end(); }

   // Byte-stream representation.
   // AARGH! THESE ARE BROKEN WITH RESPECT TO RENUMBERING!
   // FORTUNATELY, THAT ONLY OCCURS IN arMasterSlaveDataRouter...
   int size() const;
   void pack(ARchar*) const;
   bool unpack(ARchar*, arStreamConfig);
   bool unpack(ARchar* buf)
     { return unpack(buf, ar_getLocalStreamConfig()); };
   void dump();

 private:
   arTemplateType _templateContainer;

   typedef map<string, bool, less<string> > arOwnerType;
   arOwnerType _ownershipContainer;

   typedef map<int, arDataTemplate*, less<int> > arTemplat2Type;
   arTemplat2Type _templateIDContainer;
   int _numberTemplates;
   bool _addNoSetID(arDataTemplate*);
};

#endif
