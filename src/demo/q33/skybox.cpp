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
#include "util.h"
#include "bsp.h"
#include "skybox.h"

#include "globalshared.h"
extern global_shared_t *g;

#define SIDE_SIZE 9
#define POINTS_LEN (SIDE_SIZE*SIDE_SIZE)
#define ELEM_LEN ((SIDE_SIZE-1)*(SIDE_SIZE-1)*6)

#define SPHERE_RAD  10.0
#define EYE_RAD      9.0

#define SCALE_S 4.0  /* Arbitrary (?) texture scaling factors */
#define SCALE_T 4.0 

static void gen_box_side(int side, vec3_t orig, vec3_t drow, vec3_t dcol);
static void gen_box(void);
static void gen_elems(void);

void
skybox_create(void)
{
    int i;

    /* Alloc space for skybox verts, etc. */
    g->r_skybox = (skybox_t*) gc_malloc(sizeof(skybox_t));
    g->r_skybox->points[0] = (vec3_t*) gc_malloc(6 * POINTS_LEN * sizeof(vec3_t));
    g->r_skybox->tex_st[0] = (texcoord_t*) gc_malloc(6 * POINTS_LEN *
					      sizeof(texcoord_t));
    g->r_skybox->elems = (uint_t*) gc_malloc(ELEM_LEN * sizeof(uint_t));

    g->r_skybox->numpoints = POINTS_LEN;
    g->r_skybox->numelems = ELEM_LEN;
    
    for (i=1; i < 6; i++)
    {
	g->r_skybox->points[i] = g->r_skybox->points[i-1] + POINTS_LEN;
	g->r_skybox->tex_st[i] = g->r_skybox->tex_st[i-1] + POINTS_LEN;
    }

    gen_box();
    gen_elems();
}
    

void
skybox_free(void)
{
    gc_free(g->r_skybox->points[0]);
    gc_free(g->r_skybox->tex_st[0]);
    gc_free(g->r_skybox->elems);
    gc_free(g->r_skybox);    
}

static void
gen_elems(void)
{
    int u, v;
    uint_t *e;

    /* Box elems in tristrip order */
    e = g->r_skybox->elems;
    for (v = 0; v < SIDE_SIZE-1; ++v)
    {
	for (u = 0; u < SIDE_SIZE-1; ++u)
	{
	    *e++ = v * SIDE_SIZE + u;
	    *e++ = (v+1) * SIDE_SIZE + u;
	    *e++ = v * SIDE_SIZE + u + 1;
	    *e++ = v * SIDE_SIZE + u + 1;
	    *e++ = (v+1) * SIDE_SIZE + u;
	    *e++ = (v+1) * SIDE_SIZE + u + 1;	    
	}
    }
}
    
static void
gen_box(void)
{
    vec3_t orig, drow, dcol;
    float size = 1.0f;
    float step = 0.25f;
   
    /* Top */
    orig[0] = -size;
    orig[1] = size;
    orig[2] = size;
    drow[0] = 0.0;
    drow[1] = -step;
    drow[2] = 0.0;
    dcol[0] = step;
    dcol[1] = 0.0;
    dcol[2] = 0.0;
    gen_box_side(SKYBOX_TOP, orig, drow, dcol);

    /* Front */
    orig[0] = size;
    orig[1] = size;
    orig[2] = size;
    drow[0] = 0.0;
    drow[1] = 0.0;
    drow[2] = -step;
    dcol[0] = -step;
    dcol[1] = 0.0;
    dcol[2] = 0.0;
    gen_box_side(SKYBOX_FRONT, orig, drow, dcol);

    /* Right */
    orig[0] = size;
    orig[1] = -size;
    orig[2] = size;
    drow[0] = 0.0;
    drow[1] = 0.0;
    drow[2] = -step;
    dcol[0] = 0.0;
    dcol[1] = step;
    dcol[2] = 0.0;
    gen_box_side(SKYBOX_RIGHT, orig, drow, dcol);

    /* Back */
    orig[0] = -size;
    orig[1] = -size;
    orig[2] = size;
    drow[0] = 0.0;
    drow[1] = 0.0;
    drow[2] = -step;
    dcol[0] = step;
    dcol[1] = 0.0;
    dcol[2] = 0.0;
    gen_box_side(SKYBOX_BACK, orig, drow, dcol);

    /* Left */
    orig[0] = -size;
    orig[1] = size;
    orig[2] = size;
    drow[0] = 0.0;
    drow[1] = 0.0;
    drow[2] = -step;
    dcol[0] = 0.0;
    dcol[1] = -step;
    dcol[2] = 0.0;
    gen_box_side(SKYBOX_LEFT, orig, drow, dcol);

    /* Bottom */
    orig[0] = -size;
    orig[1] = -size;
    orig[2] = -size;
    drow[0] = 0.0;
    drow[1] = step;
    drow[2] = 0.0;
    dcol[0] = step;
    dcol[1] = 0.0;
    dcol[2] = 0.0;
    gen_box_side(SKYBOX_BOTTOM, orig, drow, dcol);

}

static void
gen_box_side(int side, vec3_t orig, vec3_t drow, vec3_t dcol)
{
    vec3_t pos, w, row, *v;
    texcoord_t *tc;
    float p;
    int r, c;
    float d, b, t;

    /* I don't know exactly what Q3A does for skybox texturing, but this is
     * at least fairly close.  We tile the texture onto the inside of
     * a large sphere, and put the camera near the top of the sphere.
     * We place the box around the camera, and cast rays through the
     * box verts to the sphere to find the texture coordinates.
     */
    
    d = EYE_RAD;     /* Sphere center to camera distance */ 
    b = SPHERE_RAD;  /* Sphere radius */
    
    v = &(g->r_skybox->points[side][0]);
    tc = &(g->r_skybox->tex_st[side][0]);
    vec_copy(orig, row);
    for (r = 0; r < SIDE_SIZE; ++r)
    {
	vec_copy(row, pos);
	for (c = 0; c < SIDE_SIZE; ++c)
	{
	    /* pos points from eye to vertex on box */
	    vec_copy(pos, (*v));
	    vec_copy(pos, w);

	    /* Normalize pos -> w */
	    p = sqrt(vec_dot(w, w));
	    w[0] /= p;
	    w[1] /= p;
	    w[2] /= p;

	    /* Find distance along w to sphere */
	    t = sqrt(d*d*(w[2]*w[2]-1.0) + b*b) - d*w[2];
	    w[0] *= t;
	    w[1] *= t;

	    /* Use x and y on sphere as s and t */
	    (*tc)[0] = w[0] / (2.0 * SCALE_S);
	    (*tc)[1] = w[1] / (2.0 * SCALE_T);

	    vec_add(pos, dcol, pos);
	    v++;
	    tc++;
	}
	vec_add(row, drow, row);
    }
}
