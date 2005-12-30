#!/bin/sh
if [ "$SHELL" = "/bin/bash" ]; then
  echo "Remark: assuming using bash shell."
  # Make a special szg prompt.
  export PS1=SZG:\\u@\\h:\\w\\r\\n::
else
  # Assume this is tsch and use that syntax.
  setenv PS1 SZG:\\u@\\h:\\w\\r\\n::
fi

