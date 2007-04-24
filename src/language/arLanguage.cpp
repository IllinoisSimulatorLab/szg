//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arLanguage.h"
#include "arLogStream.h"

// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
// PLEASE NOTE: not *quite* sure if these are thread-safe or not...
// this bears some investigation! Consequently, there is some additional
// locking occuring.

arTemplateDictionary* arLanguage::getDictionary(){
  return &_dictionary;
}

arStructuredData* arLanguage::makeDataRecord(int id){
  _lock();
  arDataTemplate* t = _dictionary.find(id);
  if (!t){
    ar_log_warning() << "arLanguage: failed to make record: no id " << id << ".\n";
    _unlock();
    return NULL;
  }
  arStructuredData* result = new arStructuredData(t);
  _unlock();
  return result;
}

arDataTemplate* arLanguage::find(const char* name){
  _lock();
  arDataTemplate* t = _dictionary.find(name);
  if (!t) {
    ar_log_warning() << "arLanguage: no name '" << name << "'.\n";
  }
  _unlock();
  return t;
}

arDataTemplate* arLanguage::find(const string& name){
  _lock();
  arDataTemplate* t = _dictionary.find(name);
  if (!t) {
    ar_log_warning() << "arLanguage: no name '" << name << "'.\n";
  }
  _unlock();
  return t;
}

arDataTemplate* arLanguage::find(int id){
  _lock();
  arDataTemplate* t = _dictionary.find(id);
  if (!t) {
    ar_log_warning() << "arLanguage: no id " << id << ".\n";
  }
  _unlock();
  return t;
}
