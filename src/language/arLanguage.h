//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_LANGUAGE_H
#define AR_LANGUAGE_H

class arStructuredData;
#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arLanguageCalling.h"

/// Generic language.

class SZG_CALL arLanguage{
 public:
  arLanguage();
  virtual ~arLanguage(){}

  arTemplateDictionary* getDictionary();
  arStructuredData* makeDataRecord(int);
  arDataTemplate* find(const char* name);
  arDataTemplate* find(const string& name);
  arDataTemplate* find(int);
 protected:
  arTemplateDictionary _dictionary;
  arMutex              _languageLock;
};

#endif
