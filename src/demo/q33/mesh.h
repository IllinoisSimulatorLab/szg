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
#ifndef __MESH_H__
#define __MESH_H__

/* A mesh is a quadratic bezier patch, generated from control points.
 */

typedef struct
{
    int size[2];    /* Mesh dimensions, u&v */
    vec3_t *points;
    vec3_t *normals; /* SG - for envmaps */
    colour_t *colour;
    texcoord_t *tex_st;
    texcoord_t *lm_st;
    int numelems;
    uint_t *elems;
} mesh_t;

#ifdef __cplusplus
extern "C" {
#endif

void mesh_create_all(void);
void mesh_free_all(void);

#ifdef __cplusplus
}
#endif

#endif /*_MESH_H__*/
