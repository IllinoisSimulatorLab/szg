//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arDragManager.h"
#include "arEffector.h"
#include <vector>
#if (defined(__GNUC__)&&(__GNUC__<3))
#include <algo.h>
#else
#include <algorithm>
#endif

arDragManager::~arDragManager() {
  _deleteDrags();
}

arDragManager::arDragManager( const arDragManager& getActiveGrabber ) {
  arDragMap_t::const_iterator iter;
  for (iter = getActiveGrabber._draggers.begin(); 
       iter != getActiveGrabber._draggers.end(); iter++) {
    arGrabCondition* cond = iter->first;
    arDragBehavior* behave = iter->second;
    setDrag( *cond, *behave );
  }
}

arDragManager& arDragManager::operator=
  ( const arDragManager& getActiveGrabber ){
  if (&getActiveGrabber == this)
    return *this;
  _deleteDrags();
  arDragMap_t::const_iterator iter;
  for (iter = getActiveGrabber._draggers.begin(); 
       iter != getActiveGrabber._draggers.end(); iter++) {
    arGrabCondition* cond = iter->first;
    arDragBehavior* behave = iter->second;
    setDrag( *cond, *behave );
  }
  return *this;
}

void arDragManager::setDrag( const arGrabCondition& cond,
                             const arDragBehavior& behave ) {
  arDragMap_t::iterator iter;
  for (iter = _draggers.begin(); iter != _draggers.end(); iter++) {
    if (*(iter->first) == cond) {
      if (iter->second != 0)
        delete iter->second;
      iter->second = behave.copy();
      return;
    }
  }
  _draggers.insert( std::make_pair( cond.copy(), behave.copy() ) );
}

void arDragManager::deleteDrag( const arGrabCondition& cond ) {
  arDragMap_t::iterator iter;
  for (iter = _draggers.begin(); iter != _draggers.end(); iter++) {
    if (*(iter->first) == cond) {
      if (iter->first != 0)
        delete iter->first;
      if (iter->second != 0)
        delete iter->second;
      _draggers.erase( iter );
      return;
    }
  }
}

/// This is the most important method of arDragManager. It modifies, in
/// place, a list of activated drags. Remember: the drag manager essentially
/// stores an association between grab conditions and drag behaviors.
/// As grab conditions are met (according to the effector since the effector
/// can remap input events), they are added to the list. Similarly, as
/// grab conditions fail to be met, they are removed from the list.
/// NOTE: examples of grab conditions include button presses and pushing
/// a joystick axis above or below a threshold value.
void arDragManager::getActiveDrags( arEffector* eff,
                                    const arInteractable* const object,
                                    arDragMap_t& draggers ) const {
  // Remove drags whose condition (according to arGrabCondition::check(..))
  // is no longer met.
  if (!draggers.empty()) {
    std::vector< arDragMap_t::iterator > deletions;
    arDragMap_t::iterator iter2;
    for (iter2 = draggers.begin(); iter2 != draggers.end(); iter2++) {
      arGrabCondition* gc = iter2->first;
      if (!gc->check( eff )) {
        delete iter2->first;
        delete iter2->second;
        deletions.push_back( iter2 );
      }
    }
    // Delete from the passed list.
    std::vector< arDragMap_t::iterator >::iterator delIter;
    for (delIter = deletions.begin(); delIter != deletions.end(); delIter++)
      draggers.erase( *delIter );
  }
   
  // We have an internal list of grab-conditions/drag-behaviors. Check to
  // see if any one these have been newly activated. If so, go ahead
  // and and them to the passed list.
  if (!_draggers.empty()) {
    arDragMap_t::const_iterator iter;
    for (iter = _draggers.begin(); iter != _draggers.end(); iter++) {
      arGrabCondition* gc = iter->first;
      if (gc->check( eff )) {
        arDragMap_t::iterator iter3;
	// Make sure we don't double up on the grab-conditions/drag-behaviors.
        for (iter3 = draggers.begin(); iter3 != draggers.end(); iter3++) {
          if (*(iter3->first) == *gc)
            goto FoundIt;
        }
        arDragBehavior* db = iter->second->copy();
        db->init( eff, object );
        draggers.insert( std::make_pair( gc->copy(), db ) );
      }
FoundIt: ;
    }
  }
}

void arDragManager::_deleteDrags() {
  arDragMap_t::iterator iter;
  for (iter = _draggers.begin(); iter != _draggers.end(); iter++) {
    arGrabCondition* cond = iter->first;
    arDragBehavior* behave = iter->second;
    if (cond != 0)
      delete cond;
    if (behave != 0)
      delete behave;
  }
  _draggers.clear();
}

