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
#ifndef __BSP_H__
#define __BSP_H__

enum
{
    FACETYPE_NORMAL   = 1,
    FACETYPE_MESH     = 2,
    FACETYPE_TRISURF  = 3,
    FACETYPE_FLARE    = 4
};

typedef int bbox_t[6];        /* Integer bounding box (mins, maxs)*/
typedef float bboxf_t[6];     /* Float bounding box */
typedef float texcoord_t[2];  /* Texture s&t coordinates */
typedef byte_t colour_t[4];   /* RGBA */

/* Model 0 is the main map, others are doors, gates, buttons, etc. */
typedef struct
{
    bboxf_t bbox;
    int firstface, numfaces;
    int firstunknown, numunknowns;
} model_t;

/* Face planes */
typedef struct
{
    vec3_t vec;    /* Normal to plane */
    float offset;  /* Distance to plane along normal */
} plane_t;

/* Nodes in the BSP tree */
typedef struct
{
    int plane;        /* Dividing plane */
    int children[2];  /* Left and right node.  Negatives are leafs */
    bbox_t bbox;
} node_t;

/* Leafs in BSP tree */
typedef struct
{
    int cluster;    /* Visibility cluster number */
    int area;       /* ? */
    bbox_t bbox;
    int firstface, numfaces;
    int firstunknown, numunknowns;
} leaf_t;

/* Faces (or surfaces) */
typedef struct
{
    int shader;      /* Shader reference */
    int unknown1[1];
    int facetype;   /* FACETYPE enum */
    int firstvert, numverts;
    int firstelem, numelems;
    int lm_texnum;    /* lightmap info */
    int lm_offset[2];
    int lm_size[2];
    vec3_t v_orig;   /* FACETYPE_NORMAL only */
    bboxf_t bbox;    /* FACETYPE_MESH only */
    vec3_t v_norm;   /* FACETYPE_NORMAL only */
    int mesh_cp[2];  /* mesh control point dimensions */
} face_t;

/* Shader references (indexed from faces) */
typedef struct
{
    char name[64];
    int unknown[2];
} shaderref_t;

/* Vertex info */
typedef struct
{
    vec3_t v_point;     /* World coords */
    texcoord_t tex_st;  /* Texture coords */
    texcoord_t lm_st;   /* Lightmap texture coords */
    vec3_t v_norm;      /* Normal */
    colour_t colour;    /* Colour used for vertex lighting ? */
} vertex_t;

/* Potentially visible set (PVS) data */
typedef struct
{
    int numclusters;   /* Number of PVS clusters */
    int rowsize;
    byte_t data[1];
} visibility_t;

#ifdef __cplusplus
extern "C" {
#endif

void bsp_read(const char *fname);
void bsp_list(void);
void bsp_free(void);
int bsp_addshaderref(const char *shadername);

#ifdef __cplusplus
}
#endif

/* PVS test macro:  PVS table is a packed single bit array, rowsize
   bytes times numclusters rows */
#define BSP_TESTVIS(from,to) \
        (*(g->r_visibility->data + (from)*g->r_visibility->rowsize + \
           ((to)>>3)) & (1 << ((to) & 7)))

#endif /*__BSP_H__*/
