//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arLanguage.h"
#include "arLogStream.h"

arTemplateDictionary* arLanguage::getDictionary(){
  return &_dictionary;
}

arStructuredData* arLanguage::makeDataRecord(int id){
  arGuard dummy(_l);
  arDataTemplate* t = _dictionary.find(id);
  if (!t){
    ar_log_error() << "arLanguage failed to make record: no id " << id << ".\n";
    return NULL;
  }
  return new arStructuredData(t);
}

arDataTemplate* arLanguage::find(const char* name){
  arGuard dummy(_l);
  arDataTemplate* t = _dictionary.find(name);
  if (!t) {
    ar_log_error() << "arLanguage: no name '" << name << "'.\n";
  }
  return t;
}

arDataTemplate* arLanguage::find(const string& name){
  arGuard dummy(_l);
  arDataTemplate* t = _dictionary.find(name);
  if (!t) {
    ar_log_error() << "arLanguage: no name '" << name << "'.\n";
  }
  return t;
}

arDataTemplate* arLanguage::find(int id){
  arGuard dummy(_l);
  arDataTemplate* t = _dictionary.find(id);
  if (!t) {
    ar_log_error() << "arLanguage: no id " << id << ".\n";
  }
  return t;
}
