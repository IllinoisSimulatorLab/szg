//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_API_H
#define AR_SOUND_API_H

#include "arSoundDatabase.h"

//**** one thing that'd be very nice.... don't force the user
//**** to fill-in an ID array that is the identity function...
//**** instead deal with this automatically
//**** of course, sometimes we *do* want the ID array...

//**** some thoughts about naming conventions....
//**** arXXX refers to classes in the library
//**** important executables... like the render client...
//**** should have names starting with SZG
//**** global functions... like those below... should start with a 
//**** unique two char identifier
//**** dg = distributed graphics
//**** ds = distributed sound

//**** note that some commands create/modify nodes...
//**** while some commands only modify nodes

//**** node creation commands return -1 on error and otherwise the node ID
//**** node modification commands return false on error and true otherwise

void dsSetSoundDatabase(arSoundDatabase*);

arDatabaseNode* dsMakeNode(const string&, const string&, const string&);

int dsPlayer(const arMatrix4&, const arVector3&, float);

int dsTransform(const string&, const string&, const arMatrix4&);
bool dsTransform(int, const arMatrix4&);

int dsLoop(const string&, const string&, const string&, int, float, const arVector3&);
int dsLoop(const string&, const string&, const string&, int, float, const float*);
bool dsLoop(int, const string&, int, float, const arVector3&);
bool dsLoop(int, const string&, int, float, const float*);

int dsSpeak( const string& name, const string& parent, const string& text );
bool dsSpeak( int ID, const string& text );

int dsStream( const string& name, const string& parent, const string& fileName,
              int paused, float amplitude, int time);
bool dsStream(int ID, const string& fileName, int paused, float amplitude,
              int time);

bool dsErase(const string&);

#endif
