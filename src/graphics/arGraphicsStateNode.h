//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef AR_GRAPHICS_STATE_NODE_H
#define AR_GRAPHICS_STATE_NODE_H

#include "arGraphicsNode.h"
#include "arMath.h"
#include "arGraphicsCalling.h"

// Let the programmer control some OpenGL state:
// point size, line width, shade model, lighting (on/off),
// depth test (on/off), blend (on/off), and blend function (all OpenGL values).

class SZG_CALL arGraphicsStateNode: public arGraphicsNode{
 public:
  arGraphicsStateNode();
  virtual ~arGraphicsStateNode();

  void draw(arGraphicsContext*);

  arStructuredData* dumpData();
  bool receiveData(arStructuredData*);

  // Data access functions specific to arGraphicsStateNode.

  // Returns the human-readable name.
  string getStateName();
  // Returns the state ID, which is used in updating the arGraphicsContext during database drawing.
  arGraphicsStateID getStateID();
  bool isFloatState();
  bool isFloatState(const string& stateName);
  // If this particular graphics state DOES NOT have int values, return
  // false. Otherwise, return true and pack the parameters appropriately.
  bool getStateValuesInt(arGraphicsStateValue& value1,
                         arGraphicsStateValue& value2);
  // A getter that is more compatible with Python.
  arGraphicsStateValue getStateValueInt(int i);
  string getStateValueString(int i);
  // If this particular graphics state DOES NOT have a float value,
  // return false. Otherwise, return true and pack the parameter appropriately.
  bool getStateValueFloat(float& value);
  // A getter that is more compatible with Python.
  float getStateValueFloat();

  // Setters corresponding to above.
  bool setGraphicsStateInt(const string& stateName,
                           arGraphicsStateValue value1,
                           arGraphicsStateValue value2 = AR_G_FALSE);
  bool setGraphicsStateString(const string& stateName,
                              const string& value1,
                              const string& value2 = "false");
  bool setGraphicsStateFloat(const string& stateName,
                             float stateValueFloat);

 protected:
  // Each graphics state node embodies only a single change to graphics
  // state. The idea is that it won't be *too* often that people want to
  // change away from the default state... so it's OK if there is a little
  // inefficiency.

  // This designates the human-readable string for the particular piece of
  // graphics state. point_size, line_width, shade_model, etc. These
  // are only used when getting/setting state. For internal operations
  // (like drawing the database), a state ID is used.
  string               _stateName;
  // The state's ID. This starts out as AR_G_GARBAGE_STATE before the node
  // has been altered.
  arGraphicsStateID    _stateID;
  // Some states have bool values (here represented as 0/1). For instance,
  // lighting is either enabled or disabled. Some have
  // arGraphicsStateValue (an enum) as a single value, like shade_model.
  // Some 2 arGraphicsStateValues, like blend_func, which has both source
  // and destination blending parameters.
  arGraphicsStateValue _stateValueInt[2];
  // Some states have float values, like point_size or line_width.
  float                _stateValueFloat;

  arGraphicsStateID _convertStringToStateID(const string& stateName);
  string _convertStateIDToString(arGraphicsStateID id);
  arGraphicsStateValue _convertStringToStateValue(const string& stateValue);
  string _convertStateValueToString(arGraphicsStateValue v);
  bool _checkFloatState( arGraphicsStateID id ) const;
  bool _isFloatState() const { return _checkFloatState(_stateID); }

  arStructuredData* _dumpData(const string& stateName,
                              arGraphicsStateValue* stateValueInt,
                              float stateValueFloat,
                              bool owned);
};

#endif
