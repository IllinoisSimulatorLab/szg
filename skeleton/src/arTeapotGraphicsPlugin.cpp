//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#include "arPrecompiled.h"
#include "arGlut.h"
#include "arGraphicsPlugin.h"
#include "arSTLalgo.h"

// Definition of the drawable object.
class arTeapotGraphicsPlugin: public arGraphicsPlugin {
  public:
    //  Object must provide a default (0-argument) constructor.
    arTeapotGraphicsPlugin() {}
    virtual ~arTeapotGraphicsPlugin() {}

    // Draw the object.  Restore any modified OpenGL state.
    virtual void draw( arGraphicsWindow& win, arViewport& view );

    // Update the object's state based on changes to the database made
    // by the controller program. Feel free to ignore any of the arguments.
    // Unused ones are commented out to avoid compiler warnings.
    virtual bool setState( std::vector<int>& /*intData*/,
                           std::vector<long>& /*longData*/,
                           std::vector<float>& floatData,
                           std::vector<double>& /*doubleData*/,
                           std::vector< std::string >& /*stringData*/ );
  private:
    // Object-specific data.
    float _color[4];
};

// The plugin exposes only these two functions.
// The plugin interface (in arGraphicsPluginNode.cpp) calls baseType()
// to verify that this shared library is a graphics plugin.
// It then calls factory() to instantiate an object.
// Further calls are made to that instance's methods.
#undef SZG_IMPORT_LIBRARY
#undef SZG_DO_NOT_EXPORT
#include "arCallingConventions.h"
extern "C" {
  SZG_CALL void baseType(char* buffer, int size)
    { ar_stringToBuffer("arGraphicsPlugin", buffer, size); }
  SZG_CALL void* factory()
    { return (void*) new arTeapotGraphicsPlugin(); }
}

void arTeapotGraphicsPlugin::draw( arGraphicsWindow&, arViewport& ) {
  glPushAttrib( GL_CURRENT_BIT | GL_ENABLE_BIT );
    // Texturing should be disabled during draw().
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
  const unsigned numFloats = floatData.size();
  if (numFloats != 3 && numFloats != 4) {
    ar_log_error() << "arTeapotGraphicsPlugin setState() expected 3 or 4 floats, not "
      << floatData.size() << ".\n";
    return false;
  }

  std::copy( floatData.begin(), floatData.end(), _color );
  if (numFloats == 4) {
    _color[3] = 1.;
  }
  return true;
}
