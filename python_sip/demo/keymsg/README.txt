Demo of sending keypresses to the master instance of a cluster app

keytest.py is a Syzygy application. You can run it in standalone
mode and press keys or run it on a virtual computer and run the
keymsg.py script. Then any keys you hit are relayed to the master
instance of the app until you hit ESC.

IMPORTANT! When running in standalone mode, keytest.py receives
_two_ events for each keypress, a 'key down' and a 'key up'. When
running on a cluster, it only receives one. You should probably
throw away any 'key down' events.

getch.py is a module used by keymsg.py to read the keyboard.
