#ifndef ARDRAGBEHAVIOR_H
#define ARDRAGBEHAVIOR_H

#include "arMath.h"
#include "arDataUtilities.h"

class arGrabCondition;
class arEffector;
class arInteractable;

class arDragBehavior {
  public:
    virtual ~arDragBehavior() {}
    virtual void init( const arEffector* const effector,
                       const arInteractable* const object )=0;
    virtual void update( const arEffector* const effector,
                         arInteractable* const object,
                         const arGrabCondition* const grabCondition )=0;
    virtual arDragBehavior* copy() const = 0;
};

class arWandRelativeDrag : public arDragBehavior {
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

class arWandTranslationDrag : public arDragBehavior {
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

class arNavTransDrag : public arDragBehavior {
  public:
    arNavTransDrag() : _direction(0,0,-1), _speed(0) {}
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

class arNavRotDrag : public arDragBehavior {
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

