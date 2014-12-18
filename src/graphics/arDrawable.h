#ifndef ARDRAWABLE_H
#define ARDRAWABLE_H

// THIS MUST BE THE LAST SZG INCLUDE!
#include "arGraphicsCalling.h"

#include "arMath.h"
#include "arTexture.h"

class SZG_CALL arDrawable {
  public:
    arDrawable() : _visible(true), _color(1,1,1,1), _texture(NULL) {}
    arDrawable( const arDrawable& x ) :
        _visible(x._visible),
        _color(x._color),
        _texture(x._texture) {}
    virtual ~arDrawable() {}
    virtual void setTexture( arTexture* tex ) {_texture = tex; }
    virtual arTexture* getTexture() { return _texture; }
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
    virtual void draw()=0;
  protected:
    bool _visible;
    arVector4 _color;
    arTexture* _texture;
};

#endif        //  #ifndefARDRAWABLE_H


