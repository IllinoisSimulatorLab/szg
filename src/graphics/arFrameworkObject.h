//********************************************************
// Syzygy source code is licensed under the GNU LGPL
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FRAMEWORK_OBJECT_H
#define AR_FRAMEWORK_OBJECT_H

#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include <list>
using namespace std;

class SZG_CALL arFrameworkObject{
 public:
  arFrameworkObject();
  virtual ~arFrameworkObject(){};

  // don't declare these functions as pure virtual-- that would force all
  // subcalsses to implement.

  // Many framework objects can draw themselves. (think about the head-wand
  // simulator from the demo frameworks)
  virtual void draw(){}
  // Rather annoying to have this function here.... Just seems inelegant.
  // Also, it fails to be sufficiently feature-ful.
  virtual void drawWithComposition(){}
  // Many framework objects can update their state periodically.
  virtual void update();
  // Should return true if update() has been run since the last dumpData(...)
  virtual bool changed(){ return _changed; }
  // Management of a network connection may be necessary. (i.e. for
  // little-endian vs. big-endian translation
  virtual void setRemoteStreamConfig(const arStreamConfig& c){
    _remoteStreamConfig = c;
  }
  // We have tp be able to do a recursive start() if needed (see subclass
  // arMasterSlaveDataRouter.
  virtual bool start(){ return true; }
  // The next three functions deal with communications to and from the
  // framework object.
  virtual arTemplateDictionary* getDictionary(){ return &_dictionary; }
  virtual arStructuredData* dumpData();
  virtual bool receiveData(arStructuredData*){ return false; }

  // This function sets up compositing
  void preComposition(float lowerX, float lowerY, float widthX, float widthY);
  // Returns state to normal after compositing
  void postComposition();
 protected:
  arTemplateDictionary _dictionary;
  arStreamConfig       _remoteStreamConfig;
  bool _changed;
  // the preCompostion function needs to save some things for use
  // by the postCompostion...
  // Grrrr.... These shouldn't really be in this fundamental of a base class.
  int _viewport[4];
  bool _lightingOn;
};

#endif
