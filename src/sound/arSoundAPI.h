//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_SOUND_API_H
#define AR_SOUND_API_H

#include "arSoundDatabase.h"
#include "arSoundCalling.h"

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

//**** some commands create/modify nodes, others only modify

//**** node creation commands return -1 on error and otherwise the node ID.
//**** node modification commands return false on error.

SZG_CALL void dsSetSoundDatabase(arSoundDatabase*);

SZG_CALL arDatabaseNode* dsMakeNode(const string&, const string&,
                                    const string&);

SZG_CALL bool dsPlayer(const arMatrix4&, const arVector3&, float);

SZG_CALL int dsTransform(const string&, const string&, const arMatrix4&);
SZG_CALL bool dsTransform(int, const arMatrix4&);

SZG_CALL int dsLoop(const string&, const string&, const string&,
                    int, float, const arVector3&);
SZG_CALL int dsLoop(const string&, const string&, const string&,
                    int, float, const float*);
SZG_CALL bool dsLoop(int, const string&, int, float, const arVector3&);
SZG_CALL bool dsLoop(int, const string&, int, float, const float*);

SZG_CALL int dsSpeak( const string& name, const string& parent,
                      const string& text );
SZG_CALL bool dsSpeak( int ID, const string& text );

SZG_CALL int dsStream( const string& name, const string& parent,
                       const string& fileName,
                       int paused, float amplitude, int time);
SZG_CALL bool dsStream(int ID, const string& fileName, int paused,
                       float amplitude, int time);

SZG_CALL bool dsErase(const string&);

#endif
