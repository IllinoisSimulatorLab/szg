//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARINTERACTABLE_H
#define ARINTERACTABLE_H

#include "arMath.h"
#include "arEffector.h"
#include "arGrabCondition.h"
#include "arDragBehavior.h"
#include "arDragManager.h"
#include <vector>
// THIS MUST BE THE LAST SZG INCLUDE!
#include "arInteractionCalling.h"

class SZG_CALL arInteractable {
  public:
    arInteractable();
    virtual ~arInteractable();
    arInteractable( const arInteractable& ui );
    arInteractable& operator=( const arInteractable& ui );
    
    virtual bool touch( arEffector& effector );
    virtual bool processInteraction( arEffector& effector );
    virtual bool untouch( arEffector& effector );
    virtual bool untouchAll();
    
    /// Disallow user interaction
    void disable();
    /// Allow user interaction
    void enable( bool flag=true );
    bool enabled() const { return _enabled; }
    void useDefaultDrags( bool flag );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    void setDragManager( const arDragManager& dm ) { _dragManager = dm; }
    bool touched() const;
    bool touched( arEffector& effector );
    const arEffector* grabbed() const;
    virtual void setMatrix( const arMatrix4& matrix ) { _matrix = matrix; }
    arMatrix4 getMatrix() const { return _matrix; }
    virtual void updateMatrix( const arMatrix4& deltaMatrix );
    
  protected:
    /// Sub-class' event (got-focus) handler.    
    /// Called only for the particular instance that satisfies the criteria
    /// implemented in the static method processAllTouches() (if any).
    /// @param interface An arInterfaceObject pointer, for getting
    /// miscellaneous input.
    /// @param wandTipMatrix Wand tip position & orientation.
    /// @param events A vector of button on/off events.
    virtual bool _processInteraction( arEffector& ) { return true; }
    
    /// Sub-class' gain-of-focus handler.    
    virtual bool _touch( arEffector& ) { return true; }
    /// Sub-class' loss-of-focus handler.    
    /// Called if _touched is true but this instance didn't get the focus.
    virtual bool _untouch( arEffector& ) { return true; }
    void _ungrab();
    void _clearActiveDrags();
    virtual void _cleanup();
    /// Represents position and orientation of object.
    arMatrix4 _matrix;
    /// accepting interaction?
    bool _enabled;
    arEffector* _grabEffector;
    std::vector< arEffector* > _touchEffectors;
    bool _useDefaultDrags;
    arDragManager _dragManager;
    arDragMap_t _activeDrags;
};

#endif        //  #ifndefARINTERACTABLE_H
