// Defines mapping from python to C++ strings.
%Include typemaps.sip

%ModuleHeaderCode
#include <string>
using namespace std;
#include "arInteractionUtilities.h"
%End

bool ar_pollingInteraction( arEffector&, arInteractable* /TransferBack/ );

bool ar_pollingInteraction( arEffector& a0, SIP_PYLIST a1 );
%MethodCode
  int numItems = PyList_Size( a1 );
  list<arInteractable*> l;
  for (int i=0; i<numItems; ++i) {
    PyObject* item = PyList_GET_ITEM( a1, i );
    arInteractable* ptr;
    ptr = static_cast<arInteractable*>(
           sipForceConvertToInstance( item, sipClass_arInteractable, NULL, 0, NULL, &sipIsErr )
          );
    if (!ptr || sipIsErr) {
      PyErr_SetString( PyExc_TypeError, "ar_pollingInteraction() failed to convert list item to arInteractable" );
      l.clear();
      break;
    } else {
      l.push_back( ptr );
    }
  }
  if (!sipIsErr) {
    sipRes = ar_pollingInteraction( *a0, l );
  }
  l.clear();
%End


class arEffector {
%TypeHeaderCode
#include "arEffector.h"
%End
  public:
    arEffector();
    // NOTE: the "lo" parameters, e.g. "loButton", tell the effector which indices
    // to grab from the input. If numButtons = 3 and loButton = 2, then input button
    // events with indices 2-4 will be captured here. By default they get mapped to
    // a starting index of 0, e.g. in the case just described asking the effector
    // for button 0 will get you what was input button event 2. You can change that
    // with the buttonOffset parameter, e.g. if it is also set to to then you will
    // ask the effector for button 2 to get input event 2.
    arEffector( const unsigned int matrixIndex,
                const unsigned int numButtons,
                const unsigned int loButton,
                const unsigned int buttonOffset,
                const unsigned int numAxes,
                const unsigned int loAxis,
                const unsigned int axisOffset );
    arEffector( const unsigned int matrixIndex,
                const unsigned int numButtons,
                const unsigned int loButton,
                const unsigned int numAxes,
                const unsigned int loAxis );
    virtual ~arEffector();
    arEffector( const arEffector& e );
    //void setFramework( arSZGAppFramework* fw ) { _framework = fw; }
    //arSZGAppFramework* getFramework() const { return _framework; }
    void setInteractionSelector( const arInteractionSelector& selector );
    float calcDistance( const arMatrix4& mat );
    void setUnitConversion( float conv );
    void setTipOffset( const arVector3& offset );
    void updateState( arInputEvent& event );
    void updateState( arInputState* state );
    virtual bool requestGrab( arInteractable* grabee );
    virtual void requestUngrab( arInteractable* grabee );
    virtual void forceUngrab();
    int getButton( unsigned int index );
    float getAxis( unsigned int index );
    arMatrix4 getMatrix() const /TransferBack/;
    arMatrix4 getBaseMatrix() const /TransferBack/;
    arMatrix4 getOffsetMatrix() const /TransferBack/;
    arMatrix4 getInputMatrix() const /TransferBack/;
    arMatrix4 getCenterMatrix() const /TransferBack/;
    bool getOnButton( unsigned int index );
    bool getOffButton( unsigned int index );
    virtual void setMatrix( const arMatrix4& matrix );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    //void setDragManager( const arDragManager& dm );
    //const arDragManager* getDragManager() const;
    virtual const arInteractable* getGrabbedObject();
    bool hasGrabbedObject();
%MethodCode
      sipRes = sipCpp->getGrabbedObject() != NULL;
%End

    virtual void setTouchedObject( arInteractable* touched );
    virtual arInteractable* getTouchedObject();
    bool hasTouchedObject();
%MethodCode
      sipRes = sipCpp->getTouchedObject() != NULL;
%End

    virtual void draw() const;


};


class  arInteractionSelector {
%TypeHeaderCode
#include "arInteractionSelector.h"
%End
  public:
    virtual ~arInteractionSelector();
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const = 0;
    virtual arInteractionSelector* copy() const = 0 /TransferBack/;
};

class arDistanceInteractionSelector: arInteractionSelector {
%TypeHeaderCode
#include "arInteractionSelector.h"
%End
  public:
    arDistanceInteractionSelector( float maxDistance = -1. );
    virtual ~arDistanceInteractionSelector();
    void setMaxDistance( float maxDistance );
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const;
    virtual arInteractionSelector* copy() const /TransferBack/;
};

class arAlwaysInteractionSelector: arInteractionSelector {
%TypeHeaderCode
#include "arInteractionSelector.h"
%End
  public:
    arAlwaysInteractionSelector();
    virtual ~arAlwaysInteractionSelector();
    virtual float calcDistance( const arEffector& /*effector*/,
                                const arMatrix4& /*objectMatrix*/ );
    virtual arInteractionSelector* copy() const /TransferBack/;
};

class arAngleInteractionSelector: arInteractionSelector {
%TypeHeaderCode
#include "arInteractionSelector.h"
%End
  public:
    arAngleInteractionSelector( float maxAngle = ar_convertToRad(10) );
    virtual ~arAngleInteractionSelector();
    void setMaxAngle( float maxAngle );
    virtual float calcDistance( const arEffector& effector,
                                const arMatrix4& objectMatrix ) const;
    virtual arInteractionSelector* copy() const /TransferBack/;
};



class arGrabCondition {
%TypeHeaderCode
#include "arGrabCondition.h"
%End
  public:
    arGrabCondition();
    arGrabCondition( arInputEventType eventType,
                     unsigned int eventIndex,
                     float thresholdValue );
    virtual ~arGrabCondition();
    arGrabCondition( const arGrabCondition& g );
    //bool operator==( const arGrabCondition& g ) const;
    int __eq__( const arGrabCondition& a0 );
%MethodCode
      sipRes = sipCpp->operator==( *a0 );
%End

    virtual bool check( arEffector* effector );
    virtual float type() const;
    virtual float index() const;
    virtual float threshold() const;
    virtual float value() const;
    virtual arGrabCondition* copy() const /TransferBack/;
};

class arDeltaGrabCondition : arGrabCondition {
%TypeHeaderCode
#include "arGrabCondition.h"
%End
  public:
    arDeltaGrabCondition();
    arDeltaGrabCondition( unsigned int eventIndex,
                           bool on=true );
    arDeltaGrabCondition( unsigned int eventIndex,
                           bool on, bool current );
    arDeltaGrabCondition( const arDeltaGrabCondition& x );
    virtual ~arDeltaGrabCondition();
    virtual bool check( arEffector* effector );
    virtual arGrabCondition* copy() const /TransferBack/;
};

class arAlwaysGrabCondition : arGrabCondition {
%TypeHeaderCode
#include "arGrabCondition.h"
%End
  public:
    arAlwaysGrabCondition();
    float type() const;
    float index() const;
    float threshold() const;
    float value() const;
    virtual bool check( arEffector* effector );
};


class arDragBehavior {
%TypeHeaderCode
#include "arDragBehavior.h"
%End
  public:
    virtual ~arDragBehavior();
    // NOTE: SIP barfs on e.g. 'const arEffector* const ...'
    virtual void init( const arEffector* effector,
                       const arInteractable* object ) = 0;
    virtual void update( const arEffector* effector,
                         arInteractable* object,
                         const arGrabCondition* grabCondition ) = 0;
    virtual arDragBehavior* copy() const = 0 /TransferBack/;
};

class arNullDragBehavior : arDragBehavior {
%TypeHeaderCode
#include "arDragBehavior.h"
%End
  public:
    virtual ~arNullDragBehavior();
    virtual void init( const arEffector* effector,
                       const arInteractable* object );
    virtual void update( const arEffector* effector,
                         arInteractable* object,
                         const arGrabCondition* grabCondition );
    virtual arDragBehavior* copy() const /TransferBack/;
};

class arWandRelativeDrag : arDragBehavior {
%TypeHeaderCode
#include "arDragBehavior.h"
%End
  public:
    arWandRelativeDrag();
    arWandRelativeDrag( const arWandRelativeDrag& wrd );
    ~arWandRelativeDrag();
    void init( const arEffector* effector,
               const arInteractable* object );
    void update( const arEffector* effector,
                 arInteractable* object,
                 const arGrabCondition* grabCondition );
    arDragBehavior* copy() const /TransferBack/;
};

class arWandTranslationDrag : arDragBehavior {
%TypeHeaderCode
#include "arDragBehavior.h"
%End
  public:
    arWandTranslationDrag( bool allowOffset = true );
    arWandTranslationDrag( const arWandTranslationDrag& wrd );
    ~arWandTranslationDrag();
    void init( const arEffector* effector,
               const arInteractable* object );
    void update( const arEffector* effector,
                 arInteractable* object,
                 const arGrabCondition* grabCondition );
    arDragBehavior* copy() const /TransferBack/;
};

class arWandRotationDrag : arDragBehavior {
%TypeHeaderCode
#include "arDragBehavior.h"
%End
  public:
    arWandRotationDrag();
    arWandRotationDrag( const arWandRotationDrag& wrd );
    ~arWandRotationDrag();
    void init( const arEffector* effector,
               const arInteractable* object );
    void update( const arEffector* effector,
                 arInteractable* object,
                 const arGrabCondition* grabCondition );
    arDragBehavior* copy() const /TransferBack/;
};



class arInteractable {
%TypeHeaderCode
#include "arInteractable.h"
%End
  public:
    arInteractable();
    virtual ~arInteractable();
    arInteractable( const arInteractable& ui );
    
    virtual bool touch( arEffector& effector );
    virtual bool processInteraction( arEffector& effector );
    virtual bool untouch( arEffector& effector );
    virtual bool untouchAll();
    
    void disable();                // Disallow user interaction
    void enable( bool flag=true ); // Allow user interaction
    bool enabled() const;

    void useDefaultDrags( bool flag );
    void setDrag( const arGrabCondition& cond,
                  const arDragBehavior& behave );
    void deleteDrag( const arGrabCondition& cond );
    //void setDragManager( const arDragManager& dm ) { _dragManager = dm; }
    bool touched() const;
    bool touched( arEffector& effector );
    const arEffector* grabbed() const;
    virtual void setMatrix( const arMatrix4& matrix );
    arMatrix4 getMatrix() const /TransferBack/;
    virtual void updateMatrix( const arMatrix4& deltaMatrix );
    
  protected:
    // Subclass's event (got-focus) handler.    
    // Called only for the particular instance that satisfies the criteria
    // implemented in the static method processAllTouches() (if any).
    // @param interface An arInterfaceObject pointer, for getting miscellaneous input.
    // @param wandTipMatrix Wand tip position & orientation.
    // @param events A vector of button on/off events.
    virtual bool _processInteraction( arEffector& ) /PyName=onInteraction/;
    
    // Subclass's gain-of-focus handler.    
    virtual bool _touch( arEffector& ) /PyName=onTouch/;

    // Subclass's loss-of-focus handler.    
    // Called if _touched is true but this instance didn't get the focus.
    virtual bool _untouch( arEffector& ) /PyName=onUntouch/;

    virtual void _cleanup() /PyName=onCleanup/;
};


