//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

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
#if defined(AR_USE_SGI) || defined(AR_USE_LINUX)
  #include <values.h> // for MAXFLOAT
  const float BIGGEST_FLOAT = MAXFLOAT;
#endif

// The main method via which we interact with objects. The effector
// represents an interaction device that can touch and grab interactables.
// If the effector has grabbed something in the list,
// manipulate that interactable with this effector
// and return. Otherwise, determine the "closest" object
// to the effector. This is the "touched" object. If it differs from
// the previously touched object (as understood by the effector), untouch
// the previous object. In any event, call the
// interactable's processInteraction() to update state, etc.
bool ar_pollingInteraction( arEffector& effector,
                            std::list<arInteractable*>& objects ) {
  // Interact with the grabbed object, if any.
  const arInteractable* grabbedPtr = effector.getGrabbedObject();
  std::list<arInteractable*>::iterator grabbedIter;
  if (grabbedPtr) {
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
  }
  // Figure out the closest interactable to the effector (as determined
  // by their matrices). Touch it, while untouching the
  // previously touched object if different.
  std::list<arInteractable*>::iterator touchedIter = objects.end();
  float minDist = BIGGEST_FLOAT;
  for (std::list<arInteractable*>::iterator iter = objects.begin();
       iter != objects.end(); ++iter) {
    if ((*iter)->enabled()) {
      const float dist = effector.calcDistance( (*iter)->getMatrix() );
      if ((dist >= 0.)&&(dist < minDist)) {
        minDist = dist;
        touchedIter = iter;
      }
    }
  }
  arInteractable* touchedPtr = effector.getTouchedObject();
  if (touchedPtr && touchedPtr != *touchedIter) {
    touchedPtr->untouch( effector );
  }
  if (touchedIter == objects.end()) {
    // Not touching any objects.
    return false;
  }

  if (!*touchedIter) {
    cerr << "ar_pollingInteraction error: found NULL touched pointer.\n";
    return false;
  }

  // Let the effector act on the interactable.
  return (*touchedIter)->processInteraction( effector );
}

bool ar_pollingInteraction( arEffector& effector,
                            arInteractable* object ) {
  if (object == 0) {
    cerr << "ar_pollingInteraction error: got NULL object.\n";
    return false;
  }

  const arInteractable* grabbedPtr = effector.getGrabbedObject();
  if (grabbedPtr == object) {
    if (object->enabled())
      return object->processInteraction( effector );
    // ungrab it
  }
  const float dist = effector.calcDistance( object->getMatrix() );
  arInteractable* touchedPtr = effector.getTouchedObject();
  if (dist < 0.) {
    if (object == touchedPtr)
      touchedPtr->untouch( effector );
    return false;
  }

  if (touchedPtr && touchedPtr != object)
    touchedPtr->untouch( effector );
  return object->processInteraction( effector );
}
