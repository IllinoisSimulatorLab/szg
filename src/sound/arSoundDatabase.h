//********************************************************
// Syzygy source code is licensed under the GNU LGPL
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

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arSoundCalling.h"

/// Scene graph for sound.

class SZG_CALL arSoundDatabase: public arDatabase{
 // Needs assignment operator and copy constructor, for pointer members.
 public:
  arSoundDatabase();
  virtual ~arSoundDatabase();

  virtual arDatabaseNode* alter(arStructuredData*);
  virtual void reset();

  string getPath();
  void setPath(const string& thePath);
  arSoundFile* addFile(const string&, bool);

  void setPlayTransform(arSpeakerObject*);
  void render();

  // Deliberately public, for external data input.
  arStructuredData* transformData;
  arStructuredData* filewavData;
  arStructuredData* playerData;
  arStructuredData* speechData;
  arStructuredData* streamData;

  arSoundLanguage _langSound;  

 protected:
  arMutex        _pathLock;
  list<string>*  _path;
  map<string,arSoundFile*,less<string> > _filewavNameContainer;

  void _render(arSoundNode*);
  virtual arDatabaseNode* _makeNode(const string& type);
  arDatabaseNode* _processAdmin(arStructuredData* data);
};

extern stack<arMatrix4, deque<arMatrix4> > ar_transformStack;

#endif
