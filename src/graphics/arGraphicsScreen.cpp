//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGraphicsScreen.h"
#include "arGraphicsHeader.h"

ostream& operator<<(ostream& s, arGraphicsScreen& g) {
  s << "arGraphicsScreen:\n"
    << "  Center = " << g.getCenter() << "\n"
    << "  Normal = " << g.getNormal() << "\n"
    << "  Up = " << g.getUp()<< "\n"
    << "  Width x Height = (" << g.getWidth() << "," << g.getHeight() << ")\n"
    << "  Head-mounted = " << g.getHeadMounted() << "\n";
  return s;
}

// Since the default window is 640x480, use a wall size different
// from the cube's front wall.  These are for the syzygy "front wall",
// on which all apps should show something sensible on startup.
const float defaultWidth = 13.333;
const float defaultHeight = 10.;

arGraphicsScreen::arGraphicsScreen():
  _setCenter( 0, 5, -5 ),
  _setHeight(10),
  _setWidth(defaultWidth),
  _center( 0, 0, -5 ),
  _normal( 0, 0, -1 ),
  _up( 0, 1, 0 ),
  _height( defaultHeight ),
  _width( 13.333 ),
  _tileX( 0 ),
  _numberTilesX( 1 ),
  _tileY( 0 ),
  _numberTilesY( 1 ),
  _headMounted(true),
  _ignoreFixedHeadMode(false),
  _alwaysFixedHeadMode(false),
  _fixedHeadPosition(0, 5, 0),
  _fixedHeadUpAngle(0) {
}

bool arGraphicsScreen::configure(arSZGClient& client) {
  return configure( client.getMode("graphics"), client );
}

// Return true.  Defaults for all parameters the arSZGClient might query.
bool arGraphicsScreen::configure(const string& screenName, arSZGClient& client) {
  (void)client.initResponse();
  _headMounted = client.getAttribute( screenName, "head_mounted", "|true|false|") == "true";

  float floatbuf[2];
  if (client.getAttributeFloats( screenName, "screen_dim", floatbuf, 2)) {
    _width = floatbuf[0];
    _height = floatbuf[1];
  } else {
    _width = defaultWidth;
    _height = defaultHeight;
  }
  _setHeight = _height;
  _setWidth = _width;

  if (client.getAttributeVector3 ( screenName, "screen_center", _setCenter )) {
    _center = _setCenter;
  } else {
    _setCenter = arVector3(0, 0, -5);
    _center = _setCenter;
  }
  
  if (!client.getAttributeVector3 ( screenName, "screen_normal", _normal )) {
    _normal = arVector3(0, 0, -1);
  }

  if (!client.getAttributeVector3 ( screenName, "screen_up", _up )) {
    _up = arVector3(0, 1, 0);
  }
  if (_normal.zero()) {
    ar_log_error() << "arGraphicsScreen overriding zero " << screenName << "/screen_normal.\n";
    _normal = arVector3(0, 0, -1);
  }
  _normal = _normal.normalize();
  if (_up.zero()) {
    ar_log_error() << "arGraphicsScreen overriding zero " << screenName << "/screen_up.\n";
    _up = arVector3(0, 1, 0);
  }
  _up = _up.normalize();

  int intbuf[4];
  if (ar_parseIntString(client.getAttribute(screenName, "tile"), intbuf, 4) == 4) {
    setTile( intbuf[0], intbuf[1], intbuf[2], intbuf[3] );
  } else {
    setTile( 0, 1, 0, 1 );
  }

  // demo-mode parameters
  if (!client.getAttributeVector3( screenName, "fixed_head_position", _fixedHeadPosition )) {
    _fixedHeadPosition = arVector3(0, 5.5, 0);
  }

  if (client.getAttributeFloats(screenName, "fixed_head_up_angle", &_fixedHeadUpAngle)) {
    _fixedHeadUpAngle *= M_PI/180.;
  } else {
    _fixedHeadUpAngle = 0.;
  }

  std::string fixModeString = client.getAttribute( screenName, "use_fixed_head_mode", "|allow|always|ignore|" );
  setUseFixedHeadMode( fixModeString );
  cout << "arGraphicsScreen demo parameters: " << _fixedHeadPosition << ", " 
       << _fixedHeadUpAngle << ", (" << _alwaysFixedHeadMode << "," << _ignoreFixedHeadMode << ").\n";

  return true;
}

bool arGraphicsScreen::setUseFixedHeadMode( const std::string& usageMode ) {
  if (usageMode == "allow") {
    _ignoreFixedHeadMode = false;
    _alwaysFixedHeadMode = false;
  } else if (usageMode == "always") {
    _ignoreFixedHeadMode = false;
    _alwaysFixedHeadMode = true;
  } else if (usageMode == "ignore") {
    _ignoreFixedHeadMode = true;
    _alwaysFixedHeadMode = false;
  } else {
    cerr << "arGraphicsScreen error: fixed head usage mode " << usageMode << " not recognized.\n";
    return false;
  }
  return true;
}

// Manually set the screen center. This can be nice to do if you are
// using the arGraphicsScreen directly instead of through one of the frameworks.
void arGraphicsScreen::setCenter(const arVector3& center) {
  _setCenter = center;
  _updateTileCenter();
}

// Manually set the screen normal. By convention, this points away from the
// viewer.
void arGraphicsScreen::setNormal(const arVector3& normal) {
  _normal = normal;
}

// Manually set the screen's up direction.
void arGraphicsScreen::setUp(const arVector3& up) {
  _up = up;
}

// Manually set the screen's dimensions, width followed by height.
void arGraphicsScreen::setDimensions(float width, float height) {
  _setWidth = width;
  _setHeight = height;
  _updateTileCenter();
}

// Manually set the screen's tile. Default is a single tile.
// @param tileX The number of the tile in the X direction, starting with 0.
// @param numberTilesX The total number of tiles in the X direction.
// @param tileY The number of the tile in the Y direction, starting with 0.
// @param numberTilesY The total number of tiles in the Y direction.
void arGraphicsScreen::setTile(int tileX, int numberTilesX,
                             int tileY, int numberTilesY) {
  _tileX = tileX;
  _numberTilesX = numberTilesX;
  _tileY = tileY;
  _numberTilesY = numberTilesY;
  _updateTileCenter();
}

void arGraphicsScreen::_updateTileCenter() {
  if (_numberTilesX > 1  ||  _numberTilesY > 1) {
    _center = _setCenter + ar_tileScreenOffset( _normal, _up, 
                                                _width, _height,
                                                _tileX, _numberTilesX, 
                                                _tileY, _numberTilesY );
    if (_numberTilesX != 0) {
      _width = _setWidth/_numberTilesX;
    }
    else{
      _width = _setWidth;
    }
    if (_numberTilesY != 0) {
      _height = _setHeight/_numberTilesY;
    }
    else{
      _height = _setHeight;
    }
  }
  else{
    _center = _setCenter;
    _height = _setHeight;
    _width = _setWidth;
  }
}

