//********************************************************
// Syzygy is licensed under the BSD license v2
// see the file SZG_CREDITS for details
//********************************************************

#ifndef _AR_GUI_DEFINES_H
#define _AR_GUI_DEFINES_H

#include "arGraphicsCalling.h"

/**
 * OS events.
 */
SZG_CALL enum arGUIEventType
{
  AR_GENERIC_EVENT,         // Placeholder event (for default constructors).
  AR_KEY_EVENT,             // A keyboard event.
  AR_MOUSE_EVENT,           // A mouse event.
  AR_WINDOW_EVENT,          // A window event.
  AR_NUM_GUI_EVENTS         // The number of different OS events.
};

/**
 * States that an OS event can be in.
 *
 * Also used as the 'to-do' message detail in passing requests from
 * arGUIWindowManager to arGUIWindow (i.e., in this case AR_WINDOW_MOVE means
 * "move the window" rather than its normal "the window has moved").
 */
SZG_CALL enum arGUIState
{
  AR_GENERIC_STATE,         // Placeholder state (for default constructors).
  AR_KEY_DOWN,              // A key has been pressed.
  AR_KEY_UP,                // A key has been released.
  AR_KEY_REPEAT,            // A key was pressed and is continuing to be pressed.
  AR_MOUSE_DOWN,            // A mouse button was pressed.
  AR_MOUSE_UP,              // A mouse button was released.
  AR_MOUSE_MOVE,            // The mouse has moved.
  AR_MOUSE_DRAG,            // A mouse button is held down and the mouse is being moved.
  AR_WINDOW_MOVE,           // The window has been moved.
  AR_WINDOW_RESIZE,         // The window has been resized.
  AR_WINDOW_CLOSE,          // The window has been closed.
  AR_WINDOW_FULLSCREEN,     // Change the window to fullscreen.
  AR_WINDOW_DECORATE,       // Change the window's decoration state.
  AR_WINDOW_RAISE,          // Change the window's z order.
  AR_WINDOW_CURSOR,         // Change the window's cursor.
  AR_WINDOW_DRAW,           // Draw the window.
  AR_WINDOW_SWAP,           // Swap the window's buffers.
  AR_WINDOW_VIEWPORT,       // Set the window's viewport.
  AR_WINDOW_INITGL,         // Initialize the window's opengl context.
  AR_NUM_GUI_STATES         // The number of different event states.
};

/**
 * Struct used in modifying Motif window manager properties.
 *
 * @note Used to replace dependency on the Xm/MwmUtils.h header.
 */
typedef struct {
  unsigned long flags;
  unsigned long functions;
  unsigned long decorations;
  long input_mode;
  unsigned long status;
} MotifWmHints;

//@{
/**
 * @name Convenience typedef's
 *
 * For use by arGUIEventManager functions.
 */
typedef unsigned int arGUIKey;
typedef unsigned int arGUIButton;
typedef unsigned int arCursor;
typedef unsigned int arZOrder;
//@}


#if defined( AR_USE_WIN_32 )

// DO NOT INCLUDE windows.h here. Instead, do as below.
#include "arPrecompiled.h"

  //@{
  /**
   * @name Mappings from Win32 specific virtual key codes
   *
   * @note There is some discrepancy between how Win32 and X11 handle
   *       modifiers such as shift, ctrl and alt and how they differentiate
   *       between left and right versions of such.
   */
  #define AR_VK_GARBAGE  int( 0 )
  #define AR_VK_A        int( 'A' )
  #define AR_VK_B        int( 'B' )
  #define AR_VK_C        int( 'C' )
  #define AR_VK_D        int( 'D' )
  #define AR_VK_E        int( 'E' )
  #define AR_VK_F        int( 'F' )
  #define AR_VK_G        int( 'G' )
  #define AR_VK_H        int( 'H' )
  #define AR_VK_I        int( 'I' )
  #define AR_VK_J        int( 'J' )
  #define AR_VK_K        int( 'K' )
  #define AR_VK_L        int( 'L' )
  #define AR_VK_M        int( 'M' )
  #define AR_VK_N        int( 'N' )
  #define AR_VK_O        int( 'O' )
  #define AR_VK_P        int( 'P' )
  #define AR_VK_Q        int( 'Q' )
  #define AR_VK_R        int( 'R' )
  #define AR_VK_S        int( 'S' )
  #define AR_VK_T        int( 'T' )
  #define AR_VK_U        int( 'U' )
  #define AR_VK_V        int( 'V' )
  #define AR_VK_W        int( 'W' )
  #define AR_VK_X        int( 'X' )
  #define AR_VK_Y        int( 'Y' )
  #define AR_VK_Z        int( 'Z' )
  #define AR_VK_a        int( 'a' )
  #define AR_VK_b        int( 'b' )
  #define AR_VK_c        int( 'c' )
  #define AR_VK_d        int( 'd' )
  #define AR_VK_e        int( 'e' )
  #define AR_VK_f        int( 'f' )
  #define AR_VK_g        int( 'g' )
  #define AR_VK_h        int( 'h' )
  #define AR_VK_i        int( 'i' )
  #define AR_VK_j        int( 'j' )
  #define AR_VK_k        int( 'k' )
  #define AR_VK_l        int( 'l' )
  #define AR_VK_m        int( 'm' )
  #define AR_VK_n        int( 'n' )
  #define AR_VK_o        int( 'o' )
  #define AR_VK_p        int( 'p' )
  #define AR_VK_q        int( 'q' )
  #define AR_VK_r        int( 'r' )
  #define AR_VK_s        int( 's' )
  #define AR_VK_t        int( 't' )
  #define AR_VK_u        int( 'u' )
  #define AR_VK_v        int( 'v' )
  #define AR_VK_w        int( 'w' )
  #define AR_VK_x        int( 'x' )
  #define AR_VK_y        int( 'y' )
  #define AR_VK_z        int( 'z' )
  #define AR_VK_0        int( '0' )
  #define AR_VK_1        int( '1' )
  #define AR_VK_2        int( '2' )
  #define AR_VK_3        int( '3' )
  #define AR_VK_4        int( '4' )
  #define AR_VK_5        int( '5' )
  #define AR_VK_6        int( '6' )
  #define AR_VK_7        int( '7' )
  #define AR_VK_8        int( '8' )
  #define AR_VK_9        int( '9' )
  #define AR_VK_BANG     int( '!' )
  #define AR_VK_AT       int( '@' )
  #define AR_VK_POUND    int( '#' )
  #define AR_VK_DOLLAR   int( '$' )
  #define AR_VK_PERCENT  int( '%' )
  #define AR_VK_CARET    int( '^' )
  #define AR_VK_AMP      int( '&' )
  #define AR_VK_ASTERISK int( '*' )
  #define AR_VK_LPAREN   int( '(' )
  #define AR_VK_RPAREN   int( ')' )
  #define AR_VK_LBRACE   int( '{' )
  #define AR_VK_RBRACE   int( '}' )
  #define AR_VK_LBRACKET int( '[' )
  #define AR_VK_RBRACKET int( ']' )
  #define AR_VK_QUESTION int( '?' )
  #define AR_VK_FSLASH   int( '/' )
  #define AR_VK_BSLASH   int( '\\' )
  #define AR_VK_PIPE     int( '|' )
  #define AR_VK_PLUS     int( '+' )
  #define AR_VK_EQUALS   int( '=' )
  #define AR_VK_TILDE    int( '~' )
  #define AR_VK_APOST    int( '`' )
  #define AR_VK_SQUOTE   int( '\'' )
  #define AR_VK_DQUOTE   int( '"' )
  #define AR_VK_LT       int( '<' )
  #define AR_VK_GT       int( '>' )
  #define AR_VK_COMMA    int( ',' )
  #define AR_VK_PERIOD   int( '.' )
  #define AR_VK_COLON    int( ':' )
  #define AR_VK_SCOLON   int( ';' )
  #define AR_VK_ADD      VK_ADD
  #define AR_VK_SUBTRACT VK_SUBTRACT
  #define AR_VK_MULTIPLY VK_MULTIPLY
  #define AR_VK_DIVIDE   VK_DIVIDE
  #define AR_VK_DECIMAL  VK_DECIMAL
  #define AR_VK_SEP      VK_SEPARATOR
  #define AR_VK_NUMPAD0  VK_NUMPAD0
  #define AR_VK_NUMPAD1  VK_NUMPAD1
  #define AR_VK_NUMPAD2  VK_NUMPAD2
  #define AR_VK_NUMPAD3  VK_NUMPAD3
  #define AR_VK_NUMPAD4  VK_NUMPAD4
  #define AR_VK_NUMPAD5  VK_NUMPAD5
  #define AR_VK_NUMPAD6  VK_NUMPAD6
  #define AR_VK_NUMPAD7  VK_NUMPAD7
  #define AR_VK_NUMPAD8  VK_NUMPAD8
  #define AR_VK_NUMPAD9  VK_NUMPAD9
  #define AR_VK_BACK     VK_BACK
  #define AR_VK_TAB      VK_TAB
  #define AR_VK_RETURN   VK_RETURN
  #define AR_VK_SHIFT    VK_SHIFT
  #define AR_VK_LSHIFT   VK_LSHIFT
  #define AR_VK_RSHIFT   VK_RSHIFT
  #define AR_VK_CTRL     VK_CONTROL
  #define AR_VK_LCTRL    VK_LCONTROL
  #define AR_VK_RCTRL    VK_RCONTROL
  #define AR_VK_ALT      VK_MENU
  #define AR_VK_LALT     VK_LMENU
  #define AR_VK_RALT     VK_RMENU
  #define AR_VK_PAUSE    VK_PAUSE
  #define AR_VK_ESC      VK_ESCAPE
  #define AR_VK_SPACE    VK_SPACE
  #define AR_VK_PGUP     VK_PRIOR
  #define AR_VK_PGDN     VK_NEXT
  #define AR_VK_END      VK_END
  #define AR_VK_HOME     VK_HOME
  #define AR_VK_LEFT     VK_LEFT
  #define AR_VK_RIGHT    VK_RIGHT
  #define AR_VK_UP       VK_UP
  #define AR_VK_DOWN     VK_DOWN
  #define AR_VK_INS      VK_INSERT
  #define AR_VK_DEL      VK_DELETE
  #define AR_VK_F1       VK_F1
  #define AR_VK_F2       VK_F2
  #define AR_VK_F3       VK_F3
  #define AR_VK_F4       VK_F4
  #define AR_VK_F5       VK_F5
  #define AR_VK_F6       VK_F6
  #define AR_VK_F7       VK_F7
  #define AR_VK_F8       VK_F8
  #define AR_VK_F9       VK_F9
  #define AR_VK_F10      VK_F10
  #define AR_VK_F11      VK_F11
  #define AR_VK_F12      VK_F12
  #define AR_VK_NUMLOCK  VK_NUMLOCK
  #define AR_VK_SCROLL   VK_SCROLL
  //@}

  //@{
  /**
   * @name Mouse virtual 'key' codes.
   *
   * @note Chosen such that functions can do bitwise logic on them.
   *
   * @todo Support for more than 3 mouse buttons.
   */
  #define AR_LBUTTON         0x0001
  #define AR_MBUTTON         0x0010
  #define AR_RBUTTON         0x0100
  #define AR_BUTTON_GARBAGE  0x0000
  //@}

  //@{
  /**
   * @name Cursor virtual 'key' codes.
   *
   * @todo Support for more cursor types.
   */
  #define AR_CURSOR_ARROW    0x0000
  #define AR_CURSOR_HELP     0x0001
  #define AR_CURSOR_WAIT     0x0002
  #define AR_CURSOR_NONE     0x0003
  //@}

  //@{
  /**
   * @name Z ordering virtual 'key' codes.
   *
   * @todo Support for different z orderings.
   */
  #define AR_ZORDER_NORMAL   0x0000
  #define AR_ZORDER_TOP      0x0001
  #define AR_ZORDER_TOPMOST  0x0002
  //@}

#elif defined( AR_USE_LINUX ) || defined( AR_USE_DARWIN ) || defined( AR_USE_SGI )

  #include <X11/keysym.h>

  //@{
  /**
   * @name Mapping from X11 specific virtual key codes
   *
   * @note There is some discrepancy between how Win32 and X11 handle
   *       modifiers such as shift, ctrl and alt and how they differentiate
   *       between left and right versions of such.
   */
  #define AR_VK_GARBAGE  XK_VoidSymbol
  #define AR_VK_A        XK_A
  #define AR_VK_B        XK_B
  #define AR_VK_C        XK_C
  #define AR_VK_D        XK_D
  #define AR_VK_E        XK_E
  #define AR_VK_F        XK_F
  #define AR_VK_G        XK_G
  #define AR_VK_H        XK_H
  #define AR_VK_I        XK_I
  #define AR_VK_J        XK_J
  #define AR_VK_K        XK_K
  #define AR_VK_L        XK_L
  #define AR_VK_M        XK_M
  #define AR_VK_N        XK_N
  #define AR_VK_O        XK_O
  #define AR_VK_P        XK_P
  #define AR_VK_Q        XK_Q
  #define AR_VK_R        XK_R
  #define AR_VK_S        XK_S
  #define AR_VK_T        XK_T
  #define AR_VK_U        XK_U
  #define AR_VK_V        XK_V
  #define AR_VK_W        XK_W
  #define AR_VK_X        XK_X
  #define AR_VK_Y        XK_Y
  #define AR_VK_Z        XK_Z
  #define AR_VK_a        XK_a
  #define AR_VK_b        XK_b
  #define AR_VK_c        XK_c
  #define AR_VK_d        XK_d
  #define AR_VK_e        XK_e
  #define AR_VK_f        XK_f
  #define AR_VK_g        XK_g
  #define AR_VK_h        XK_h
  #define AR_VK_i        XK_i
  #define AR_VK_j        XK_j
  #define AR_VK_k        XK_k
  #define AR_VK_l        XK_l
  #define AR_VK_m        XK_m
  #define AR_VK_n        XK_n
  #define AR_VK_o        XK_o
  #define AR_VK_p        XK_p
  #define AR_VK_q        XK_q
  #define AR_VK_r        XK_r
  #define AR_VK_s        XK_s
  #define AR_VK_t        XK_t
  #define AR_VK_u        XK_u
  #define AR_VK_v        XK_v
  #define AR_VK_w        XK_w
  #define AR_VK_x        XK_x
  #define AR_VK_y        XK_y
  #define AR_VK_z        XK_z
  #define AR_VK_0        XK_0
  #define AR_VK_1        XK_1
  #define AR_VK_2        XK_2
  #define AR_VK_3        XK_3
  #define AR_VK_4        XK_4
  #define AR_VK_5        XK_5
  #define AR_VK_6        XK_6
  #define AR_VK_7        XK_7
  #define AR_VK_8        XK_8
  #define AR_VK_9        XK_9
  #define AR_VK_BANG     XK_exclam
  #define AR_VK_AT       XK_at
  #define AR_VK_POUND    XK_numbersign
  #define AR_VK_DOLLAR   XK_dollar
  #define AR_VK_PERCENT  XK_percent
  #define AR_VK_CARET    XK_asciicircum
  #define AR_VK_AMP      XK_ampersand
  #define AR_VK_ASTERISK XK_asterisk
  #define AR_VK_LPAREN   XK_parenleft
  #define AR_VK_RPAREN   XK_parenright
  #define AR_VK_LBRACE   XK_braceleft
  #define AR_VK_RBRACE   XK_braceright
  #define AR_VK_LBRACKET XK_bracketleft
  #define AR_VK_RBRACKET XK_bracketright
  #define AR_VK_QUESTION XK_question
  #define AR_VK_FSLASH   XK_slash
  #define AR_VK_BSLASH   XK_backslash
  #define AR_VK_PIPE     XK_bar
  #define AR_VK_PLUS     XK_plus
  #define AR_VK_EQUALS   XK_equal
  #define AR_VK_TILDE    XK_asciitilde
  #define AR_VK_APOST    XK_apostrophe
  #define AR_VK_SQUOTE   XK_quoteright
  #define AR_VK_DQUOTE   XK_quotedbl
  #define AR_VK_LT       XK_less
  #define AR_VK_GT       XK_greater
  #define AR_VK_COMMA    XK_comma
  #define AR_VK_PERIOD   XK_period
  #define AR_VK_COLON    XK_colon
  #define AR_VK_SCOLON   XK_semicolon
  #define AR_VK_ADD      XK_KP_Add
  #define AR_VK_SUBTRACT XK_KP_Subtract
  #define AR_VK_MULTIPLY XK_KP_Multiply
  #define AR_VK_DIVIDE   XK_KP_Divide
  #define AR_VK_DECIMAL  XK_KP_Decimal
  #define AR_VK_SEP      XK_KP_Separator
  #define AR_VK_NUMPAD0  XK_KP_0
  #define AR_VK_NUMPAD1  XK_KP_1
  #define AR_VK_NUMPAD2  XK_KP_2
  #define AR_VK_NUMPAD3  XK_KP_3
  #define AR_VK_NUMPAD4  XK_KP_4
  #define AR_VK_NUMPAD5  XK_KP_5
  #define AR_VK_NUMPAD6  XK_KP_6
  #define AR_VK_NUMPAD7  XK_KP_7
  #define AR_VK_NUMPAD8  XK_KP_8
  #define AR_VK_NUMPAD9  XK_KP_9
  #define AR_VK_BACK     XK_BackSpace
  #define AR_VK_TAB      XK_Tab
  #define AR_VK_RETURN   XK_Return
  // #define AR_VK_SHIFT    ( XK_Shift_L | XK_Shift_R )
  #define AR_VK_LSHIFT   XK_Shift_L
  #define AR_VK_RSHIFT   XK_Shift_R
  // #define AR_VK_CTRL     ( XK_Control_L | XK_Control_R )
  #define AR_VK_LCTRL    XK_Control_L
  #define AR_VK_RCTRL    XK_Control_R
  // #define AR_VK_ALT      ( XK_Alt_L | XK_Alt_R )
  #define AR_VK_LALT     XK_Alt_L
  #define AR_VK_RALT     XK_Alt_R
  #define AR_VK_PAUSE    XK_Pause
  #define AR_VK_ESC      XK_Escape
  #define AR_VK_SPACE    XK_space
  #define AR_VK_PGUP     XK_Page_Up
  #define AR_VK_PGDN     XK_Page_Down
  #define AR_VK_END      XK_End
  #define AR_VK_HOME     XK_Home
  #define AR_VK_LEFT     XK_Left
  #define AR_VK_RIGHT    XK_Right
  #define AR_VK_UP       XK_Up
  #define AR_VK_DOWN     XK_Down
  #define AR_VK_INS      XK_Insert
  #define AR_VK_DEL      XK_Delete
  #define AR_VK_F1       XK_F1
  #define AR_VK_F2       XK_F2
  #define AR_VK_F3       XK_F3
  #define AR_VK_F4       XK_F4
  #define AR_VK_F5       XK_F5
  #define AR_VK_F6       XK_F6
  #define AR_VK_F7       XK_F7
  #define AR_VK_F8       XK_F8
  #define AR_VK_F9       XK_F9
  #define AR_VK_F10      XK_F10
  #define AR_VK_F11      XK_F11
  #define AR_VK_F12      XK_F12
  #define AR_VK_NUMLOCK  XK_Num_Lock
  #define AR_VK_SCROLL   XK_Scroll_Lock
  //@}

  //@{
  /**
   * @name Mouse virtual 'key' codes.
   *
   * @note Chosen such that functions can do bitwise logic on them.
   *
   * @todo Support for more than 3 mouse buttons.
   */
  #define AR_LBUTTON         0x0001
  #define AR_MBUTTON         0x0010
  #define AR_RBUTTON         0x0100
  #define AR_BUTTON_GARBAGE  0x0000
  //@}

  //@{
  /**
   * @name Cursor virtual 'key' codes.
   *
   * @todo Support for more cursor types.
   */
  #define AR_CURSOR_ARROW    0x0000
  #define AR_CURSOR_HELP     0x0001
  #define AR_CURSOR_WAIT     0x0002
  #define AR_CURSOR_NONE     0x0003
  //@}

  //@{
  /**
   * @name Z ordering virtual 'key' codes.
   *
   * @todo Support for different z orderings.
   */
  #define AR_ZORDER_NORMAL   0x0000
  #define AR_ZORDER_TOP      0x0001
  #define AR_ZORDER_TOPMOST  0x0002
  //@}

#endif
#endif
