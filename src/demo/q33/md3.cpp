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
#include "pak.h"
#include "bsp.h"
#include "md3.h"
#include <stdio.h>
#include <string.h>

#include "globalshared.h"
extern global_shared_t *g;

typedef struct
{
    char id[4];
    int version;
    char filename[68];
    int numboneframes;
    int numtags;
    int nummeshes;
    int numskins;
    int bone_offs;
    int tag_offs;
    int mesh_offs;
    int filesize;
} md3header_t;

typedef struct
{
    char name[64];
    vec3_t pos;
    mat3_t rot;
} md3tag_t;

typedef struct
{
    bboxf_t bbox;
    vec3_t pos;
    float scale;
    char creator[16];
} md3boneframe_t;

typedef struct
{
    char id[4];
    char name[68];
    int numframes;
    int numskins;
    int numverts;
    int numtris;
    int elem_offs;
    int skin_offs;
    int tc_offs;
    int vert_offs;
    int meshsize;
} md3mesh_file_t;

typedef struct
{
    signed short vec[3];
    byte_t norm[2]; /* Vertex normals in polar coordinates */
} md3vert_t;

void
md3_init(int max_nummodels)
{
    g->r_nummd3models = 0;
    g->r_md3models = (md3model_t*) gc_malloc(max_nummodels * sizeof(md3model_t));
}

void
md3_free(void)
{
    int i, j;
    md3model_t *model;

    for (i = 0; i < g->r_nummd3models; i++)
    {
	model = &(g->r_md3models[i]);
	for (j = 0; j < model->nummeshes; j++)
	    free(model->meshes[j].points);
	free(model->meshes);
    }
    gc_free(g->r_md3models);
}

int
md3_load(const char *path)
{
    byte_t *md3;
    int md3len;
    md3header_t *header;
    md3boneframe_t *boneframe;
    md3mesh_file_t *fmesh;
    md3model_t *model;
    md3mesh_t *mesh;
    md3vert_t *vert;
    int i, j;
    char *skin;
    double phi, theta; /* SG - for normals, change to LUT later */

    model = &(g->r_md3models[g->r_nummd3models++]);
    
    /* fprintf(stderr, "...loading '%s'\n", path); */

    /* Read md3 file */
    if (!pak_open(path))
       Error("Failed to open md3 file %s", path);
    md3len = pak_getlen();
    md3 = (byte_t *) malloc(md3len);
    pak_read(md3, md3len, 1);
    pak_close();

    /* Parse header */
    header = (md3header_t*)md3;
    BYTESWAP(header->version);
    BYTESWAP(header->numboneframes);
    BYTESWAP(header->numtags);
    BYTESWAP(header->nummeshes);
    BYTESWAP(header->numskins);
    BYTESWAP(header->bone_offs);
    BYTESWAP(header->tag_offs);
    BYTESWAP(header->mesh_offs);
    BYTESWAP(header->filesize);
    if (header->numboneframes != 1)
	Error("Unexpected number of boneframes: %d", header->numboneframes);
    boneframe = (md3boneframe_t*)(md3 + header->bone_offs);
    BYTESWAPBBOX(boneframe->bbox);
    BYTESWAPVEC3(boneframe->pos);
    BYTESWAPFLOAT(boneframe->scale);

    /* Setup model */
    memcpy(model->bbox, boneframe->bbox, sizeof(bboxf_t));
    model->nummeshes = header->nummeshes;
    model->meshes = (md3mesh_t*)malloc(model->nummeshes * sizeof(md3mesh_t));

    /* Setup meshes */
    fmesh = (md3mesh_file_t*)(md3 + header->mesh_offs);
    for (i = 0; i < model->nummeshes; i++)
    {
	mesh = &model->meshes[i];

	BYTESWAP(fmesh->numframes);
	BYTESWAP(fmesh->numskins);
	BYTESWAP(fmesh->numverts);
	BYTESWAP(fmesh->numtris);
	BYTESWAP(fmesh->elem_offs);
	BYTESWAP(fmesh->skin_offs);
	BYTESWAP(fmesh->tc_offs);
	BYTESWAP(fmesh->vert_offs);
	BYTESWAP(fmesh->meshsize);

	if (fmesh->numframes != 1)
	    Error("Unexpected number of frames: %d %d", i, fmesh->numframes);
	if (fmesh->numskins != 1)
	    Error("Unexpected number of skins: %d %d", i, fmesh->numskins);

	mesh->numverts = fmesh->numverts;
	mesh->numelems = fmesh->numtris * 3;
	/* Use one big malloc for all arrays */
	mesh->points = (vec3_t*)malloc(
	    (mesh->numverts * (2*sizeof(vec3_t) + sizeof(texcoord_t))) +
	     mesh->numelems * sizeof(uint_t));
	mesh->tex_st = (texcoord_t*)(mesh->points + mesh->numverts);
	mesh->normals = (vec3_t*)(mesh->tex_st + mesh->numverts);
	mesh->elems = (uint_t*)(mesh->normals + mesh->numverts);

	/* Strip trailing .tga from skin name */
	skin = (char*)((byte_t*)fmesh + fmesh->skin_offs);
	skin[strlen(skin)-4] = '\0';
	mesh->shader = bsp_addshaderref(skin);

	/* Copy texture and element data */
	memcpy(mesh->tex_st, (byte_t*)fmesh + fmesh->tc_offs,
	       mesh->numverts * sizeof(texcoord_t));
	for (j=0; j<mesh->numverts; j++) {
	  BYTESWAPFLOAT(mesh->tex_st[j][0]);
	  BYTESWAPFLOAT(mesh->tex_st[j][1]);
	}
	memcpy(mesh->elems, (byte_t*)fmesh + fmesh->elem_offs,
	       mesh->numelems * sizeof(uint_t));
	for (j=0; j<mesh->numelems; j++)
	  BYTESWAP(mesh->elems[j]);

	/* Transform vertexes to floating point */
	vert = (md3vert_t*)((byte_t*)fmesh + fmesh->vert_offs);
	for (j=0; j < mesh->numverts; j++)
	{
	    BYTESWAPSHORT(vert->vec[0]);
	    BYTESWAPSHORT(vert->vec[1]);
	    BYTESWAPSHORT(vert->vec[2]);

	    /* FIXME: what is the scaling factor here (guessing 64) */
	    mesh->points[j][0] = (float)vert->vec[0] / 64.0f;
	    mesh->points[j][1] = (float)vert->vec[1] / 64.0f;
	    mesh->points[j][2] = (float)vert->vec[2] / 64.0f;

	    /* SG - This REALLY ought to be speeded up by a LUT */
	    theta = (double)vert->norm[0] / 128.0 * M_PI;
	    phi = (double)vert->norm[1] / 256.0 * 2.0 * M_PI;
	    /* SG - is "z=up" the correct convention for Q3A? */
	    mesh->normals[j][0] = sin(theta) * cos(phi);
	    mesh->normals[j][1] = sin(theta) * sin(phi);
	    mesh->normals[j][2] = cos(theta);
	    
	    vert++;
	}
	
	fmesh = (md3mesh_file_t*)((byte_t*)fmesh + fmesh->meshsize);
    }    
    
    free(md3);
    return g->r_nummd3models-1;
}
