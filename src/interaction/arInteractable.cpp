//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arInteractable.h"

#if (defined(__GNUC__)&&(__GNUC__<3))
  #include <algo.h>
#else
  #include <algorithm>
#endif

arInteractable::arInteractable() :
  _enabled( true ),
  _grabEffector( 0 ),
  _useDefaultDrags( true ) {
}

arInteractable::arInteractable( const arInteractable& ui ) :
  _matrix( ui._matrix ),
  _enabled( ui._enabled ),
  _grabEffector( 0 ),
  _useDefaultDrags( ui._useDefaultDrags ),
  _dragManager( ui._dragManager ) {
  _clearActiveDrags();
}

arInteractable& arInteractable::operator=( const arInteractable& ui ) {
  if (&ui == this)
    return *this;
  if (grabbed())
    _ungrab();
  if (touched())
    untouchAll();
  _matrix = ui._matrix;
  _enabled = ui._enabled;
  _useDefaultDrags = ui._useDefaultDrags;
  _dragManager = ui._dragManager;
  _clearActiveDrags();
  return *this;
}

void arInteractable::_cleanup() {
//  cerr << "_cleanup() " << touched() << ".\n";
  if (touched()) {
    untouchAll(); // also ungrab()s
  }
  _touchEffectors.clear();
}

arInteractable::~arInteractable() {
  _cleanup();
}

void arInteractable::useDefaultDrags( bool flag ) {
  if (flag != _useDefaultDrags)
    _clearActiveDrags();
  _useDefaultDrags = flag;
}

void arInteractable::enable( bool flag ) {
  if (!flag) {
    disable();
  } else {
    _enabled = true;
  }
}

void arInteractable::disable() {
  _cleanup();
  _clearActiveDrags();
  _enabled = false;
}

void arInteractable::setDrag( const arGrabCondition& cond,
                              const arDragBehavior& behave ) {
  _dragManager.setDrag( cond, behave );
}

void arInteractable::deleteDrag( const arGrabCondition& cond ) {
  _dragManager.deleteDrag( cond );
}

bool arInteractable::touch( arEffector& effector ) {
  if (!_enabled)
    return false;
  std::vector<arEffector*>::iterator effIter =
    std::find( _touchEffectors.begin(), _touchEffectors.end(), &effector );
  if (effIter != _touchEffectors.end()) {
    // This effector is already touching this interactable. Nothing to do!
    return true;
  }
  // Call the virtual _touch method on this effector.
  const bool ok = _touch( effector );
  if (ok) {
    // The touch succeeded.
    effector.setTouchedObject( this );
    _touchEffectors.push_back( &effector ); // ASSIGNMENT
  }
  return ok;
}

// Main method. Let the
// effector manipulate the interactable. If not
// already touching the object, touch it. If we try to
// touch it, call the virtual _touch method (which, for instance
// in the arCallbackInteractable subclass, ends up calling the touch
// callback).
bool arInteractable::processInteraction( arEffector& effector ) {
  // The interactable cannot be manipulated if it hasn't been enabled.
  if (!_enabled) {
    return false;
  }

  // Try to touch the interactable with the effector.
  if (!touched( effector )) {
    // Not already touching
    if (!touch( effector )) {
      // failed to touch this object
      return false;
    }
  }

  // Handle grabbing. If another effector is grabbing us, go on to
  // the virtual _processInteraction method (as can be changed in subclasses
  // like arCallbackInteractable).
  const bool otherGrabbed = grabbed() && (_grabEffector != &effector);
    // true iff object locked to different effector
  if (!otherGrabbed) {
    // The drag manager is an object that associates grabbing conditions
    // with dragging behaviors. Essentially, this object defines how
    // the interactable will be manipulated. We either use the drag manager
    // associated with the effector OR the drag manager associated with the
    // interactable (as in the case of scene navigation).
    const arDragManager* dm = _useDefaultDrags ?
      effector.getDragManager() : (const arDragManager*)&_dragManager;
    if (!dm) {
      cerr << "arInteractable error: NULL grab manager pointer.\n";
      return false;
    }

    // The main method of arDragManager is getActiveDrags.
    // The drag manager maintains a list of grab-conditions/ drag-behavior
    // pairs. As the grab conditions are met (based on the effector's input
    // state), they get added to the active drag list. As they are no
    // longer are met, they get removed.
    dm->getActiveDrags( &effector, (const arInteractable*)this, _activeDrags );
    if (_activeDrags.empty()) {
     if (grabbed())
        _ungrab();
    } else {
      if (effector.requestGrab( this )) {
        _grabEffector = &effector;  // ASSIGNMENT
      } else {
        cerr << "arInteractable error: lock request failed.\n";
        _ungrab();
      }
      for (arDragMap_t::iterator iter = _activeDrags.begin();
           iter != _activeDrags.end(); iter++) {
        // The drag behavior updates the interactable's matrix using the
        // grab condition.
        iter->second->update( &effector, this, iter->first );
      }
    }
  }
  // Do other event processing. This is defined by a virtual method of the
  // arInteractable so that subclasses can create various sorts of behaviors.
  // For instance, an object might change color when it is grabbed or touched.
  return _processInteraction( effector );
}

bool arInteractable::untouch( arEffector& effector ) {
  if (!touched( effector ))
    return true;
  if (grabbed() == &effector)
    _ungrab();
  effector.setTouchedObject(0);
  std::vector<arEffector*>::iterator iter =
    std::find( _touchEffectors.begin(), _touchEffectors.end(), &effector );
  if (iter != _touchEffectors.end())
    _touchEffectors.erase( iter );  // ASSIGNMENT
  return _untouch( effector );
}

bool arInteractable::untouchAll() {
  bool ok = true;
  while (!_touchEffectors.empty()) {
    if (!untouch( *_touchEffectors.back() ))
      ok = false;
  }
  if (grabbed()) { // shouldn't ever be, after untouches
    cerr << "arInteractable warning: still grabbed after untouchAll().\n";
    _ungrab();
  }
  return ok;
}

// Is this effector currently touching this object?
bool arInteractable::touched( arEffector& effector ) {
  return std::find(_touchEffectors.begin(), _touchEffectors.end(), &effector)
    != _touchEffectors.end();
}

bool arInteractable::touched() const {
  return !_touchEffectors.empty();
}

const arEffector* arInteractable::grabbed() const {
  return _grabEffector;
}

void arInteractable::updateMatrix( const arMatrix4& deltaMatrix ) {
  _matrix = _matrix * deltaMatrix;
}

void arInteractable::_ungrab() {
  if (_grabEffector)
    _grabEffector->requestUngrab( this );
  _grabEffector = 0;  // ASSIGNMENT
}

void arInteractable::_clearActiveDrags() {
  for (arDragMap_t::iterator iter = _activeDrags.begin();
       iter != _activeDrags.end(); ++iter) {
    delete iter->first;
    delete iter->second;
  }
  _activeDrags.clear();
}
