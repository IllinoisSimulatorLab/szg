//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DRAG_MANAGER_H
#define AR_DRAG_MANAGER_H

#include "arGrabCondition.h"
#include "arDragBehavior.h"
#include "arInteractionCalling.h"

#include <map>

class arEffector;
class arInteractable;

typedef std::map< arGrabCondition*, arDragBehavior* > arDragMap_t;

class SZG_CALL arDragManager {
  public:
    arDragManager() {}
    virtual ~arDragManager();
    arDragManager( const arDragManager& dm );
    arDragManager& operator=( const arDragManager& dm );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    void getActiveDrags( arEffector* eff,
                         const arInteractable* const object,
                         arDragMap_t& draggers ) const;
  private:
    void _deleteDrags();
    arDragMap_t _draggers;
};

#endif        //  #ifndefARDRAGMANAGER_H
