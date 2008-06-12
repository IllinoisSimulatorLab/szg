//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arUniqueInteractor.h"

#if (defined(__GNUC__)&&(__GNUC__<3))
  #include <algo.h>
#else
  #include <algorithm>
#endif

using std::list;
using std::vector;

list<arUniqueInteractor*> arUniqueInteractor::_listUs;
vector<int> arUniqueInteractor::_lastButtons;
arUniqueInteractor* arUniqueInteractor::_lockedPtr = 0;
unsigned int arUniqueInteractor::_activeInteractionGroup = 0;

arUniqueInteractor::arUniqueInteractor() :
  _touched( false ),
  _enabled( true ),
  _interactionGroup( 0 ) {
  _listUs.push_back( this );
}

arUniqueInteractor::~arUniqueInteractor() {
  list<arUniqueInteractor*>::iterator iter =
    std::find( _listUs.begin(), _listUs.end(), this );
  if (iter != _listUs.end())
    _listUs.erase( iter );
  if (_listUs.empty())
    _lastButtons.clear();
}

arUniqueInteractor::arUniqueInteractor( const arUniqueInteractor& ui ) :
  _touched( false ),
  _enabled( ui._enabled ),
  _interactionGroup( ui._interactionGroup ) {
  _listUs.push_back( this );
}

arUniqueInteractor& arUniqueInteractor::operator=( const arUniqueInteractor& ui ) {
  if (&ui == this)
    return *this;
  _touched = false;
  _enabled = ui._enabled;
  _interactionGroup = ui._interactionGroup;
  return *this;
}

void arUniqueInteractor::setInteractionGroup( const unsigned int group ) {
  if (_touched) {
    unTouch();
    _touched = false;
  }
  _interactionGroup = group;
}

void arUniqueInteractor::enable() {
  _enabled = true;
}

void arUniqueInteractor::disable() {
  if (_touched) {
    unTouch();
    _touched = false;
  }
  _enabled = false;
}

void arUniqueInteractor::activateInteractionGroup( const unsigned int group ) {
  if (group == _activeInteractionGroup)
    return;

  if (_lockedPtr)
      _lockedPtr = NULL;

  unsigned int numTouched = 0;
  for (list<arUniqueInteractor*>::iterator iter = _listUs.begin();
       iter != _listUs.end(); iter++) {
    if ((*iter)->_touched) {
      (*iter)->unTouch();
      (*iter)->_touched = false;
      numTouched++;
    }
  }
  if (numTouched > 1) {
    cerr << "arUniqueInteractor warning: number items touched > 1.\n";
    return;
  }
  _activeInteractionGroup = group;
}

// Should generally be called once/frame.
// Does the following:
// (1) loop through all instances, find out if one has a lock.  If so, call its
//     processTouch() and return. (an instance might want to set a lock if e.g.
//     it's been grabbed).
// (2) loop through all instances, find out if any was already being touched.
// (3) loop through all instances that are currently accepting interaction
//     to find out which is (a) in the currently-active interaction group (b) closest
//     (or has the highest priority score, if that method's been overridden) and
//     (c) exceeds the minimum priority score.
// (4) if no such instance is found, or it is but it's not the same as the
//     currently-touched instance (if any exists), unTouch() the currently-touched one.
// (5) call processTouch() for the instance found in (3), if any.
//
// @param interfaceObject An arInterfaceObject pointer, for getting input data.
// @param wandTipMatrix Position and orientation of wand tip.
// @param touchDistanceLimit How close does the wand have to be for interaction
// to occur?

bool arUniqueInteractor::processAllTouches(  arInputState* inputState,
                                             const arMatrix4& wandTipMatrix,
                                             const float minPriorityScore ) {
  const unsigned numButtons = inputState->getNumberButtons();
  if (_lastButtons.size() != numButtons) {
    _lastButtons.assign(numButtons, 0);
  }
  int i;
  std::vector<arButtonClickEvent> events;
  for (i=0; i<(int)numButtons; i++) {
    const int button = inputState->getButton(i);
    if (button && !_lastButtons[i]) {
      events.push_back( arButtonClickEvent( i, ON_EVENT ) );
    } else if (!button && _lastButtons[i]) {
      events.push_back( arButtonClickEvent( i, OFF_EVENT ) );
    }
    _lastButtons[i] = button;
  }

  if (_lockedPtr) {
    // See if locked guy still exists
    list<arUniqueInteractor*>::iterator lockedIter =
      std::find( _listUs.begin(), _listUs.end(), _lockedPtr );
    if (lockedIter != _listUs.end()) {
      const bool ok = (*lockedIter)->processTouch( inputState, wandTipMatrix, events );
      events.clear();
      return ok;
    }
    _lockedPtr = NULL;
  }

  unsigned numTouched = 0;
  list<arUniqueInteractor*>::iterator touchedIter = _listUs.end();
  list<arUniqueInteractor*>::iterator iter;
  for (iter = _listUs.begin(); iter != _listUs.end(); iter++) {
    if ((*iter)->_touched) {
      touchedIter = iter;
      numTouched++;
    }
  }
  if (numTouched > 1) {
    cerr << "arUniqueInteractor error: touched more than 1 item.\n";
    events.clear();
    return false;
  }

  float highScore = -1.e-100;
  list<arUniqueInteractor*>::iterator highPriorityIter = _listUs.end();
  for (iter = _listUs.begin(); iter != _listUs.end(); ++iter) {
    if (((*iter)->active()) && ((*iter)->enabled())) {
      const float score = (*iter)->priorityScore( wandTipMatrix );
      if (score >= minPriorityScore &&
         ((highPriorityIter == _listUs.end() || score > highScore))) {
	highScore = score;
	highPriorityIter = iter;
      }
    }
  }
  if (highPriorityIter != touchedIter && touchedIter != _listUs.end()) {
    (*touchedIter)->unTouch();
    (*touchedIter)->_touched = false;
  }
  const bool ok = (highPriorityIter != _listUs.end()) &&
    (*highPriorityIter)->processTouch( inputState, wandTipMatrix, events );
  events.clear();
  return ok;
}

// Partial determinant of which object gets interacted with.
// First _interactionGroup is checked, then this.  priorityScore()
// must exceed minimum specified in arUniqueInteractor::processAllTouches().
// This default priorityScore() returns inverse of Euclidean distance from
// wand tip; to specify a maximum interaction distance, pass the inverse of
// it as the minimum acceptable priority score in processAllTouches().
float arUniqueInteractor::priorityScore( const arMatrix4& wandTipMatrix ) {
  return 1./((
       ar_extractTranslation( wandTipMatrix ) - ar_extractTranslation( _matrix )
       ).magnitude());
}

// This is what an object does if e.g. it's been clicked on and wants to be
// dragged. Ensures that only this instance's processTouch() will be called
// until it unlocks itself. Should always succeed if called from instance's
// processTouch().
bool arUniqueInteractor::lockMe() {
  if (_lockedPtr == this)
    return true;
  if (_lockedPtr)
    return false;
  _lockedPtr = this;
  return true;
}

// E.g. when the user lets go of the button at the end of a drag.
bool arUniqueInteractor::unlockMe() {
  if (!_lockedPtr)
    return true;
  if (_lockedPtr != this)
    return false;
  _lockedPtr = NULL;
  return true;
}
