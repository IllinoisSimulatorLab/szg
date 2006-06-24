//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arPForth.h"
#include "arDataUtilities.h"
#include "arInputEvent.h"
#include "arPForthFilter.h"

// Run-time behaviors

namespace arPForthSpace {

class GetCurrentEventIndex : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetCurrentEventIndex::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arInputEvent* e = ar_PForthGetCurrentEvent();
  if (e == 0)
    throw arPForthException("NULL arInputEvent.");
  unsigned int temp = e->getIndex();
  pf->stackPush( (float) temp );
  return true;
}

class GetCurrentEventButton : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetCurrentEventButton::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arInputEvent* e = ar_PForthGetCurrentEvent();
  if (e == 0)
    throw arPForthException("NULL arInputEvent.");
  if (e->getType() != AR_EVENT_BUTTON)
    throw arPForthException("Requested button value for non-button event.");
  int temp = e->getButton();
  pf->stackPush( (float) temp );
  return true;
}

class GetCurrentEventAxis : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetCurrentEventAxis::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arInputEvent* e = ar_PForthGetCurrentEvent();
  if (e == 0)
    throw arPForthException("NULL arInputEvent.");
  if (e->getType() != AR_EVENT_AXIS)
    throw arPForthException("Requested axis value for non-axis event.");
  float temp = e->getAxis();
  pf->stackPush( temp );
  return true;
}

class GetCurrentEventMatrix : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetCurrentEventMatrix::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arInputEvent* e = ar_PForthGetCurrentEvent();
  if (e == 0)
    throw arPForthException("NULL arInputEvent.");
  if (e->getType() != AR_EVENT_MATRIX)
    throw arPForthException("Requested matrix value for non-matrix event.");
  long address = (long)pf->stackPop();
  arMatrix4 temp = e->getMatrix();
  pf->putDataMatrix( address, temp );
  return true;
}

class SetCurrentEventIndex : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool SetCurrentEventIndex::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arInputEvent* e = ar_PForthGetCurrentEvent();
  if (e == 0)
    throw arPForthException("NULL arInputEvent.");
  int temp = (int)pf->stackPop();
  if (temp < 0)
    throw arPForthException("Attempt to set negative event index.");
  e->setIndex( (unsigned int) temp );
  return true;
}

class SetCurrentEventButton : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool SetCurrentEventButton::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arInputEvent* e = ar_PForthGetCurrentEvent();
  if (e == 0)
    throw arPForthException("NULL arInputEvent.");
  int temp = (int)pf->stackPop();
  if (!e->setButton( temp ))
    throw arPForthException("failed to set button event value.");
  return true;
}

class SetCurrentEventAxis : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool SetCurrentEventAxis::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arInputEvent* e = ar_PForthGetCurrentEvent();
  if (e == 0)
    throw arPForthException("NULL arInputEvent.");
  float temp = pf->stackPop();
  if (!e->setAxis( temp ))
    throw arPForthException("failed to set button event value.");
  return true;
}

class SetCurrentEventMatrix : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool SetCurrentEventMatrix::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arInputEvent* e = ar_PForthGetCurrentEvent();
  if (e == 0)
    throw arPForthException("NULL arInputEvent.");
  long address = (long)pf->stackPop();
  arMatrix4 temp = pf->getDataMatrix( address );
  if (!e->setMatrix( temp ))
    throw arPForthException("failed to set matrix event value.");  
  return true;
}

class DeleteCurrentEvent : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool DeleteCurrentEvent::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arInputEvent* e = ar_PForthGetCurrentEvent();
  if (e == 0)
    throw arPForthException("NULL arInputEvent.");
  e->trash();
  return true;
}

class GetButton : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetButton::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arPForthFilter* f = ar_PForthGetFilter();
  if (f == 0)
    throw arPForthException("NULL filter");
  int buttonNumber = (int)pf->stackPop();
  if (buttonNumber < 0)
    throw arPForthException("negative event index.");
  int temp = f->getButton( buttonNumber );
  pf->stackPush( (float) temp );
  return true;
}

class GetAxis : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetAxis::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arPForthFilter* f = ar_PForthGetFilter();
  if (f == 0)
    throw arPForthException("NULL filter");
  int axisNumber = (int)pf->stackPop();
  if (axisNumber < 0)
    throw arPForthException("negative event index.");
  float val = f->getAxis( axisNumber );
  pf->stackPush( val );
  return true;
}

class GetMatrix : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool GetMatrix::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arPForthFilter* f = ar_PForthGetFilter();
  if (f == 0)
    throw arPForthException("NULL filter");
  int matrixNumber = (int)pf->stackPop();
  long address = (long)pf->stackPop();
  if (matrixNumber < 0)
    throw arPForthException("negative event index.");
  // Note if matrixNumber is invalid, returns identity
  arMatrix4 temp = f->getMatrix( matrixNumber );
  pf->putDataMatrix( address, temp );
  return true;
}

class InsertButtonEvent : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool InsertButtonEvent::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arPForthFilter* f = ar_PForthGetFilter();
  if (f == 0)
    throw arPForthException("NULL filter");
  int temp = (int)pf->stackPop();
  int value = (int)pf->stackPop();
  if (temp < 0)
    throw arPForthException("Attempt to create negative-indexed button event.");
  unsigned int index = (unsigned int) temp;
  f->insertNewEvent( arButtonEvent( index, value ) );
  return true;
}

class InsertAxisEvent : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool InsertAxisEvent::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arPForthFilter* f = ar_PForthGetFilter();
  if (f == 0)
    throw arPForthException("NULL filter");
  int temp = (int)pf->stackPop();
  float value = pf->stackPop();
  if (temp < 0)
    throw arPForthException("Attempt to create negative-indexed axis event.");
  unsigned int index = (unsigned int) temp;
  f->insertNewEvent( arAxisEvent( index, value ) );
  return true;
}

class InsertMatrixEvent : public arPForthAction {
  public:
    bool run( arPForth* pf );
};
bool InsertMatrixEvent::run( arPForth* pf ) {
  if (pf == 0)
    return false;
  arPForthFilter* f = ar_PForthGetFilter();
  if (f == 0)
    throw arPForthException("NULL filter");
  int temp = (int)pf->stackPop();
  long address = (long)pf->stackPop();
  if (temp < 0)
    throw arPForthException("Attempt to create negative-indexed button event.");
  unsigned int index = (unsigned int) temp;
  arMatrix4 value = pf->getDataMatrix( address );
  f->insertNewEvent( arMatrixEvent( index, value ) );
  return true;
}

} // arPForthSpace namespace

// variables

arPForthFilter* __PForthFilterPtr = 0;
arInputEvent* __PForthInputEventPtr = 0;

// functions

void ar_PForthSetInputEvent( arInputEvent* inputEvent ) {
  __PForthInputEventPtr = inputEvent;
}
void ar_PForthSetFilter( arPForthFilter* filter ) {
  __PForthFilterPtr = filter;
}
arInputEvent* ar_PForthGetCurrentEvent() {
  return __PForthInputEventPtr;
}
arPForthFilter* ar_PForthGetFilter() {
  return __PForthFilterPtr;
}

bool ar_PForthAddEventVocabulary( arPForth* pf ) {
  
  // Simple action words (run-time behavior only)
  if (!pf->addSimpleActionWord( "getCurrentEventIndex", new arPForthSpace::GetCurrentEventIndex() ))
    return false;
  if (!pf->addSimpleActionWord( "getCurrentEventButton", new arPForthSpace::GetCurrentEventButton() ))
    return false;
  if (!pf->addSimpleActionWord( "getCurrentEventAxis", new arPForthSpace::GetCurrentEventAxis() ))
    return false;
  if (!pf->addSimpleActionWord( "getCurrentEventMatrix", new arPForthSpace::GetCurrentEventMatrix() ))
    return false;
  if (!pf->addSimpleActionWord( "setCurrentEventIndex", new arPForthSpace::SetCurrentEventIndex() ))
    return false;
  if (!pf->addSimpleActionWord( "setCurrentEventButton", new arPForthSpace::SetCurrentEventButton() ))
    return false;
  if (!pf->addSimpleActionWord( "setCurrentEventAxis", new arPForthSpace::SetCurrentEventAxis() ))
    return false;
  if (!pf->addSimpleActionWord( "setCurrentEventMatrix", new arPForthSpace::SetCurrentEventMatrix() ))
    return false;
  if (!pf->addSimpleActionWord( "deleteCurrentEvent", new arPForthSpace::DeleteCurrentEvent() ))
    return false;
  if (!pf->addSimpleActionWord( "getButton", new arPForthSpace::GetButton() ))
    return false;
  if (!pf->addSimpleActionWord( "getAxis", new arPForthSpace::GetAxis() ))
    return false;
  if (!pf->addSimpleActionWord( "getMatrix", new arPForthSpace::GetMatrix() ))
    return false;
  if (!pf->addSimpleActionWord( "insertButtonEvent", new arPForthSpace::InsertButtonEvent() ))
    return false;
  if (!pf->addSimpleActionWord( "insertAxisEvent", new arPForthSpace::InsertAxisEvent() ))
    return false;
  if (!pf->addSimpleActionWord( "insertMatrixEvent", new arPForthSpace::InsertMatrixEvent() ))
    return false;

  return true;
}    
