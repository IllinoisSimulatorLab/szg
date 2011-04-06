
%ModuleHeaderCode
#include "arGlutRenderFuncs.h"
%End

%Include typemaps.sip

void ar_glutWireCube ( double dSize );
void ar_glutSolidCube( double dSize );

void ar_glutWireSphere (double radius, int slices, int stacks);
void ar_glutSolidSphere(double radius, int slices, int stacks);

void ar_glutSolidCone( double base, double height, int slices, int stacks );
void ar_glutWireCone ( double base, double height, int slices, int stacks);

void ar_glutSolidCylinder(double radius, double height, int slices, int stacks);
void ar_glutWireCylinder (double radius, double height, int slices, int stacks);

void ar_glutWireTorus ( double dInnerRadius, double dOuterRadius, int nSides, int nRings );
void ar_glutSolidTorus( double dInnerRadius, double dOuterRadius, int nSides, int nRings );

void ar_glutWireDodecahedron ();
void ar_glutSolidDodecahedron();

void ar_glutWireOctahedron ();
void ar_glutSolidOctahedron();

void ar_glutWireTetrahedron ();
void ar_glutSolidTetrahedron();

void ar_glutWireIcosahedron ();
void ar_glutSolidIcosahedron();

void ar_glutWireRhombicDodecahedron ();
void ar_glutSolidRhombicDodecahedron();

//void ar_glutWireSierpinskiSponge  ( int num_levels, double offset[3], double scale );
//void ar_glutSolidSierpinskiSponge ( int num_levels, double offset[3], double scale );

void ar_glutWireTeapot ( double size );
void ar_glutSolidTeapot( double size );

