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
#include "mesh.h"

#include "globalshared.h"
extern global_shared_t *g;

#define LEVEL_WIDTH(lvl) ((1 << (lvl+1)) + 1)

static void mesh_create(face_t *face, mesh_t *mesh);

void
mesh_create_all(void)
{
    int i;    
    
    /* Count meshes */
    for (g->r_nummeshes=0; g->r_nummeshes < g->r_numfaces; g->r_nummeshes++)
	if (g->r_faces[g->r_nummeshes].facetype != FACETYPE_MESH)
	    break;

    g->r_meshes = (mesh_t*)malloc(g->r_nummeshes * sizeof(mesh_t));

    for (i=0; i < g->r_nummeshes; i++)
    {
	mesh_create(&(g->r_faces[i]), &(g->r_meshes[i]));
    }
}

void
mesh_free_all(void)
{
    int i;

    for (i=0; i < g->r_nummeshes; i++)
    {
	gc_free(g->r_meshes[i].points);
	/* tex_st and lm_st are part of points: don't free */
	gc_free(g->r_meshes[i].elems);
    }
    gc_free(g->r_meshes);
}

static int
mesh_find_level(vec3_t *v)
{
    int level;
    vec3_t a, b, dist;

    /* Subdivide on the left until tolerance is reached */
    for (level=0; level < g->r_maxmeshlevel-1; level++)
    {
	/* Subdivide on the left */
	vec_avg(v[0], v[1], a);
	vec_avg(v[1], v[2], b);
	vec_avg(a, b, v[2]);

	/* Find distance moved */
	vec_sub(v[2], v[1], dist);

	/* Check for tolerance */
	if (vec_dot(dist, dist) < g->r_subdivisiontol * g->r_subdivisiontol)
	    break;

	/* Insert new middle vertex */
	vec_copy(a, v[1]);
    }

    return level;
}

static void
mesh_find_size(int *numcp, vec3_t *cp, int *size)
{
    int u, v, found, level;
    float *a, *b;
    vec3_t test[3];
    
    /* Find non-coincident pairs in u direction */
    found = 0;
    for (v=0; v < numcp[1]; v++)
    {
	for (u=0; u < numcp[0]-1; u += 2)
	{
	    a = cp[v * numcp[0] + u];
	    b = cp[v * numcp[0] + u + 2];
	    if (!vec_cmp(a,b))
	    {
		found = 1;
		break;
	    }
	}
	if (found) break;
    }
    if (!found) Error("Bad mesh control points");

    /* Find subdivision level in u */
    vec_copy(a, test[0]);
    vec_copy((a+3), test[1]);
    vec_copy(b, test[2]);
    level = mesh_find_level(test);
    size[0] = (LEVEL_WIDTH(level) - 1) * ((numcp[0]-1) / 2) + 1;
    
    /* Find non-coincident pairs in v direction */
    found = 0;
    for (u=0; u < numcp[0]; u++)
    {
	for (v=0; v < numcp[1]-1; v += 2)
	{
	    a = cp[v * numcp[0] + u];
	    b = cp[(v + 2) * numcp[0] + u];
	    if (!vec_cmp(a,b))
	    {
		found = 1;
		break;
	    }
	}
	if (found) break;
    }
    if (!found) Error("Bad mesh control points");

    /* Find subdivision level in v */
    vec_copy(a, test[0]);
    vec_copy((a+numcp[0]*3), test[1]);
    vec_copy(b, test[2]);
    level = mesh_find_level(test);
    size[1] = (LEVEL_WIDTH(level) - 1)* ((numcp[1]-1) / 2) + 1;    
}

static void
mesh_fill_curve_3(int numcp, int size, int stride, vec3_t *p)
{
    int step, halfstep, i, mid;
    vec3_t a, b;

    step = (size-1) / (numcp-1);

    while (step > 0)
    {
	halfstep = step / 2;
	for (i=0; i < size-1; i += step*2)
	{
	    mid = (i+step)*stride;
	    vec_avg(p[i*stride], p[mid], a);
	    vec_avg(p[mid], p[(i+step*2)*stride], b);
	    vec_avg(a, b, p[mid]);

	    if (halfstep > 0)
	    {
		vec_copy(a, p[(i+halfstep)*stride]);
		vec_copy(b, p[(i+3*halfstep)*stride]);
	    }
	}
	
	step /= 2;
    }
}

static void
mesh_fill_curve_2(int numcp, int size, int stride, vec2_t *p)
{
    int step, halfstep, i, mid;
    vec2_t a, b;

    step = (size-1) / (numcp-1);

    while (step > 0)
    {
	halfstep = step / 2;
	for (i=0; i < size-1; i += step*2)
	{
	    mid = (i+step)*stride;
	    vec2_avg(p[i*stride], p[mid], a);
	    vec2_avg(p[mid], p[(i+step*2)*stride], b);
	    vec2_avg(a, b, p[mid]);

	    if (halfstep > 0)
	    {
		vec2_copy(a, p[(i+halfstep)*stride]);
		vec2_copy(b, p[(i+3*halfstep)*stride]);
	    }
	}
	
	step /= 2;
    }
}

static void
mesh_fill_curve_c(int numcp, int size, int stride, colour_t *p)
{
    int step, halfstep, i, mid;
    colour_t a, b;

    step = (size-1) / (numcp-1);

    while (step > 0)
    {
	halfstep = step / 2;
	for (i=0; i < size-1; i += step*2)
	{
	    mid = (i+step)*stride;
	    colour_avg(p[i*stride], p[mid], a);
	    colour_avg(p[mid], p[(i+step*2)*stride], b);
	    colour_avg(a, b, p[mid]);

	    if (halfstep > 0)
	    {
		colour_copy(a, p[(i+halfstep)*stride]);
		colour_copy(b, p[(i+3*halfstep)*stride]);
	    }
	}
	
	step /= 2;
    }
}

static void
mesh_fill_patch_3(int *numcp, int *size, vec3_t *p)
{
    int step, u, v;

    /* Fill in control points in v direction */
    step = (size[0]-1) / (numcp[0]-1);    
    for (u = 0; u < size[0]; u += step)
    {
	mesh_fill_curve_3(numcp[1], size[1], size[0], p + u);
    }

    /* Fill in the rest in the u direction */
    for (v = 0; v < size[1]; v++)
    {
	mesh_fill_curve_3(numcp[0], size[0], 1, p + v * size[0]);
    }
}

static void
mesh_fill_patch_2(int *numcp, int *size, vec2_t *p)
{
    int step, u, v;

    /* Fill in control points in v direction */
    step = (size[0]-1) / (numcp[0]-1);    
    for (u = 0; u < size[0]; u += step)
    {
	mesh_fill_curve_2(numcp[1], size[1], size[0], p + u);
    }

    /* Fill in the rest in the u direction */
    for (v = 0; v < size[1]; v++)
    {
	mesh_fill_curve_2(numcp[0], size[0], 1, p + v * size[0]);
    }
}

static void
mesh_fill_patch_c(int *numcp, int *size, colour_t *p)
{
    int step, u, v;

    /* Fill in control points in v direction */
    step = (size[0]-1) / (numcp[0]-1);    
    for (u = 0; u < size[0]; u += step)
    {
	mesh_fill_curve_c(numcp[1], size[1], size[0], p + u);
    }

    /* Fill in the rest in the u direction */
    for (v = 0; v < size[1]; v++)
    {
	mesh_fill_curve_c(numcp[0], size[0], 1, p + v * size[0]);
    }
}

static void
mesh_create(face_t *face, mesh_t *mesh)
{
    int step[2], size[2], len, i, u, v, p;
    vec3_t *cp;
    vertex_t *vert;

    cp = (vec3_t*)malloc(face->numverts * sizeof(vec3_t));
    vert = &(g->r_verts[face->firstvert]);
    for (i=0; i < face->numverts; i++)
    {
	vec_copy(vert->v_point, cp[i]);
	vert++;
    }

    /* Find the degree of subdivision in the u and v directions */
    mesh_find_size(face->mesh_cp, cp, size);
    free(cp);

    /* Allocate space for mesh */
    len = size[0] * size[1];
    mesh->size[0] = size[0];
    mesh->size[1] = size[1];
    mesh->points = (vec3_t*)malloc(len * (2* sizeof(vec3_t) +
					  2 * sizeof(texcoord_t) +
					 sizeof(colour_t)));
    mesh->normals = mesh->points + len;
    mesh->colour = (colour_t*)(mesh->normals + len);
    mesh->tex_st = (texcoord_t*)(mesh->colour + len);
    mesh->lm_st = mesh->tex_st + len;

    /* Fill in sparse mesh control points */
    step[0] = (size[0]-1) / (face->mesh_cp[0]-1);
    step[1] = (size[1]-1) / (face->mesh_cp[1]-1);
    vert = &(g->r_verts[face->firstvert]);
    for (v = 0; v < size[1]; v += step[1])
    {
	for (u = 0; u < size[0]; u += step[0])
	{
	    p = v * size[0] + u;
	    vec_copy(vert->v_point, mesh->points[p]);
	    /* SG - for envmaps */
	    vec_copy(vert->v_norm, mesh->normals[p]);
	    /* SG - FIXME: Proper handling of normals? */
	    colour_copy(vert->colour, mesh->colour[p]);
	    vec2_copy(vert->tex_st, mesh->tex_st[p]);
	    vec2_copy(vert->lm_st, mesh->lm_st[p]);
	    vert++;
	}
    }

    /* Fill in each mesh */
    mesh_fill_patch_3(face->mesh_cp, size, mesh->points);
    /* SG - Normals should really be generated from (du x dv) */
    mesh_fill_patch_3(face->mesh_cp, size, mesh->normals);
    mesh_fill_patch_c(face->mesh_cp, size, mesh->colour);
    mesh_fill_patch_2(face->mesh_cp, size, (vec2_t*)mesh->tex_st);
    mesh_fill_patch_2(face->mesh_cp, size, (vec2_t*)mesh->lm_st);

    /* Allocate and fill element table */
    mesh->numelems = (size[0]-1) * (size[1]-1) * 6;
    mesh->elems = (uint_t*)malloc(mesh->numelems * sizeof(uint_t));

    i = 0;
    for (v = 0; v < size[1]-1; ++v)
    {
	for (u = 0; u < size[0]-1; ++u)
	{
	    mesh->elems[i++] = v * size[0] + u;
	    mesh->elems[i++] = (v+1) * size[0] + u;
	    mesh->elems[i++] = v * size[0] + u + 1;
	    mesh->elems[i++] = v * size[0] + u + 1;
	    mesh->elems[i++] = (v+1) * size[0] + u;
	    mesh->elems[i++] = (v+1) * size[0] + u + 1;
	}
    }
}
