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
#include "shader.h"
#include "mapent.h"
#include "render.h"
#include "renderback.h"
#include "glinc.h"
#include <stdio.h>
#include <string.h>

#include "globalshared.h"
extern global_shared_t *g;

/* The front end of the rendering pipeline decides what to draw, and
 * the back end actually draws it.  These functions build a list of faces
 * to draw, sorts it by shader, and sends it to the back end (renderback.c)
 */

#define MAX_TRANSPARENT 1000

enum
{
    CLIP_RIGHT_BIT   = 1,
    CLIP_LEFT_BIT    = 1 << 1,
    CLIP_TOP_BIT     = 1 << 2,
    CLIP_BOTTOM_BIT  = 1 << 3,
    CLIP_FAR_BIT     = 1 << 4,
    CLIP_NEAR_BIT    = 1 << 5
};

static GLfloat mtxC[16];

static GLfloat mtxCli[16] = {
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f };

/* expose to outside */
int find_cluster(vec3_t pos);

static void render_walk_model(r_context_t *c, int n);
static void render_walk_node(r_context_t *c, int n, int accept);
static void render_walk_leaf(r_context_t *c, int n, int accept);
static void render_walk_face(r_context_t *c, int n);
static void gen_clipmat(r_context_t *c);
static void sort_faces(r_context_t *c);
static int cliptest_bbox(r_context_t *c, bbox_t bbox);
static int cliptest_bboxf(r_context_t *c, bboxf_t bbox);
static int cliptest_point(vec4_t p);
static int classify_point(vec3_t p, int plane_n);

void
render_init(r_context_t *c)
{
    c->facelist.faces = (rendface_t*) rc_malloc(g->r_numfaces * sizeof(rendface_t));
    c->translist.faces = (rendface_t*) rc_malloc(MAX_TRANSPARENT *
					  sizeof(rendface_t));
    c->r_faceinc = (int*) rc_malloc(g->r_numfaces * sizeof(int));
    c->skylist = (int*) rc_malloc(100 * sizeof(int));
    render_backend_init(c);
}

void
render_init_mapents() {
    int i;

    /* Find cluster for each mapent */
    for (i=0; i < g->g_mapent_numinst; i++)
	g->g_mapent_inst[i].cluster = find_cluster(g->g_mapent_inst[i].origin);
}

void
render_finalize(r_context_t *c)
{
    rc_free(c->facelist.faces);
    rc_free(c->translist.faces);
    rc_free(c->r_faceinc);
    rc_free(c->skylist);
    render_backend_finalize(c);
}

void
render_objects(r_context_t *c)
{
    int i;
    
    c->r_leafcount = 0;

#if 0
    /* Useful for exploring face culling effectiveness */
    if (g->r_lockpvs)
    {
	render_backend(&facelist);
	return;
    }
#endif

    c->cos_fov = cos(g->r_eyefov/2.0f * DEG2RAD);

    /* Get clip coordinate transformation matrix */
    gen_clipmat(c);

    c->facelist.numfaces = c->translist.numfaces = 0;
    c->numsky = 0;
    /* Clear "included" faces list */
    memset(c->r_faceinc, 0, g->r_numfaces * sizeof(int));

    /* "Walk" the BSP tree to generate a list of faces to render */
    /* FIXME: include other models */
    render_walk_model(c, 0);

    /* Sort the face list */
    sort_faces(c);

    /* FIXME: Reset depth buffer range based on max/min Z in facelist */
    
    /* Draw sky first -- on first pipe if geometry decomposed */
    if (c->numsky && g->r_sky && c->range[0] == 0.0)
	render_backend_sky(c);


    /* Draw normal faces */
    render_backend(c, &(c->facelist));

    /* Draw visible mapents (md3 models) -- on last pipe if geometry decomposed */
	if (g->r_drawitems && c->range[1] == 1.0)
	{
	  for(i=0; i < g->g_mapent_numinst; i++)
	  {
		if (c->r_eyecluster < 0 ||
		    BSP_TESTVIS(c->r_eyecluster, g->g_mapent_inst[i].cluster))
		  render_backend_mapent(c, i);
	  }
	}
    /* Draw transparent faces last */
    if (c->translist.numfaces)
	render_backend(c, &(c->translist));
}

void render_clear(void)
{
	/* FIXME: color buffer clear can be optionally removed */
#define CLEAR_BITS GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT
        // AARGH! we need to eliminate this! to enable passive stereo
	// drawing into a single window, we need to be able to
        // do this in the master/slave framework code!
	/* Need to enable depth mask before clear */
	//glDepthMask(GL_TRUE);
	//glClear(CLEAR_BITS);
}
    
void render_draw(r_context_t *c)
{
  //**********************************************************************
  // one of the very few functions to be modified for the Syzygy releases
  //**********************************************************************

	c->r_leafcount = 0;

	glGetFloatv(GL_MODELVIEW_MATRIX, mtxC);
	// Invert the camera rotation matrix (orthonormal: inverse = transpose)
	mtxCli[0]=mtxC[0];
	mtxCli[1]=mtxC[4];
	mtxCli[2]=mtxC[8];
	mtxCli[4]=mtxC[1];
	mtxCli[5]=mtxC[5];
	mtxCli[6]=mtxC[9];
	mtxCli[8]=mtxC[2];
	mtxCli[9]=mtxC[6];
	mtxCli[10]=mtxC[10];
	// (The remaining seven elements never change)
	c->mtxCli = mtxCli;

        glScalef(0.08,0.08,0.08);
       	glTranslatef(-(c->r_eyepos[0]), -(c->r_eyepos[1]), -(c->r_eyepos[2]));
	render_objects(c);

#if 0
    /* Enable for speeds reporting (like g->r_speeds 1) */
    printf("faces: %d, leafs: %d\n", facelist.numfaces + translist.numfaces,
	   g->r_leafcount);
#endif
}

static void
render_walk_model(r_context_t *c, int n)
{
    if (n == 0)
	render_walk_node(c, 0, 0);
    else
	/* FIXME: models > 0 are just leafs ? */
	Error("Models > 0 not supported");
}

static int
classify_point(vec3_t p, int plane_n)
{
    /* Determine which side of plane p is on */
    plane_t *plane = &(g->r_planes[plane_n]);

    return (vec_dot(p, plane->vec) < plane->offset ? -1 : 1);
}

static void
render_walk_node(r_context_t *c, int n, int accept)
{
    node_t *node = &(g->r_nodes[n]);

    if (!accept)
    {
	/* Test the node bounding box for intersection with the view volume */
	int clipstate = cliptest_bbox(c, node->bbox);
	/* If this node is rejected, reject all sub-nodes */
	if (!clipstate) return;
	/* If this node is trivially accepted, accept all sub-nodes */
	if (clipstate == 2) accept = 1;
    }

    /* Classify eye wrt splitting plane */
    if (classify_point(c->r_eyepos, node->plane) > 0)
    {
	/* In front of plane: render front first, then back */
	if (node->children[0] < 0)
	    render_walk_leaf(c, -(node->children[0] + 1), accept);
	else
	    render_walk_node(c, node->children[0], accept);
	if (node->children[1] < 0)
	    render_walk_leaf(c, -(node->children[1] + 1), accept);
	else
	    render_walk_node(c, node->children[1], accept);
    }
    else
    {
	/* Behind plane: render back first, then front */
	if (node->children[1] < 0)
	    render_walk_leaf(c, -(node->children[1] + 1), accept);
	else
	    render_walk_node(c, node->children[1], accept);
	if (node->children[0] < 0)
	    render_walk_leaf(c, -(node->children[0] + 1), accept);
	else
	    render_walk_node(c, node->children[0], accept);	
    }
}

static void
render_walk_leaf(r_context_t *c, int n, int accept)
{
    leaf_t *leaf = &(g->r_leafs[n]);
    int i;

    /* Test visibility before bounding box */
    if (c->r_eyecluster >= 0)
    {
	if (! BSP_TESTVIS(c->r_eyecluster, leaf->cluster)) return;
    }
    
    if (!accept)
    {
	if (!cliptest_bbox(c, leaf->bbox)) return;
    }    

    c->r_leafcount++;
    
    for (i=0; i < leaf->numfaces; ++i)
    {
	render_walk_face(c, g->r_lfaces[i + leaf->firstface]);
    }
}

static void
render_walk_face(r_context_t *c, int n)
{
    face_t *face = &(g->r_faces[n]);

    /* Check if face is already included in the facelist */
    if (c->r_faceinc[n]) return;
    c->r_faceinc[n] = 1;

    if (face->facetype == FACETYPE_NORMAL)
    {
	/* Face plane culling */
	/* FIXME: This simple test is clearly not sufficient.
	   Q3A gets a lot more culling at this point. */
	/* Ignore this test for the time being
	if (vec_dot(face->v_norm, g->r_movedir) > cos_fov)
	    return;
	*/
    }
    
    else if (face->facetype == FACETYPE_MESH)
    {
	/* Check bounding box for meshes */
	if (!cliptest_bboxf(c, face->bbox))
	    return;
    }

    /* Check for sky flag */
    if (g->r_shaders[face->shader].flags & SHADER_SKY)
    {
	/* Push to sky list */
	c->skylist[c->numsky++] = n;
    }

    /* Check for transparent */
    else if (g->r_shaders[face->shader].flags & SHADER_TRANSPARENT)
    {
	c->translist.faces[c->translist.numfaces].face = n;
	c->translist.faces[c->translist.numfaces++].sortkey = SORTKEY(face);
    }

    /* Normal face */
    else
    {
	/* Push face to facelist */
	c->facelist.faces[c->facelist.numfaces].face = n;
	c->facelist.faces[c->facelist.numfaces++].sortkey = SORTKEY(face);
    }
}

static void
gen_clipmat(r_context_t *c)
{
    mat4_t modelview;
    mat4_t proj;

    /* Get the modelview and projection matricies from GL */
    glGetFloatv(GL_MODELVIEW_MATRIX, modelview);
    glGetFloatv(GL_PROJECTION_MATRIX, proj);

    /* Multiply to get clip coordinate transformation */
    mat4_mmult(proj, modelview, c->clipmat);
}

static int
cliptest_point(vec4_t p)
{
    int mask = 0;
    const float cx = p[0];
    const float cy = p[1];
    const float cz = p[2];
    const float cw = p[3];      
    
    if (cx >  cw) mask |= CLIP_RIGHT_BIT;
    if (cx < -cw) mask |= CLIP_LEFT_BIT;
    if (cy >  cw) mask |= CLIP_TOP_BIT;
    if (cy < -cw) mask |= CLIP_BOTTOM_BIT;
    if (cz >  cw) mask |= CLIP_FAR_BIT;
    if (cz < -cw) mask |= CLIP_NEAR_BIT;

    return mask;
}

/* Test bounding box for intersection with view fustrum.
 * Return val:   0 = reject
 *               1 = accept
 *               2 = trivially accept (entirely in fustrum)
 */
static int
cliptest_bboxf(r_context_t *c, bboxf_t bv)
{
    static int corner_index[8][3] =
    {
	{0, 1, 2}, {3, 1, 2}, {3, 4, 2}, {0, 4, 2},
	{0, 1, 5}, {3, 1, 5}, {3, 4, 5}, {0, 4, 5}
    };

    vec4_t corner[8];
    int clipcode, clip_or, clip_and, clip_in;
    int i;

    /* Check if eye point is contained */
    if (c->r_eyepos[0] >= bv[0] && c->r_eyepos[0] <= bv[3] &&
	c->r_eyepos[1] >= bv[1] && c->r_eyepos[1] <= bv[4] &&
	c->r_eyepos[2] >= bv[2] && c->r_eyepos[2] <= bv[5])
	return 1;
    
    clip_in = clip_or = 0; clip_and = 0xff;
    for (i=0; i < 8; ++i)
    {
	corner[i][0] = bv[corner_index[i][0]];
	corner[i][1] = bv[corner_index[i][1]];
	corner[i][2] = bv[corner_index[i][2]];
	corner[i][3] = 1.0;

	mat4_vmult(c->clipmat, corner[i], corner[i]);
	clipcode = cliptest_point(corner[i]);
	clip_or |= clipcode;
	clip_and &= clipcode;
	if (!clipcode) clip_in = 1;
    }

    /* Check for trival acceptance/rejection */
    if (clip_and) return 0;
    if (!clip_or) return 2;
    if (clip_in) return 1;   /* At least one corner in view fustrum */

#if 0
    /* FIXME: need something better for this. */
    /* Maybe find maximum radius to each corner */
    {
	/* Normalize coordinates */
	vec3_t center, rad;
	float cw;

	cw = 1.0f/corner[0][3];
	vec_scale(corner[0], cw, corner[0]);
	corner[0][3] = 1.0;
	cw = 1.0f/corner[6][3];
	vec_scale(corner[6], cw, corner[6]);
	corner[6][3] = 1.0;

	/* Check for non-trivial acceptance */
	vec_avg(corner[0], corner[6], center);
	vec_sub(corner[0], center, rad);
	if (sqrt(vec_dot(center, center)) -
	    sqrt(vec_dot(rad, rad)) <= 1.41421356)
	    return 1;
    }
	
    return 0;
#endif
    return 1;
}

static int
cliptest_bbox(r_context_t *c, bbox_t bbox)
{
    bboxf_t bv;

    bv[0] = (float)bbox[0];
    bv[1] = (float)bbox[1];
    bv[2] = (float)bbox[2];
    bv[3] = (float)bbox[3];
    bv[4] = (float)bbox[4];
    bv[5] = (float)bbox[5];

    return cliptest_bboxf(c, bv);
}

/* static int */
int find_cluster(vec3_t pos)
{
    node_t *node;
    int cluster = -1;
    int leaf = -1;
    
    node = &(g->r_nodes[0]);

    /* Find the leaf/cluster containing the given position */
    
    while (1)
    {
	if (classify_point(pos, node->plane) > 0)
	{
	    if (node->children[0] < 0)
	    {
		leaf = -(node->children[0] + 1);
		break;
	    }
	    else
	    {
		node = &(g->r_nodes[node->children[0]]);
	    }
	}
	else
	{
	    if (node->children[1] < 0)
	    {
		leaf = -(node->children[1] + 1);
		break;
	    }
	    else
	    {
		node = &(g->r_nodes[node->children[1]]);
	    }
	}	    
    }

    if (leaf >= 0)
	cluster = g->r_leafs[leaf].cluster;
    return cluster;
}

static int
face_cmp(const void *a, const void *b)
{
    return ((const rendface_t*)a)->sortkey - ((const rendface_t*)b)->sortkey;
}

static void
sort_faces(r_context_t *c)
{
    /* FIXME: expand qsort code here to avoid function calls */
    qsort((void*)c->facelist.faces, c->facelist.numfaces, sizeof(rendface_t),
	  face_cmp);
}
