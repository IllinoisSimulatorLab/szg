//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_DRAG_BEHAVIOR_H
#define AR_DRAG_BEHAVIOR_H

#include "arMath.h"
#include "arDataUtilities.h"
#include "arInteractionCalling.h"

class arGrabCondition;
class arEffector;
class arInteractable;

class SZG_CALL arDragBehavior {
  public:
    virtual ~arDragBehavior() {}
    virtual void init( const arEffector* const /*effector*/,
                       const arInteractable* const /*object*/ ) {}
    virtual void update( const arEffector* const /*effector*/,
                         arInteractable* const /*object*/,
                         const arGrabCondition* const /*grabCondition*/ ) {}
    virtual arDragBehavior* copy() const { return new arDragBehavior(); }
};

class SZG_CALL arNullDragBehavior : public arDragBehavior {
  public:
    virtual ~arNullDragBehavior() {}
    virtual void init( const arEffector* const /*effector*/,
                       const arInteractable* const /*object*/ ) {}
    virtual void update( const arEffector* const /*effector*/,
                         arInteractable* const /*object*/,
                         const arGrabCondition* const /*grabCondition*/ ) {}
    virtual arDragBehavior* copy() const;
};

typedef void (* arDragInitCallback_t)( const arEffector* const,
                                       const arInteractable* const );
typedef void (* arDragUpdateCallback_t)( const arEffector* const,
                                         const arInteractable* const,
                                         const arGrabCondition* const );

class SZG_CALL arCallbackDragBehavior : public arDragBehavior {
  public:
    arCallbackDragBehavior() :
      _initCallback(0),
      _updateCallback(0) {
      }
    arCallbackDragBehavior( arDragInitCallback_t initFunc, arDragUpdateCallback_t updateFunc ) {
      setCallbacks( initFunc, updateFunc );
      }
    virtual ~arCallbackDragBehavior() {}
    virtual void init( const arEffector* const effector,
                       const arInteractable* const object );
    virtual void update( const arEffector* const effector,
                         arInteractable* const object,
                         const arGrabCondition* const grabCondition );
    virtual arDragBehavior* copy() const;
    void setCallbacks( arDragInitCallback_t initFunc, arDragUpdateCallback_t updateFunc ) {
      _initCallback = initFunc;
      _updateCallback = updateFunc;
    }
  private:
    arDragInitCallback_t _initCallback;
    arDragUpdateCallback_t _updateCallback;
};

class SZG_CALL arWandRelativeDrag : public arDragBehavior {
  public:
    arWandRelativeDrag();
    arWandRelativeDrag( const arWandRelativeDrag& wrd );
    ~arWandRelativeDrag() {}
    void init( const arEffector* const effector,
               const arInteractable* const object );
    void update( const arEffector* const effector,
                 arInteractable* const object,
                 const arGrabCondition* const grabCondition );
    arDragBehavior* copy() const;
  private:
    arMatrix4 _diffMatrix;
};

class SZG_CALL arWandTranslationDrag : public arDragBehavior {
  public:
    arWandTranslationDrag( bool allowOffset = true );
    arWandTranslationDrag( const arWandTranslationDrag& wrd );
    ~arWandTranslationDrag() {}
    void init( const arEffector* const effector,
               const arInteractable* const object );
    void update( const arEffector* const effector,
                 arInteractable* const object,
                 const arGrabCondition* const grabCondition );
    arDragBehavior* copy() const;
  private:
    bool _allowOffset;
    arMatrix4 _positionOffsetMatrix;
    arMatrix4 _objectOrientationMatrix;
};

class SZG_CALL arWandRotationDrag : public arDragBehavior {
  public:
    arWandRotationDrag();
    arWandRotationDrag( const arWandRotationDrag& wrd );
    ~arWandRotationDrag() {}
    void init( const arEffector* const effector,
               const arInteractable* const object );
    void update( const arEffector* const effector,
                 arInteractable* const object,
                 const arGrabCondition* const grabCondition );
    arDragBehavior* copy() const;
  private:
    arMatrix4 _positionMatrix;
    arMatrix4 _orientDiffMatrix;
};

class SZG_CALL arNavTransDrag : public arDragBehavior {
  public:
    arNavTransDrag() : _direction(0, 0, -1), _speed(0) {}
    arNavTransDrag( const arVector3& displacement );
    arNavTransDrag( const arVector3& direction, const float speed );
    arNavTransDrag( const char axis, const float speed );
    arNavTransDrag( const arNavTransDrag& ntd );
    ~arNavTransDrag() {}
    void setSpeed( const float speed );
    void setDirection( const arVector3& direction );
    void init( const arEffector* const effector,
               const arInteractable* const navigator );
    void update( const arEffector* const effector,
                 arInteractable* const navigator,
                 const arGrabCondition* const grabCondition );
    arDragBehavior* copy() const;
  private:
    arVector3 _direction;
    float _speed;
    ar_timeval _lastTime;
};

class SZG_CALL arNavRotDrag : public arDragBehavior {
  public:
    arNavRotDrag( const arVector3& axis, const float degreesPerSec );
    arNavRotDrag( const char axis, const float degreesPerSec );
    arNavRotDrag( const arNavRotDrag& nrd );
    ~arNavRotDrag() {}
    void setRotationSpeed( const float degreesPerSec ) {
      _angleSpeed = ar_convertToRad( degreesPerSec );
    }
    void init( const arEffector* const effector,
               const arInteractable* const navigator );
    void update( const arEffector* const effector,
                 arInteractable* const navigator,
                 const arGrabCondition* const grabCondition );
    arDragBehavior* copy() const;
  private:
    arVector3 _axis;
    float _angleSpeed;
    ar_timeval _lastTime;
};

//class arNavWorldRotDrag : public arDragBehavior {
//  public:
//    arNavWorldRotDrag();
//    arNavWorldRotDrag( const arNavWorldRotDrag& nwrd );
//    ~arNavWorldRotDrag() {}
//    void init( const arEffector* const effector,
//               const arInteractable* const navigator );
//    void update( const arEffector* const effector,
//                         arInteractable* const navigator,
//                         const arGrabCondition* const grabCondition );
//    arDragBehavior* copy() const;
//  private:
//    arMatrix4 _effMatrix;
//    arMatrix4 _navMatrix;
//};

#endif        //  #ifndefARDRAGBEHAVIOR_H

