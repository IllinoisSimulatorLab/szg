//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arInteractionUtilities.h"
#if (defined(__GNUC__)&&(__GNUC__<3))
#include <algo.h>
#else
#include <algorithm>
#endif
#ifdef AR_USE_WIN_32
#include <float.h>
const float BIGGEST_FLOAT = FLT_MAX;
#endif
#ifdef AR_USE_DARWIN
#define MAXFLOAT ((float)3.40282346638528860e+38)
const float BIGGEST_FLOAT = MAXFLOAT; 
#endif
#ifdef AR_USE_LINUX
#include <values.h> // for MAXFLOAT
const float BIGGEST_FLOAT = MAXFLOAT;
#endif
#ifdef AR_USE_SGI
#include <values.h> // for MAXFLOAT         
const float BIGGEST_FLOAT = MAXFLOAT;
#endif

/// The main method via which we interact with objects. The effector 
/// represents an interaction device that can touch and grab interactables.
/// First, we see if the effector has grabbed something in the list.
/// If so, go ahead and manipulate that interactable with this effector
/// and return. Otherwise, go on and determine the "closest" object
/// to the effector. This is the "touched" object. If it is different than
/// the previously touched object (as understood by the effector), untouch
/// the previous object. In any event, go ahead and call the
/// interactable's processInteraction method (where state will be updated,
/// etc.).
bool ar_pollingInteraction( arEffector& effector,
                            std::list<arInteractable*>& objects ) {
  // Interact with the grabbed object, if any.
  const arInteractable* grabbedPtr = effector.getGrabbedObject();
  std::list<arInteractable*>::iterator grabbedIter;
  if (grabbedPtr != 0) {
    // If this effector has grabbed an object not in this set, don't
    // interact with any of these
    grabbedIter = std::find( objects.begin(), objects.end(), grabbedPtr );
    if (grabbedIter == objects.end()) {
      return false; // not an error, just means no interaction occurred
    }
    if ((*grabbedIter)->enabled()) {
      // If it's grabbed an object in this set, interact only with it.
      return (*grabbedIter)->processInteraction( effector );
    } 
    else{
      // Ungrab it. HOW STRANGE! THIS DOESN'T DO ANYTHING!
    }
  }
  // Figure out the closest interactable to the effector (as determined
  // by their matrices). Go ahead and touch it (while untouching the
  // previously touched object if such are different).
  std::list<arInteractable*>::iterator iter;
  std::list<arInteractable*>::iterator touchedIter = objects.end();
  float minDist = BIGGEST_FLOAT;
  for (iter = objects.begin(); iter != objects.end(); iter++) {
    if ((*iter)->enabled()) {
      float dist = effector.calcDistance( (*iter)->getMatrix() );
      if ((dist >= 0.)&&(dist < minDist)) {
        minDist = dist;
        touchedIter = iter;
      }
    }
  }  
  arInteractable* touchedPtr = effector.getTouchedObject();
  if (*touchedIter != touchedPtr) {
    if (touchedPtr != 0)
      touchedPtr->untouch( effector );
  }
  if (touchedIter == objects.end()){ 
    // Not touching any objects.
    return false;
  }
  if (*touchedIter == 0) {
    cerr << "ar_pollingInteraction error: found NULL touched pointer.\n";
    return false;
  }
  // Finally, and most importantly, process the action of the effector on
  // the interactable.
  return (*touchedIter)->processInteraction( effector );
}

bool ar_pollingInteraction( arEffector& effector,
                            arInteractable* object ) {
  if (object == 0) {
    cerr << "ar_pollingInteraction error: NULL object pointer passed.\n";
    return false;
  }
  const arInteractable* grabbedPtr = effector.getGrabbedObject();
  if (grabbedPtr == object) {
    if (object->enabled()) {
      return object->processInteraction( effector );
    } else {
      // ungrab it
    }
  }
  float minDist = BIGGEST_FLOAT;
  float dist = effector.calcDistance( object->getMatrix() );
  arInteractable* touchedPtr = effector.getTouchedObject();
  if ((dist < 0.)||(dist >= minDist)) {
    if (object == touchedPtr)
      touchedPtr->untouch( effector );
    return false;
  }
  if (object != touchedPtr) {
    if (touchedPtr != 0)
      touchedPtr->untouch( effector );
  }
  return object->processInteraction( effector );
}


