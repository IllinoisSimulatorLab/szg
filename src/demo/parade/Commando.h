//***************************************************************************
//Written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

#ifndef COMMANDO_H
#define COMMANDO_H

#include "arGraphicsAPI.h"
#include "arMath.h"
#include "BlobbyMan.h"
#include <string>
using namespace std;

enum { STAND=0, PACE, CANCAN }; // Marching orders

class Commando : public BlobbyMan
{
 public:
  Commando();
  Commando(char);
  Commando(char, ARfloat shirtColor[3]);
  ~Commando();

  void stand(void);
  void pace(float phase, float howfar, float howfast);
  void cancan(float howfast);
  void execute();

protected:
  int _orders;

  void _standAction();
  void _paceAction();
  void _cancanAction();

  float _phase, _howfar, _howfast;

  float _pacelength;
  int _numPaceIterations;
  int _numWalkIterations;
  int _iterationCount;
  char _whichWayUp;
};

#endif
