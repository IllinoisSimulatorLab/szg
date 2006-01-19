//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arLanguage.h"

// TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO TODO
// PLEASE NOTE: not *quite* sure if these are thread-safe or not...
// this bears some investigation! Consequently, there is some additional
// locking occuring.

arLanguage::arLanguage(){
  ar_mutex_init(&_languageLock);
}

arTemplateDictionary* arLanguage::getDictionary(){
  return &_dictionary;
}

arStructuredData* arLanguage::makeDataRecord(int recordID){
  ar_mutex_lock(&_languageLock);
  arDataTemplate* theTemplate = _dictionary.find(recordID);
  if (!theTemplate){
    cout << "arLanguage::makeDataRecord error: record ID not found.\n";
    ar_mutex_unlock(&_languageLock);
    return NULL;
  }
  arStructuredData* result = new arStructuredData(theTemplate);
  ar_mutex_unlock(&_languageLock);
  return result;
}

arDataTemplate* arLanguage::find(const char* name){
  ar_mutex_lock(&_languageLock);
  arDataTemplate* t = getDictionary()->find(name);
  if (!t) {
    cerr << "arLanguage error: dictionary contains no \""
         << name << "\".\n";
  }
  ar_mutex_unlock(&_languageLock);
  return t;
}

arDataTemplate* arLanguage::find(const string& name){
  ar_mutex_lock(&_languageLock);
  arDataTemplate* t = getDictionary()->find(name);
  if (!t) {
    cerr << "arLanguage error: dictionary contains no \""
         << name << "\".\n";
  }
  ar_mutex_unlock(&_languageLock);
  return t;
}

arDataTemplate* arLanguage::find(int id){
  ar_mutex_lock(&_languageLock);
  arDataTemplate* t = getDictionary()->find(id);
  if (!t) {
    cerr << "arLanguage error: dictionary contains no id \""
         << id << "\".\n";
  }
  ar_mutex_unlock(&_languageLock);
  return t;
}
