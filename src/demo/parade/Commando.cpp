//***************************************************************************
//Written by Matthew Woodruff, Ben Bernard, and Doug Nachand
//Released under the GNU LPL
//***************************************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "Commando.h"

#ifndef M_PI_4
#define M_PI 3.15159
#define M_PI_2 1.5708
#define M_PI_4 0.78540
#endif

#ifndef lSETi
#define lSETi(u, i, x,y,z) {u[i+0]=x;u[i+1]=y;u[i+2]=z;} 
#endif
#ifndef lCPYi
#define lCPYi(u,i,v) {u[i+0]=v[0];u[i+1]=v[1];u[i+2]=v[2];}
#endif
#ifndef lCROSSi
#define lCROSSi(u,a,v,b,t,c) {u[a+0]=v[b+1]*t[c+2]-v[b+2]*t[c+1];\
                              u[a+1]=v[b+2]*t[c+0]-v[b+0]*t[c+2];\
                              u[a+2]=v[b+0]*t[c+1]-v[b+1]*t[c+0];}
#endif

#ifndef lDOTi
#define lDOTi(u,a,v,b) (u[a+0]*v[b+0]+u[a+1]*v[b+1]+u[a+2]*v[a+2])
#endif

#ifndef lSUBi
#define lSUBi(u,a,v,b,t,c){u[a+0]=v[b+0]-t[c+0];\
                           u[a+1]=v[b+1]-t[c+1];\
                           u[a+2]=v[b+2]-t[c+2];}
#endif

#ifndef lLENGTHi
#define lLENGTHi(u,i) (sqrt(lDOTi(u,i,u,i)))
#endif

Commando::Commando() :
  _whichWayUp('y')
{
  float shirtColor[3]={0.7,0.2,0.2};
  initialize(shirtColor);
}

Commando::Commando(char whichWayUp)
{
  _whichWayUp=whichWayUp;
  float shirtColor[3]={0.7,0.2,0.2};
  initialize(shirtColor);
  switch (_whichWayUp)
  {
    case 'x':
      manTransform = ar_rotationMatrix('y', M_PI_2);
      break;
    case 'y':
      manTransform = ar_rotationMatrix('x', 3*M_PI_2)*
                     ar_rotationMatrix('z', M_PI) * ar_rotationMatrix('y', M_PI);
      break;
    case 'z':
      break;
  }
}

Commando::Commando(char whichWayUp, ARfloat shirtColor[3])
{
  _whichWayUp = whichWayUp;
  initialize(shirtColor);
  switch (_whichWayUp)
  {
    case 'x': 
      manTransform = ar_rotationMatrix('y', M_PI_2);
      break;
    case 'y':
      manTransform = ar_rotationMatrix('x', 3*M_PI_2)*
                     ar_rotationMatrix('z', M_PI);
      break;
    case 'z':
      break;
  }
}

Commando::~Commando(){
  for (int ii=0; ii<NUMBER_OF_PARTS; ii++)
    if (blobs[ii])
      delete blobs[ii];
}

void Commando::stand()
{
  _orders = STAND;
  resetMan();
}

void Commando::pace(float startPhase, float howfar, float howfast)
{
  float fudgefactor=4.3;
  //to ensure that an integral number of paces 
  //corresponds to each part of the walk
  int speedfactor=2;  
  _orders = PACE;
  _phase = startPhase;
  _howfar = howfar;

  _pacelength = 0.482*M_PI_4*fudgefactor;
  //multiply by four to make things nice
  _numPaceIterations = speedfactor * int(_pacelength/howfast);
  _numWalkIterations = speedfactor * int(_howfar/howfast);
  if (_numPaceIterations == 0 || _numWalkIterations == 0) {
    _numPaceIterations=4;
    _numWalkIterations=32;
  }
  _howfast = _howfar / float(_numWalkIterations);

  _iterationCount = int((_phase-int(_phase)) * _numWalkIterations);
  _iterationCount = _iterationCount - _iterationCount % _numPaceIterations;
  
  rotatePart(RIGHT_KNEE, 'x', M_PI/6);
  rotatePart(RIGHT_ELBOW, 'x', -M_PI/12.0);
  rotatePart(LEFT_ELBOW, 'x', -M_PI/12.0);
  changeFlags[NUMBER_OF_PARTS]=1;
}

void Commando::cancan(float howfast)
{
  _orders = CANCAN;
  _phase = 0;
  _howfast = howfast;
}

void Commando::execute()
{
  switch (_orders)
    {
  case STAND:
    _standAction();
    break;
  case PACE:
    _paceAction();
    break;
  case CANCAN:
    _cancanAction();
    break;
    }
}

void Commando::_standAction()
{
  update();
}

void Commando::_paceAction()
{
  if(_iterationCount<0)
    _iterationCount=0;
  float foo = _numPaceIterations / 4.;
  if((_iterationCount%_numPaceIterations) < (0.25*_numPaceIterations))
  {
    //step with right leg
    rotatePart(LEFT_HIP,'x',M_PI/(16.0*foo));
    rotatePart(RIGHT_HIP, 'x',-M_PI/(16.0*foo));
    rotatePart(RIGHT_KNEE, 'x',-M_PI/(6.0*foo));
    //swing the arms a little
    rotatePart(LEFT_SHOULDER, 'x',-M_PI/(12.0*foo));
    rotatePart(RIGHT_SHOULDER, 'x', M_PI/(12.0*foo));
    rotatePart(LEFT_ELBOW, 'x',-M_PI/(12.0*foo));
    rotatePart(RIGHT_ELBOW, 'x', M_PI/(12.0*foo));
    //twist at the waist, swinging shoulders and hips
    rotatePart(LUMBAR, 'z', (M_PI/(24.0*foo)));
    rotatePart(WAIST, 'z', (-M_PI/(24.0*foo)));
    //rotate neck, so he keeps looking forward
    rotatePart(NECK, 'z', (M_PI/(24.0*foo)));
  }
  else if((_iterationCount%_numPaceIterations) < (0.5*_numPaceIterations))
  {
    //step with left leg
    rotatePart(LEFT_HIP,'x',-M_PI/(16.0*foo));
    rotatePart(RIGHT_HIP, 'x', M_PI/(16.0*foo));
    rotatePart(LEFT_KNEE, 'x',M_PI/(6.0*foo));
    //swing the arms a little
    rotatePart(LEFT_SHOULDER, 'x', M_PI/(12.0*foo));
    rotatePart(RIGHT_SHOULDER, 'x',-M_PI/(12.0*foo));
    rotatePart(LEFT_ELBOW, 'x', M_PI/(12.0*foo));
    rotatePart(RIGHT_ELBOW, 'x',-M_PI/(12.0*foo));
    //twist at the waist, swinging shoulders and hips
    rotatePart(LUMBAR, 'z', (-M_PI/(24.0*foo)));
    rotatePart(WAIST, 'z', ( M_PI/(24.0*foo)));
    //rotate neck, so he keeps looking forward
    rotatePart(NECK, 'z', -(M_PI/(24.0*foo)));
  }
  else if((_iterationCount%_numPaceIterations) < (0.75*_numPaceIterations))
  {
    //step with left leg
    rotatePart(LEFT_HIP,'x',-M_PI/(16.0*foo));
    rotatePart(RIGHT_HIP, 'x', M_PI/(16.0*foo));
    rotatePart(LEFT_KNEE, 'x',-M_PI/(6.0*foo));
    //swing the arms a little
    rotatePart(LEFT_SHOULDER, 'x', M_PI/(12.0*foo));
    rotatePart(RIGHT_SHOULDER, 'x',-M_PI/(12.0*foo));
    rotatePart(LEFT_ELBOW, 'x', M_PI/(12.0*foo));
    rotatePart(RIGHT_ELBOW, 'x',-M_PI/(12.0*foo));
    //twist at the waist, swinging shoulders and hips
    rotatePart(LUMBAR, 'z', (-M_PI/(24.0*foo)));
    rotatePart(WAIST, 'z', ( M_PI/(24.0*foo)));
    //rotate neck, so he keeps looking forward
    rotatePart(NECK, 'z', -(M_PI/(24.0*foo)));
  }
  else
  {
    //step with right leg
    rotatePart(LEFT_HIP,'x',M_PI/(16.0*foo));
    rotatePart(RIGHT_HIP, 'x',-M_PI/(16.0*foo));
    rotatePart(RIGHT_KNEE, 'x',M_PI/(6.0*foo));
    //swing the arms a little
    rotatePart(LEFT_SHOULDER, 'x',-M_PI/(12.0*foo));
    rotatePart(RIGHT_SHOULDER, 'x', M_PI/(12.0*foo));
    rotatePart(LEFT_ELBOW, 'x',-M_PI/(12.0*foo));
    rotatePart(RIGHT_ELBOW, 'x', M_PI/(12.0*foo));
    //twist at the waist, swinging shoulders and hips
    rotatePart(LUMBAR, 'z', (M_PI/(24.0*foo)));
    rotatePart(WAIST, 'z', (-M_PI/(24.0*foo)));
    //rotate neck, so he keeps looking forward
    rotatePart(NECK, 'z', (M_PI/(24.0*foo)));
  }

  //only y axis up is implemented so far
  // Why this is a member variable, anyways?
  manTransform =
    ar_rotationMatrix('y', M_PI) *
    ar_rotationMatrix('x', 3*M_PI_2)*
    ar_rotationMatrix('z', M_PI);

  float xTurnDistance, zTurnDistance;
  int turn1,turn2,turn3,turn4;
  int zfactor=_numPaceIterations*1;
  turn1=(_numWalkIterations/4)-zfactor;
  turn3=(_numWalkIterations-zfactor)-(_numWalkIterations/4);
  turn2=turn1+2*zfactor;
  turn4=turn3+2*zfactor;
  xTurnDistance=turn1*_howfast;
  zTurnDistance=(turn2-turn1)*_howfast;

  if((_iterationCount%_numWalkIterations)<turn1)
  {
    //right
    manTransform = ar_rotationMatrix('y', 3*M_PI_2) * manTransform;
    manTransform = 
      ar_translationMatrix(0-(_iterationCount%_numWalkIterations)*_howfast,0,
      zTurnDistance)
      *manTransform;
  }
  else if((_iterationCount%_numWalkIterations)<turn2)
  {
    manTransform = ar_rotationMatrix('y', M_PI) * manTransform;
    manTransform = 
      ar_translationMatrix
        (-xTurnDistance, 0, 
        zTurnDistance-((_iterationCount%_numWalkIterations)-turn1)*_howfast)
      *manTransform;
  }
  else if((_iterationCount%_numWalkIterations)<turn3)
  {
    //left
    manTransform = ar_rotationMatrix('y',   M_PI_2)*manTransform;
    manTransform =
      ar_translationMatrix
        ((_iterationCount%_numWalkIterations-turn2)*_howfast-xTurnDistance,0,0)
      *manTransform;
  }
  else if((_iterationCount%_numWalkIterations)<turn4)
  {
    manTransform = ar_rotationMatrix('y', M_PI) * manTransform;
    manTransform = 
      ar_translationMatrix
        (xTurnDistance, 0, 
        ((_iterationCount%_numWalkIterations)-turn3)*_howfast)
      *ar_rotationMatrix('y',M_PI)*manTransform;
  }
  else
  {
    //right again
    manTransform = ar_rotationMatrix('y', 3*M_PI_2) * manTransform;
    manTransform = 
      ar_translationMatrix
        (xTurnDistance-(_iterationCount%_numWalkIterations-turn4)*_howfast,
        0,zTurnDistance)
      * manTransform;
  }

  changeFlags[NUMBER_OF_PARTS]=1;
  _iterationCount++;
  update();
}

// not yet implemented
void Commando::_cancanAction()
{
  update();
}
