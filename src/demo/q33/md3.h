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
#ifndef __MD3_H__
#define __MD3_H__

/* Each MD3 model is composed of one or more meshes.  Currently the md3
 * loader is used only for mapents, so animated (multiframe) models
 * are not supported.
 */

/* SG - Normals are now imported correctly (needed for envmaps) */

typedef struct
{
    int shader;          /* Shader reference */
    int numverts;
    vec3_t *points;
    texcoord_t *tex_st;  /* Texture coords */
    vec3_t *normals;  /* Used for environment mapping */
    int numelems;
    uint_t *elems;
} md3mesh_t;

typedef struct
{
    bboxf_t bbox;
    int nummeshes;
    md3mesh_t *meshes;
} md3model_t;

#ifdef __cplusplus
extern "C" {
#endif

void md3_init(int max_nummodels);
void md3_free(void);
int md3_load(const char *path);

#ifdef __cplusplus
}
#endif

#endif /*__MD3_H__*/
