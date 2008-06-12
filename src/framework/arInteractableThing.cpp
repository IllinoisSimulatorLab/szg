#include "arPrecompiled.h"
#include "arInteractableThing.h"

arInteractableThing::arInteractableThing() :
  arInteractable(),
  _visible(true),
  _color(1, 1, 1, 1),
  _texture(0),
  _highlighted(false) {  
}

arInteractableThing::arInteractableThing( const arInteractableThing& x ) :
  arInteractable(x),
  _visible(x._visible),
  _color(x._color),
  _texture(x._texture),
  _highlighted(x._highlighted) {
}

arInteractableThing& arInteractableThing::operator=( const arInteractableThing& x ) {
  if (&x == this)
    return *this;
  arInteractable::operator=(x);
  _visible = x._visible;
  _color = x._color;
  _texture = x._texture;
  _highlighted = x._highlighted;
  return *this;
}

arInteractableThing::~arInteractableThing() {
}

bool arInteractableThing::_touch( arEffector& ) {
  setHighlight(true);
  return true;
}

bool arInteractableThing::_untouch( arEffector& ) {
  setHighlight(false);
  return true;
}
