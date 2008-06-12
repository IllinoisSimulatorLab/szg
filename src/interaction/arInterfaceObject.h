//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INTERFACE_OBJECT
#define AR_INTERFACE_OBJECT

#include "arInputNode.h"
#include "arMath.h"
#include "arInteractionCalling.h"

#include <vector>

// Input device (gamepad, motion tracker, keyboard, etc.).

class SZG_CALL arInterfaceObject{
  // Needs assignment operator and copy constructor, for pointer members.
  friend void ar_interfaceObjectIOPollTask(void*);
 public:
  arInterfaceObject();
  ~arInterfaceObject();

  void setInputDevice(arInputNode*);
  bool start();
  void setNavMatrix(const arMatrix4&);
  arMatrix4 getNavMatrix() const;
  void setObjectMatrix(const arMatrix4&);
  arMatrix4 getObjectMatrix() const;

  void setSpeedMultiplier(float);

  void setNumMatrices( const int num );
  void setNumButtons( const int num );
  void setNumAxes( const int num );

  int getNumMatrices() const;
  int getNumButtons() const;
  int getNumAxes() const;

  bool setMatrix( const int num, const arMatrix4& mat );
  bool setButton( const int num, const int but );
  bool setAxis( const int num, const float val );
  void setMatrices( const arMatrix4* matPtr );
  void setButtons( const int* butPtr );
  void setAxes( const float* axisPtr );

  arMatrix4 getMatrix( const int num ) const;
  int getButton( const int num ) const;
  float getAxis( const int num ) const;

 private:
  arInputNode* _inputDevice;
  arThread     _IOPollThread;

  float _speedMultiplier;
  arVector3 _vMovePrev;

  bool _grabbed;
  arMatrix4 _mGrab;

  mutable arLock _infoLock;

  arMatrix4 _mNav;
  arMatrix4 _mObj;

  std::vector<arMatrix4> _matrices;
  std::vector<int> _buttons;
  std::vector<float> _axes;

  void _ioPollTask();
};

#endif
