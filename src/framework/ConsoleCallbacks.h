//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef CONSOLE_CALLBACKS
#define CONSOLE_CALLBACKS

#include "arMath.h"

extern arMatrix4 _headMatrix;
extern arMatrix4 _wandMatrix;
extern int _button[6];
extern float _caveJoystickX;
extern float _caveJoystickY;

void console_drawInterface(float);
void console_keyboard(unsigned char, int, int, int);
void console_mouseMoved(int, int);
void console_mousePushed(int, int, int, int);
void console_init();

#endif
