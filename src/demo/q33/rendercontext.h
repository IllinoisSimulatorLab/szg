/* Copyright (C) 2001 Paul Rajlich
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
#ifndef __RENDER_CONTEXT_H__
#define __RENDER_CONTEXT_H__

#include "util.h"
#include "bsp.h"

/* backend structures */
typedef struct
{
    int face;
    uint_t sortkey;
} rendface_t;

/* List of faces to render */
typedef struct
{
    int numfaces;
    rendface_t *faces;
} facelist_t;


/* Triangle arrays */
typedef struct
{
    int numverts;
    vec4_t *verts;
    /* SG - per-vertex normals included for envmaps */
    vec3_t *normals;
    colour_t *colour;
    texcoord_t *tex_st;
    texcoord_t *lm_st;
    int numelems;
    int *elems;
} arrays_t;

/* rendering context - everything a render thread/proc needs its own
 * copy of. */
typedef struct
{
  uint_t *r_textures;     
  uint_t *r_lightmaptex;
  int r_tex_units;

  mat4_t clipmat;        /* Matrix to go from worldspace to clipspace */
  facelist_t facelist;   /* Faces to be drawn */
  facelist_t translist;  /* Transparent faces to be drawn */
  int r_leafcount;       /* Counts up leafs walked for this scene */
  int *r_faceinc;        /* Flags faces as "included" in the facelist */
  int *skylist;          /* Sky faces hit by walk */
  int numsky;            /* Number of sky faces in list */
  float cos_fov;         /* Cosine of the field of view angle */
  float *mtxCli;         /* SG - inverse of camera rotation mtx for envmaps */
  arrays_t arrays;       /* rendering backend */

  float range[2];        /* for geometry decomposition */

  float view[16];
  float head[16];

  vec3_t r_eyepos;
  float r_eye_az, r_eye_el;
  int r_eyecluster;
  double r_frametime;

} r_context_t;

#ifdef __cplusplus
extern "C" {
#endif

void *rc_malloc(size_t size); /* for allocating rendering context data */
void rc_free(void *mem);      /* for deallocating rendering context data */

#ifdef __cplusplus
}
#endif

#endif
