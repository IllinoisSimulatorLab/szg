%Module _szg 0

%Feature EXPT

%ModuleHeaderCode
//#include "Python.h"
#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#define PY_SSIZE_T_MAX INT_MAX
#define PY_SSIZE_T_MIN INT_MIN
#endif
%End

// Defines mapping from python to C++ strings.
%Include typemaps.sip
%Include sockets.sip
%Include datautilities.sip
%Include data.sip
%Include logstream.sip
%Include database.sip
%Include math.sip
%Include navigation.sip
%Include phleet.sip
%Include szgclient.sip
%Include input.sip
%Include graphics.sip
%Include arfreeglut.sip
%Include mesh.sip
%Include scenegraphapi.sip
%Include view.sip
%Include gui.sip
%Include obj.sip
%Include sound.sip
%Include interact.sip
%Include frameworks.sip

%If (EXPT)
%Include szgexpt.sip
%End
