//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arScreenObject.h"
#include "arGraphicsHeader.h"


// PLEASE NOTE: SINCE OUR DEFAULT WINDOW IS 640x480, WE MUST USE A WALL
// SIZE THAT IS DIFFERENT FROM THE CUBE FRONT WALL, i.e. 13.333 x 10.
arScreenObject::arScreenObject():
  _center( 0,5,-5 ),
  _normal( 0,0,-1 ),
  _up( 0,1,0 ),
  _height( 10 ),
  _width( 13.333 ),
  _tileX(0),
  _tileY(0),
  _numberTilesX(1),
  _numberTilesY(1),
  _nearClip(1),
  _farClip(1000),
  _midEyeOffset(-6./(2.54*12),0,0),
  _eyeDirection(1,0,0),
  _eyeSpacing(6/(2.54*12)),
  _unitConversion(1),
  _demoHeadUpAngle(0.),
  _useFixedHeadPos(false),
  _fixedHeadPos(0,0,0),
  _fixToHead(false),
  _eyeSign(0) {

  // make sure that _eyePosition and _localClipPlane are consistently
  // initialized
  setEyeInfo(0, ar_identityMatrix());
}

// If the attribute exists, stuff its value into v.
// Otherwise report v's (unchanged) default value.
// Sort of copypasted with arDistSceneGraphFramework::_getVector3.
void arScreenObject::_getVector3(arSZGClient* cli, arVector3& v, 
                                 const char* param) {
  stringstream& initResponse = cli->initResponse();
  if (!cli->getAttributeFloats(_screenName, param, v.v, 3)) {
    initResponse << "arScreenObject remark: " << _screenName 
                 << "/" << param << " defaulting to " << v << endl;
  }
}

bool arScreenObject::configure(arSZGClient* client){
  _screenName = client->getMode("graphics");
  return configure(_screenName, client);
}

// Always returns true.  There are sesnsible defaults for all parameters
// the arSZGClient might query.
bool arScreenObject::configure(const string& screenName, arSZGClient* cli){
  stringstream& initResponse = cli->initResponse();
  _screenName = screenName;
  _useFixedHeadPos =
    cli->getAttribute(screenName,"fixed_head","|false|true|") == "true";
  _fixToHead =
    cli->getAttribute(screenName,"head_mounted","|false|true|") == "true";

  if (cli->getAttributeFloats(screenName,"fixed_head_up_angle", 
                              &_demoHeadUpAngle)){
    // mysterious scaling factor from kilopascals to furlongs
    _demoHeadUpAngle *= 0.0174532928; 
  }
  else{
    initResponse << "arScreenObject remark: "
		 << screenName << "/fixed_head_up_angle "
		 << "defaulting to " << _demoHeadUpAngle << " degrees.\n";
  }

  _getVector3(cli, _fixedHeadPos, "fixed_head_pos");

  // Eye/head parameters, copypasted with 
  // arDistSceneGraphFramework::_loadParameters().
  if (!cli->getAttributeFloats(screenName,"eye_spacing",&_eyeSpacing)){
    initResponse << "arScreenObject remark: "
		 << screenName << "/eye_spacing "
		 << "defaulting to " << _eyeSpacing << endl;
  }
  _getVector3(cli, _eyeDirection, "eye_direction");
  _getVector3(cli, _midEyeOffset, "mid_eye_offset");

  float floatbuf[2];
  if (cli->getAttributeFloats(screenName,"screen_dim",floatbuf,2)) {
    _width = floatbuf[0];
    _height = floatbuf[1];
    _getVector3(cli, _center, "screen_center");
    _getVector3(cli, _normal, "screen_normal");
    _getVector3(cli, _up, "screen_up");
  }
  else{
    // we must have reasonable defaults, 
    // these are for the syzygy "front wall", on
    // which all demos show something sensible upon start up

    // PLEASE NOTE: SINCE OUR DEFAULT WINDOW IS 640x480, WE MUST USE A WALL
    // SIZE THAT IS DIFFERENT FROM THE CUBE FRONT WALL, i.e. 13.333 x 10.
    _width = 13.333;
    _height = 10;
    _center = arVector3(0,5,-5);
    _normal = arVector3(0,0,-1);
    _up = arVector3(0,1,0);
  }
  _normal = _normal.normalize();
  _up = _up.normalize();

  int intbuf[4];
  if (ar_parseIntString(cli->getAttribute(screenName,"tile"), intbuf, 4) 
      == 4) {
    _tileX = intbuf[0];
    _numberTilesX = intbuf[1];
    _tileY = intbuf[2];
    _numberTilesY = intbuf[3];
    if (_numberTilesX > 1  ||  _numberTilesY > 1)
      _center += ar_tileScreenOffset( _center, _normal, _up, _width, _height,
        _tileX, _numberTilesX, _tileY, _numberTilesY );
  }
  else{
    // reasonable defaults, as above
    _tileX = 0;
    _numberTilesX = 1;
    _tileY = 0;
    _numberTilesY = 1;
  }

  return true;
}

/// Allows us to get the projection matrix as computed by this screen object,
/// without putting it directly on the OpenGL matrix stack. This is useful
/// for authoring applications that use a different graphics engine than
/// OpenGL
arMatrix4 arScreenObject::getProjectionMatrix(){
  if (!_fixToHead) {
    return ar_frustumMatrix( _unitConversion*_center, _normal, _up, 
                             _unitConversion*0.5*_width,
                             _unitConversion*0.5*_height,
                             _localNearClip, _farClip,
                             _eyePosition );
  } else {
    const arVector3 eyeOffsetVector = _midEyeOffset +
      0.5 *_eyeSign * _eyeSpacing * _eyeDirection.normalize();
    arVector3 center( _unitConversion*(_locHeadMatrix*(_center+eyeOffsetVector)) ); 
    arMatrix4 orient(ar_extractRotationMatrix(_locHeadMatrix)); 
    arVector3 normal( orient*_normal ); 
    arVector3 up( orient*_up ); 
    return ar_frustumMatrix( center, normal, up, 
                             _unitConversion*0.5*_width,
                             _unitConversion*0.5*_height,
                             _localNearClip, _farClip,
                             _eyePosition );
  }
}

/// Allows us to get the modelview matrix as computed by this screen object,
/// without putting it directly on the OpenGL matrix stack. This is useful
/// for authoring applications that use a different engine than OpenGL.
arMatrix4 arScreenObject::getModelviewMatrix(){
  if (!_fixToHead) {
    return ar_lookatMatrix(_eyePosition, _eyePosition+_normal, _up);
  } else {
    arMatrix4 orient(ar_extractRotationMatrix(_locHeadMatrix)); 
    arVector3 normal( orient*_normal ); 
    arVector3 up( orient*_up ); 
    return ar_lookatMatrix(_eyePosition, _eyePosition+normal, up);
  }
}

/// Puts the viewing matrices on the OpenGL stack.
void arScreenObject::loadViewMatrices(){
  glMatrixMode( GL_PROJECTION );
  glLoadIdentity();
  arMatrix4 temp = getProjectionMatrix();
  glMultMatrixf( temp.v );

  glMatrixMode( GL_MODELVIEW );
  glLoadIdentity();
  temp = getModelviewMatrix();
  glMultMatrixf( temp.v );
}

/// Computes _eyePosition and _localNearClip (internal values used in
/// determining the camera matrices).
void arScreenObject::setEyeInfo(float eyeSign, 
				const arMatrix4& headMatrix){
  // First of all, move into internal storage.
  _eyeSign = eyeSign;
  _headMatrix = headMatrix;

  // Next, go ahead and use this information to compute the two quantities
  // that are used, internally, to compute the viewing matrices, namely
  // _eyePosition and _localNearClip

  // NOTE: the head matrix we use may be different from the head matrix
  // given. We might be in "demo" mode.
  _locHeadMatrix 
    = _useFixedHeadPos ? _demoHeadMatrix(_headMatrix) : _headMatrix;
  const arVector3 eyeOffsetVector = _midEyeOffset +
    0.5 *_eyeSign * _eyeSpacing * _eyeDirection.normalize();
  // The eye position is a quantity we need computed.
  _eyePosition = _unitConversion * (_locHeadMatrix * eyeOffsetVector);

  const arVector3 midEyePosition 
    = _unitConversion * (_locHeadMatrix * _midEyeOffset);
  const arVector3 zHat = _normal;
  // '%' = dot product
  const float zEye = (_eyePosition - midEyePosition) % zHat; 
  // If this eye is the closer to the screen, use nearClip; if not, increase
  // _nearClip so it corresponds to the same physical plane.
  // This is the other quantity that needs to be computed. We are making sure
  // that 2 stereo eyes use the same clipping plane.
  _localNearClip = zEye >= 0. ? _nearClip : _nearClip-2*zEye;
}

/// A deprecated interface provided for backwards code compatibility.
/// It combines setEyeInfo(...) and getProjectionMatrix().
arMatrix4 arScreenObject::getProjectionMatrix(const float eyeSign,
                                              const arMatrix4& headMatrix){
  setEyeInfo(eyeSign, headMatrix);
  return getProjectionMatrix();
}

/// A deprecated interface provided for backwards code compatibility.
/// It combines setEyeinfo(...) and getModelviewMatrix().
arMatrix4 arScreenObject::getModelviewMatrix(const float eyeSign,
                                             const arMatrix4& headMatrix){
  setEyeInfo(eyeSign, headMatrix);
  return getModelviewMatrix();
}

/// A deprecated interface provided for backwards code compatibility.
/// It combines setEyeInfo(...) and loadViewMatrices().
void arScreenObject::loadViewMatrices( const float eyeSign,
				       const arMatrix4& headMatrix ) {
  setEyeInfo(eyeSign, headMatrix);
  loadViewMatrices();
}

arMatrix4 arScreenObject::_demoHeadMatrix( const arMatrix4& headMatrix ) {
  const arVector3 demoHeadPos = _useFixedHeadPos ?
                        _fixedHeadPos : ar_extractTranslation(headMatrix);
  const arVector3 zHat = _normal.normalize();
  const arVector3 xHat = zHat * _up.normalize();  // '*' = cross product
  const arVector3 yHat = xHat * zHat;
  const arVector3 yPrime 
    = cos(_demoHeadUpAngle)*yHat - sin(_demoHeadUpAngle)*xHat;
  const arVector3 xPrime = zHat * yPrime;

  arMatrix4 demoRotMatrix;
  int j = 0;
  for (int i=0; i<9; i+=4) {
    demoRotMatrix[i] = xPrime.v[j];
    demoRotMatrix[i+1] = yPrime.v[j];
    demoRotMatrix[i+2] = zHat.v[j++];
  }
  return ar_translationMatrix(demoHeadPos) * demoRotMatrix;
}

void arScreenObject::setViewTransform( float nearClip, float farClip, 
                                       float unitConversion, float eyeSpacing, 
                                       const arVector3& midEyeOffset, 
                                       const arVector3& eyeDirection) {
  setClipPlanes(nearClip, farClip);
  setUnitConversion(unitConversion);
  setEyeSpacing(eyeSpacing);
  _midEyeOffset = midEyeOffset;
  _eyeDirection = eyeDirection;
}

/// Manually set the screen center. This can be nice to do if you are
/// using the arScreenObject directly instead of through one of the frameworks.
void arScreenObject::setCenter(const arVector3& center){
  _center = center;
}

/// Manually set the screen normal. By convention, this points away from the
/// viewer.
void arScreenObject::setNormal(const arVector3& normal){
  _normal = normal;
}

/// Manually set the screen's up direction.
void arScreenObject::setUp(const arVector3& up){
  _up = up;
}

/// Manually set the screen's dimensions, width followed by height.
void arScreenObject::setDimensions(float width, float height){
  _width = width;
  _height = height;
}

/// Manually set the screen's tile. Default is a single tile.
/// @param tileX The number of the tile in the X direction, starting with 0.
/// @param numberTilesX The total number of tiles in the X direction.
/// @param tileY The number of the tile in the Y direction, starting with 0.
/// @param numberTilesY The total number of tiles in the Y direction.
void arScreenObject::setTile(int tileX, int numberTilesX,
			     int tileY, int numberTilesY){
  _tileX = tileX;
  _numberTilesX = numberTilesX;
  _tileY = tileY;
  _numberTilesY = numberTilesY;
}

/// Manually set the clipping planes, near followed by far.
void arScreenObject::setClipPlanes(float nearClip, float farClip) {
  _nearClip = nearClip;
  _farClip = farClip;
}

void arScreenObject::setUnitConversion(float unitConversion) {
  _unitConversion = unitConversion;
}

void arScreenObject::setEyeSpacing(float feet) {
  _eyeSpacing = feet;
}
