//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef ARINTERACTABLETHING_H
#define ARINTERACTABLETHING_H

#include "arMath.h"
#include "arTexture.h"
#include "arInteractable.h"

class arMasterSlaveFramework;

class SZG_CALL arInteractableThing : public arInteractable {
  public:
    arInteractableThing();
    arInteractableThing( const arInteractableThing& x );
    arInteractableThing& operator=( const arInteractableThing& x );
    virtual ~arInteractableThing() {}
    virtual void setTexture( arTexture* tex ) {_texture = tex; }
    virtual arTexture* getTexture() { return _texture; }
    virtual void setHighlight( bool flag ) { _highlighted = flag; }
    virtual bool getHighlight() { return _highlighted; }
    virtual void setColor( float r, float g, float b, float a=1. ) {_color = arVector4(r,g,b,a);}
    virtual void setColor( const arVector4& col ) {_color = col;}
    virtual void setColor( const arVector3& col ) {_color = arVector4(col,1);}
    virtual void setAlpha( float a ) {_color[3] = a;}
    virtual float getAlpha() { return _color[3]; }
    virtual arVector4 getColor() { return _color; }
    virtual void setVisible( bool vis ) {_visible = vis; }
    virtual bool getVisible() { return _visible; }
    virtual void activateColor() { glColor4f( _color[0], _color[1], _color[2], _color[3] ); }
    virtual bool activateTexture() { if (!_texture) return false; _texture->activate(); return true; }
    virtual void deactivateTexture() { if (_texture) _texture->deactivate(); }
    virtual void draw( arMasterSlaveFramework* fw=0 ) = 0;
  private:
    virtual bool _touch( arEffector& effector );
    virtual bool _processInteraction( arEffector& /*effector*/ ) { return true; }
    virtual bool _untouch( arEffector& effector );
    bool _visible;
    arVector4 _color;
    arTexture* _texture;
    bool _highlighted;
};

#endif        //  #ifndefARINTERACTABLETHING_H

