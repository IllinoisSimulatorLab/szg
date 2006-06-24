//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_VRCAMERA_H
#define AR_VRCAMERA_H

#include "arCamera.h"
#include "arGraphicsCalling.h"

#include <string>

class arHead;
class arGraphicsScreen;

class SZG_CALL arVRCamera : public arCamera {
  public:
    arVRCamera( arHead* head=0 ) : _head(head),  _complained(false) {}
    virtual ~arVRCamera() {}
    virtual arCamera* clone() const
      { return (arCamera*) new arVRCamera(_head); }
    virtual arMatrix4 getProjectionMatrix();
    virtual arMatrix4 getModelviewMatrix();
    virtual void loadViewMatrices();
    virtual std::string type( void ) const { return "arVRCamera"; }
    void setHead( arHead* head) { _head = head; }
    arHead* getHead() const { return _head; }
  private:
    arMatrix4 _getFixedHeadModeMatrix( const arGraphicsScreen& screen );
    arHead* _head;
    bool _complained;
};

#endif // AR_VRCAMERA_H
