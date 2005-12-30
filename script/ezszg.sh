#!/bin/sh
# ONLY WORKS FOR MAC OS X SO FAR!
# Make a special szg prompt.
TEMP=$PWD/doc/images/NormalAl.jpg
# Check to see if we are in the right spot... Necessary since all variables
# are relative to the current directory.
if [ -f $TEMP ]; then
  echo "Setting up your szg SDK..."
  # TODO: Check to make sure this script has not been executed twice.
  export PS1=SZG:\\u@\\h:\\w\\r\\n::
  export PATH=$PWD/bin:$PATH
  export PYTHONPATH=$PWD/bin:$PYTHONPATH
  export DYLD_LIBRARY_PATH=$PWD/bin:$DYLD_LIBRARY_PATH
  export SZG_RENDER_texture_path=$PWD/rsc
  export SZG_RENDER_text_path=$PWD/rsc
  export SZG_PEER_path=$PWD/rsc
  export SZG_SOUND_path=$PWD/rsc
  export SZG_PARAM=$PWD/doc/window_configs.txt
  export SZG_DISPLAY0_name=single_window
  export SZG_DISPLAY1_name=virtual_cube_window
  export SZG_DISPLAY2_name=quad_window
  export SZG_DISPLAY3_name=single_window_fullscreen
else
  echo "This script must be run from the top-level of your szg SDK directory."
fi




