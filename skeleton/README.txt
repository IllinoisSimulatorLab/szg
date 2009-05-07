This directory is a build template for user applications. Copy it wherever
you want, replace the source files and modify build/makefiles/Makefile.my_app
appropriately.

To build, type 'make' in the top directory, i.e. in 'skeleton'.
'make clean' to clean up.

Build targets are specified in build/makefiles/Makefile.my_app.

For the build to work, you must set $SZGHOME (pointing to the top-level
directory of your syzygy install). Of course, you must have built the
Syzygy libraries first.

You will also want to set $SZGBIN (which indicates the directory in which
executables will be copied). If you don't set it, the exeutable for this
project will end up in szg/build/<platform> e.g. szg/build/win32 on
Windows.
Finally, if you are using external libraries
(such as sound), you will need to set $SZGEXTERNAL appropriately. Please
see the syzygy documentation.

skeleton.cpp is a simple, old-style master/slave application. It uses
some fancy user-interaction classes, but those can easily be ignored/removed.
In particular, if you just want to do simple things with user input you can
query the framework directly (using e.g. getButton( <index #> )) instead
of using an effector.

oopskel.cpp is an object-oriented version of the same, in which the
arMasterSlaveFramework is subclassed and callback methods (those
starting with on...()) are overridden instead of installing
callback functions.

arTeapotGraphicsPlugin.cpp is a scene-graph (szgrender) plugin, to show
how to use the skeleton to build a shared library.
