//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARUNIQUEINTERACTOR_H
#define ARUNIQUEINTERACTOR_H

#include "arMath.h"
#include "arInputState.h"
#include "arInteractionCalling.h"

#include <list>
#include <vector>

enum buttonevent_t { NO_EVENT=0, ON_EVENT, OFF_EVENT };

// A press, release, or no-op of a button.

class SZG_CALL arButtonClickEvent {
  public:
    int _number;
    buttonevent_t _type;
    arButtonClickEvent( const int num, const buttonevent_t t ) :
      _number(num), _type(t) {}
    arButtonClickEvent( const arButtonClickEvent& b ) :
      _number(b._number), _type(b._type) {}
};

/*
An abstract class, subclassed by render objects that need to be
interacted with. It's a 'unique' interactor in that it's assumed that
the subject or user will only be interacting with one object at a time.
The class maintains a static list of members, to remember of who should
be interacted with. Who gets the 'focus' is determined by two things:
which objects' _interactionGroup (an unsigned int) matches the static
member _activeInteractionGroup (this allows for interaction with one
group of objects at a time), zero means an object isn't accepting
interactions) and the priorityScore() method (highest score for all
objects within the active priority group wins).  The default group is
0. Default priorityScore() returns the inverse of the Euclidean distance
from the wand tip. There are exceptions, e.g. if an object gets 'clicked'
on or some such, it can lock itself such that it keeps the focus until
it feels like giving it up.

Sub-classes should implement processTouch() and unTouch(); the latter gets
called when an object loses focus. They may also override priorityScore().

The application calls arUniqueInteractor::processAllTouches() (a static
method) to handle all interactions with all instances of any sub-classes.
*/

class SZG_CALL arUniqueInteractor {
  public:
    arUniqueInteractor();
    virtual ~arUniqueInteractor();
    arUniqueInteractor( const arUniqueInteractor& ui );
    arUniqueInteractor& operator=( const arUniqueInteractor& ui );

    // Static method, called by app to handle interaction with any subclass instances.
    static bool processAllTouches(  arInputState* inputState,
                                    const arMatrix4& wandTipMatrix,
                                    const float minPriorityScore );

    // Set interaction group (initial value = 1)
    void setInteractionGroup( const unsigned int group );

    // Return current interaction group
    unsigned int interactionGroup() { return _interactionGroup; }

    void disable(); // Disallow user interaction
    void enable();  // Allow user interaction

    bool enabled() const { return _enabled; }
    bool active() const { return _interactionGroup == _activeInteractionGroup; }

    static void activateInteractionGroup( const unsigned int group );

    arMatrix4 _matrix; // Position and orientation of object.

    // todo: decopypaste this and arInteractable.h

  protected:
    // Sub-class' event (got-focus) handler.
    // Called only for the particular instance that satisfies the criteria
    // implemented in the static method processAllTouches() (if any).
    // @param interface An arInterfaceObject pointer, for getting
    // miscellaneous input.
    // @param wandTipMatrix Wand tip position & orientation.
    // @param events A vector of button on/off events.
    virtual bool processTouch( const arInputState* const inputState,
                               const arMatrix4& wandTipMatrix,
                               const std::vector<arButtonClickEvent>& events ) = 0;

    virtual float priorityScore( const arMatrix4& wandTipMatrix );

    // Sub-class' loss-of-focus handler.
    // Called if _touched is true but this instance didn't get the focus.
    virtual void unTouch() = 0;
    bool lockMe(); // Lock focus.
    bool unlockMe(); // Unlock focus.

    bool _touched; // Do I currently have the focus?
    bool _enabled; // accepting interaction?
    unsigned int _interactionGroup; // priority group

  private:
    static arUniqueInteractor* _lockedPtr;
    static std::list<arUniqueInteractor*> _listUs;
    static std::vector<int> _lastButtons;
    static unsigned int _activeInteractionGroup;
};

#endif        //  #ifndefARUNIQUEINTERACTOR_H
