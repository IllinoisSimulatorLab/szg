//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
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
  // should it do this? Might be better to retain this stuff.
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
  if (touched())
    untouchAll(); // also ungrab()s
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
  std::vector<arEffector*>::iterator effIter;
  effIter 
    = std::find( _touchEffectors.begin(), _touchEffectors.end(), &effector );
  bool state = effIter != _touchEffectors.end();
  if (state){
    // This effector is already touching this interactable. Nothing to do!
    return true;
  }
  // Call the virtual _touch method on this effector.
  state = _touch( effector );
  if (state) {
    // The touch has, indeed, succeeded.
    effector.setTouchedObject( this );
    _touchEffectors.push_back( &effector );
  }
  // If the virtual _touch method returned false, then we return that.
  // If it succeeded, we'll return true.
  return state;
}

/// This is the most important method of arInteractable. We allow the 
/// effector to manipulate the interactable. First of all, if we aren't
/// already touching the object, go ahead and touch it. If we attempt to
/// touch it, this will call the virtual _touch method (which, for instance
/// in the arCallbackInteractable subclass, ends up calling the touch
/// callback).
bool arInteractable::processInteraction( arEffector& effector ) {
  // The interactable cannot be manipulated if it hasn't been enabled.
  if (!_enabled){
    return false;
  }

  // Attempt to touch the interactable with the effector.
  if (!touched( effector )){ 
    // Not already touching
    if (!touch( effector )){ 
      // failed to touch this object
      return false;
    }
  }

  // Handle grabbing. If another effector is grabbing us, go on to
  // the virtual _processInteraction method (as can be changed in subclasses
  // like arCallbackInteractable).
  bool otherGrabbed = false;
  if (grabbed() && (_grabEffector != &effector)){ 
    // object locked to different effector
    otherGrabbed = true;
  }
  if (!otherGrabbed) {
    const arDragManager* dm;
    // The drag manager is an object that associates grabbing conditions
    // with dragging behaviors. Essentially, this object defines how
    // the interactable will be manipulated. We either use the drag manager
    // associated with the effector OR the drag manager associated with the
    // interactable (as in the case of scene navigation).
    if (_useDefaultDrags){
      dm = effector.getDragManager();
    }
    else{
      dm = (const arDragManager*)&_dragManager;
    }
    if (dm == 0) {
      cerr << "arInteractable error: NULL grab manager pointer.\n";
      return false;
    }
    // The most important method of arDragManager is getActiveDrags.
    // The drag manager maintains a list of grab-conditions/ drag-behavior
    // pairs. As the grab conditions are met (based on the effector's input
    // state), they get added to the active drag list. As they are no
    // longer are met, they get removed. 
    dm->getActiveDrags( &effector, (const arInteractable*)this, _activeDrags );
    if (_activeDrags.empty()) {
     if (grabbed())
        _ungrab();
    } else {
      if (!effector.requestGrab( this )) {
        cerr << "arInteractable error: lock request failed.\n";
        _ungrab();
      } else {
        _grabEffector = &effector;
      }
      arDragMap_t::iterator iter;
      for (iter = _activeDrags.begin(); iter != _activeDrags.end(); iter++) {
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
  std::vector<arEffector*>::iterator iter;
  iter = std::find( _touchEffectors.begin(), 
                    _touchEffectors.end(), 
                    &effector );
  if (iter != _touchEffectors.end())
    _touchEffectors.erase( iter );
  bool stat = _untouch( effector );
  return stat;
}

bool arInteractable::untouchAll() {
  bool stat(true);
  while (!_touchEffectors.empty()) {
    arEffector* eff = _touchEffectors.back();
    if (!untouch( *eff ))
      stat = false;
  }
  if (grabbed()) { // shouldn't ever be
    cerr << "arInteractable warning: still grabbed after untouchAll().\n";
    _ungrab();
  }
  return stat;
}    

/// Tells us whether or not this effector is currently touching this object.
bool arInteractable::touched( arEffector& effector ) {
  std::vector<arEffector*>::iterator iter;
  iter = std::find( _touchEffectors.begin(), 
                    _touchEffectors.end(), 
                    &effector );
  return iter != _touchEffectors.end();
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
  if (_grabEffector != 0)
    _grabEffector->requestUngrab( this );
  _grabEffector = 0;
}

void arInteractable::_clearActiveDrags() {
  arDragMap_t::iterator iter;
  for (iter = _activeDrags.begin(); iter != _activeDrags.end(); iter++) {
    delete iter->first;
    delete iter->second;
  }
  _activeDrags.clear();
}
