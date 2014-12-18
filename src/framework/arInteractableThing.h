//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_INTERACTABLE_THING_H
#define AR_INTERACTABLE_THING_H

#include "arMath.h"
#include "arTexture.h"
#include "arInteractable.h"
#include "arFrameworkCalling.h"

class arMasterSlaveFramework;
class arGraphicsWindow;
class arViewport;

class SZG_CALL arInteractableThing : public arInteractable {
  public:
    arInteractableThing();
    arInteractableThing( const arInteractableThing& x );
    arInteractableThing& operator=( const arInteractableThing& x );
    virtual ~arInteractableThing();
    virtual void setTexture( arTexture* tex ) {_texture = tex; }
    virtual arTexture* getTexture() { return _texture; }
    virtual void setHighlight( bool flag ) { _highlighted = flag; }
    virtual bool getHighlight() const { return _highlighted; }
    virtual void setColor( float r, float g, float b, float a=1. ) {_color = arVector4(r, g, b, a);}
    virtual void setColor( const arVector4& col ) {_color = col;}
    virtual void setColor( const arVector3& col ) {_color = arVector4(col, 1);}
    virtual void setAlpha( float a ) {_color[3] = a;}
    virtual float getAlpha() { return _color[3]; }
    virtual arVector4 getColor() const { return _color; }
    virtual void setVisible( bool vis ) {_visible = vis; }
    virtual bool getVisible() const { return _visible; }
    virtual void activateColor() const { glColor4fv(_color.v); }
    virtual bool activateTexture() { return _texture && _texture->activate(); }
    virtual void deactivateTexture() { if (_texture) _texture->deactivate(); }
    virtual void draw( arMasterSlaveFramework* =0 ) {}
    virtual void draw( arMasterSlaveFramework&, arGraphicsWindow&, arViewport& ) {}
  protected:
    virtual bool _touch( arEffector& effector );
    virtual bool _processInteraction( arEffector& ) { return true; }
    virtual bool _untouch( arEffector& effector );
    bool _visible;
    arVector4 _color;
    arTexture* _texture;
    bool _highlighted;
};

#endif        //  #ifndefARINTERACTABLETHING_H
