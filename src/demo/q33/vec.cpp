/* Aftershock 3D rendering engine
 * Copyright (C) 1999 Stephen C. Taylor
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "vec.h"
#include <math.h>

void vec_normalize(vec3_t a)
{
       float b=sqrt((a[0]*a[0])+(a[1]*a[1])+(a[2]*a[2]));

       a[0]/=b; a[1]/=b; a[2]/=b;
}

void
vec_point(vec3_t point, float az, float el)
{
    float c = cos(el * DEG2RAD);
    
    point[0] = c * cos(az * DEG2RAD);
    point[1] = c * sin(az * DEG2RAD);
    point[2] = sin(el * DEG2RAD);
}

/* Stolen from Mesa:matrix.c */
#define A(row,col)  a[(col<<2)+row]
#define B(row,col)  b[(col<<2)+row]
#define P(row,col)  product[(col<<2)+row]

void
mat4_mmult(mat4_t a, mat4_t b, mat4_t product)
{
   int i;
   for (i = 0; i < 4; i++)
   {
      float ai0=A(i,0),  ai1=A(i,1),  ai2=A(i,2),  ai3=A(i,3);
      P(i,0) = ai0 * B(0,0) + ai1 * B(1,0) + ai2 * B(2,0) + ai3 * B(3,0);
      P(i,1) = ai0 * B(0,1) + ai1 * B(1,1) + ai2 * B(2,1) + ai3 * B(3,1);
      P(i,2) = ai0 * B(0,2) + ai1 * B(1,2) + ai2 * B(2,2) + ai3 * B(3,2);
      P(i,3) = ai0 * B(0,3) + ai1 * B(1,3) + ai2 * B(2,3) + ai3 * B(3,3);
   }
}

void
mat4_vmult(mat4_t a, vec4_t b, vec4_t product)
{
    float b0=b[0], b1=b[1], b2=b[2], b3=b[3];

    product[0] = A(0,0)*b0 + A(0,1)*b1 + A(0,2)*b2 + A(0,3)*b3;
    product[1] = A(1,0)*b0 + A(1,1)*b1 + A(1,2)*b2 + A(1,3)*b3;
    product[2] = A(2,0)*b0 + A(2,1)*b1 + A(2,2)*b2 + A(2,3)*b3;
    product[3] = A(3,0)*b0 + A(3,1)*b1 + A(3,2)*b2 + A(3,3)*b3;
}

void
mat4_v3mult(mat4_t a, vec3_t b, vec3_t product)
{
    float b0=b[0], b1=b[1], b2=b[2];

    product[0] = A(0,0)*b0 + A(0,1)*b1 + A(0,2)*b2;
    product[1] = A(1,0)*b0 + A(1,1)*b1 + A(1,2)*b2;
    product[2] = A(2,0)*b0 + A(2,1)*b1 + A(2,2)*b2;
}

