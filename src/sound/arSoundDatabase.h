//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_DATABASE_H
#define AR_SOUND_DATABASE_H

#include "arDatabase.h"
#include "arSoundHeader.h"
#include "arMath.h"
#include "arSoundLanguage.h"
#include "arSoundTransformNode.h"
#include "arSoundFileNode.h"
#include "arSpeakerObject.h"
#include "arPlayerNode.h"
#include "arSpeechNode.h"

#include "arSoundCalling.h"

// Scene graph for sound.

enum { mode_fmod, mode_fmodplugins, mode_vss, mode_mmio };
  // fmod:        thin wrapper around 'gamer' 2-speaker style.
  // fmodplugins: transform sources to compensate for stationary listener
  // vss:         todo, as library, not separate exe
  // mmio:        todo, windows legacy code (fallback if fmod's missing).

class SZG_CALL arSoundDatabase: public arDatabase {
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arSoundDatabase();
  virtual ~arSoundDatabase();

  virtual arDatabaseNode* alter(arStructuredData*, bool refNode=false);
  virtual void reset();

  string getPath() const;
  void setPath(const string& thePath);
  arSoundFile* addFile(const string&, bool);

  void setPlayTransform(arSpeakerObject*);
  bool render();

  int getMode() const { return _renderMode; };
  void setMode(const int m) { _renderMode = m; };

  // Deliberately public, for external data input.
  arStructuredData* transformData;
  arStructuredData* filewavData;
  arStructuredData* playerData;
  arStructuredData* speechData;
  arStructuredData* streamData;

  arSoundLanguage _langSound;

 protected:
  int _renderMode;
  mutable arLock _pathLock; // Guard _path.
  list<string>*  _path;
  map<string, arSoundFile*, less<string> > _filewavNameContainer;

  bool _render(arSoundNode*);
  virtual arDatabaseNode* _makeNode(const string& type);
  arDatabaseNode* _processAdmin(arStructuredData* data);
};

extern stack<arMatrix4, deque<arMatrix4> > ar_transformStack;

#endif
