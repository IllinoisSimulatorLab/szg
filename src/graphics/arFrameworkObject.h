//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_FRAMEWORK_OBJECT_H
#define AR_FRAMEWORK_OBJECT_H

#include "arTemplateDictionary.h"
#include "arStructuredData.h"
#include "arGraphicsCalling.h"

#include <list>
using namespace std;

class SZG_CALL arFrameworkObject{
 public:
  arFrameworkObject();
  virtual ~arFrameworkObject() {};

  // Don't declare these functions as pure virtual.
  // That would force all subclasses to implement them.

  // Framework objects like head-wand simulator can draw themselves.
  // Not const, because of derived classes' implementations.
  virtual void draw() {}

  // Inelegant and lacking features?
  virtual void drawWithComposition() {}

  // Many framework objects can update their state periodically.
  virtual void update();

  // Return true iff update() has been run since the last dumpData(...)
  virtual bool changed() const
    { return _changed; }

  // Management of a network connection may be necessary,
  // e.g endian translation
  virtual void setRemoteStreamConfig(const arStreamConfig& c)
    { _remoteStreamConfig = c; }

  // Support a recursive start(): see subclass arMasterSlaveDataRouter.
  virtual bool start()
    { return true; }

  // Handle communications with the framework object.
  virtual arTemplateDictionary* getDictionary()
    { return &_dictionary; }
  virtual arStructuredData* dumpData();
  virtual bool receiveData(arStructuredData*)
    { return false; }

  // Set up compositing
  void preComposition(float lowerX, float lowerY, float widthX, float widthY);

  // Restore state after compositing
  void postComposition();

 protected:
  arTemplateDictionary _dictionary;
  arStreamConfig       _remoteStreamConfig;
  bool _changed;

  // Save state for postCompostion.  Ugly that these are
  // in so fundamental a base class.
  int _viewport[4];
  bool _lightingOn;
};

#endif
