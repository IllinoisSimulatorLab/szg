//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "arGraphicsHeader.h"
#include "arGraphicsPlugin.h"
#include "arSTLalgo.h"

// Definition of our drawable object.
class arTeapotGraphicsPlugin: public arGraphicsPlugin {
  public:
    //  Object MUST provide a default (0-argument) constructor.
    arTeapotGraphicsPlugin() {}
    virtual ~arTeapotGraphicsPlugin() {}

    // Draw the object. Currently, the object is responsible for restoring any
    // OpenGL state that it messes with.
    virtual void draw( arGraphicsWindow& win, arViewport& view );
    
    // Update the object's state based on changes to the database made
    // by the controller program. Feel free to ignore any of the arguments
    // that are not useful to you. The unused ones are commented out below
    // to keep pickier compilers from generating warnings about unused
    // arguments.
    virtual bool setState( std::vector<int>& /*intData*/,
                           std::vector<long>& /*longData*/,
                           std::vector<float>& floatData,
                           std::vector<double>& /*doubleData*/,
                           std::vector< std::string >& /*stringData*/ );
  private:
    // object-specific data.
    float _color[4];
};


// These are the only two functions that the plugin exposes.
// The plugin interface (in arGraphicsPluginNode.cpp) calls the 
// baseType() function to determine that yes, this shared library
// is in fact a graphics plugin. It then calls the factory() function
// to get an instance of the object that this plugin defines, and
// any further calls are made to methods of that instance.

extern "C" { // NOTE: These MUST have "C" linkage!
  SZG_CALL void baseType(char* buffer, int size) {
    ar_stringToBuffer("arGraphicsPlugin", buffer, size);
  }
  SZG_CALL void* factory(){
    return (void*) new arTeapotGraphicsPlugin();
  }
}


void arTeapotGraphicsPlugin::draw( arGraphicsWindow& win, arViewport& view ) {
  glPushAttrib( GL_CURRENT_BIT | GL_ENABLE_BIT );
  // Currently texturing is enabled when draw() is called. It shouldn't be.
  glDisable( GL_TEXTURE_2D );
  glColor4fv( _color );
  glutSolidTeapot( 1. );
  glPopAttrib();
}


bool arTeapotGraphicsPlugin::setState( std::vector<int>& /*intData*/,
                                       std::vector<long>& /*longData*/,
                                       std::vector<float>& floatData,
                                       std::vector<double>& /*doubleData*/,
                                       std::vector< std::string >& /*stringData*/ ) {
  if ((floatData.size() < 3)&&(floatData.size() > 4)) {
    ar_log_error() << "arTeapotGraphicsPlugin setState() received " << floatData.size()
                   << " floats. It needs either 3 or 4 for the color." << ar_endl;
    return false;
  }
  std::copy( floatData.begin(), floatData.end(), _color );
  if (floatData.size() == 4) {
    _color[3] = 1.;
  }
  return true;
}

