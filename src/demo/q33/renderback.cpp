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
#include <stdio.h> // debug
#include "util.h"
#include "bsp.h"
#include "shader.h"
#include "render.h"
#include "tex.h"
#include "lightmap.h"
#include "mesh.h"
#include "skybox.h"
#include "md3.h"
#include "mapent.h"
#include "renderback.h"
#include "glinc.h"

#ifdef WIN32
#include "glext.h"
PFNGLACTIVETEXTUREARBPROC               glActiveTextureARB = NULL;
PFNGLCLIENTACTIVETEXTUREARBPROC         glClientActiveTextureARB = NULL;
PFNGLLOCKARRAYSEXTPROC                  glLockArraysEXT = NULL;
PFNGLUNLOCKARRAYSEXTPROC                glUnlockArraysEXT = NULL;
#else
extern void glLockArraysEXT(GLint, GLsizei);
extern void glUnlockArraysEXT(void);
#endif


//*************BJS: here's something that makes it easier for me to compile
#define SW_REFLECTION
#define NO_CVA

#include "globalshared.h"
extern global_shared_t *g;


/* The back-end of the rendering pipeline does the actual drawing.
 * All triangles which share a rendering state (shader) are pushed together
 * into the 'arrays' structure.  This includes verts, texcoords, and element
 * numbers.  The renderer is then 'flushed': the rendering state is set
 * and the triangles are drawn.  The arrays and rendering state is then
 * cleared for the next set of triangles.
 */

/* FIXME: The manner in which faces are "pushed" to the arrays is really
   absimal.  I'm sure it could be highly optimized. */
/* FIXME: It would be nice to have a consistent view of faces, meshes,
   mapents, etc. so we don't have to have a "push" function for each. */

#define TWOPI 6.28318530718
#define TURB_SCALE 0.2

static void render_pushface(r_context_t *c, face_t *face);
static void render_pushmesh(r_context_t *c, mesh_t *mesh);
static void render_flush(r_context_t *c, int shader, int lmtex);
static double render_func_eval(double time, uint_t func, float *args);
static int render_setstate(r_context_t *c, shaderpass_t *pass, uint_t lmtex, int lmFlag);
static void render_clearstate(r_context_t *c, shaderpass_t *pass);
static void render_pushface_deformed(r_context_t *c, int shadernum, face_t *face);

static GLfloat mtxTP[16]= {
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, 0.5f, 0.0f, 0.0f,
    0.5f, 0.5f, 1.0f, 1.0f,
    0.0f, 0.0f, 0.0f, 0.0f};

#ifdef SW_REFLECTION
/* This matrix does an almost linear mapping from a reflection vector
   direction to the angle with the z axis, but it works for z>0 only,
   so reflection vectors with the wrong sign for z need to be adjusted.
   For hardware reflection vectors, use dual paraboloid maps instead.
*/
static GLfloat mtxS[16] = {
    -1.67721f, 0.0f, 0.0f, 0.0f,
    0.0f, -1.67721f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 1.67721f};

/* Software copy of modelview matrix for reflection vector generation */
static GLfloat mtxModelview[16];

#else
static GLfloat mtxSfront[16] = {
    -1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 1.0f };

/* This matrix is not used in the current single-pass implementation
static GLfloat mtxSback[16] = {
    -1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, -1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, -1.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 1.0f };
*/
#endif

void
render_backend_init(r_context_t *c)
{
    /* FIXME: need to account for extra verts/elems in meshes */
    
    c->arrays.verts = (vec4_t*) rc_malloc(g->r_numverts * sizeof(vec4_t));
    c->arrays.tex_st = (texcoord_t*) rc_malloc(g->r_numverts * sizeof(texcoord_t));
    c->arrays.lm_st = (texcoord_t*) rc_malloc(g->r_numverts * sizeof(texcoord_t));
    c->arrays.elems = (int*) rc_malloc(g->r_numelems * sizeof(int));
    c->arrays.colour = (colour_t*) rc_malloc(g->r_numverts * sizeof(colour_t));
    /* SG: For envmaps */
    c->arrays.normals = (vec3_t*) rc_malloc(g->r_numverts * sizeof(vec3_t));

#ifdef WIN32
    glActiveTextureARB   = (PFNGLACTIVETEXTUREARBPROC) wglGetProcAddress("glActiveTextureARB");
    glClientActiveTextureARB= (PFNGLCLIENTACTIVETEXTUREARBPROC) wglGetProcAddress("glClientActiveTextureARB");
    glLockArraysEXT = (PFNGLLOCKARRAYSEXTPROC) wglGetProcAddress("glLockArraysEXT");
    glUnlockArraysEXT = (PFNGLUNLOCKARRAYSEXTPROC) wglGetProcAddress("glUnlockArraysEXT");
#endif
}

void
render_backend_finalize(r_context_t *c)
{
    rc_free(c->arrays.verts);
    rc_free(c->arrays.tex_st);
    rc_free(c->arrays.lm_st);
    rc_free(c->arrays.elems);
    rc_free(c->arrays.colour);
    rc_free(c->arrays.normals); /* SG - for envmaps */
}

void
render_backend(r_context_t *c, facelist_t *facelist)
{
    int f, shader, lmtex;
    uint_t key;
    face_t *face;

    /* fprintf(stderr, "faces: %d\n", facelist->numfaces); */

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    c->arrays.numverts = c->arrays.numelems = 0;
    key = (uint_t)-1;
    /* for (f=0; f < facelist->numfaces; ++f) */
    for (f= facelist->numfaces * c->range[0];
         f < facelist->numfaces * c->range[1];
         ++f)
    {
        face = &(g->r_faces[facelist->faces[f].face]);

        /* Look for faces that share rendering state */
        if (facelist->faces[f].sortkey != key)
        {
            /* Flush the renderer and reset */
            if (f) render_flush(c, shader, lmtex);
            shader = face->shader;
            lmtex = face->lm_texnum;
            key = facelist->faces[f].sortkey;
        }

        /* Push the face to the triangle arrays */
        switch (face->facetype)
        {
            case FACETYPE_NORMAL:
            case FACETYPE_TRISURF:
                if (g->r_shaders[shader].flags & SHADER_DEFORMVERTS)
                    render_pushface_deformed(c, shader, face);
                else
                    render_pushface(c, face);
                break;
            case FACETYPE_MESH:
                render_pushmesh(c, &(g->r_meshes[facelist->faces[f].face]));
                break;
            default:
                break;
        }
    }
    /* Final flush to clear queue */
    render_flush(c, shader, lmtex);    
}

void
render_backend_sky(r_context_t *c)
{
    int s, i, shader;
    float skyheight;
    uint_t *elem;

    shader = g->r_faces[c->skylist[0]].shader;

    skyheight = g->r_shaders[shader].skyheight;
    c->arrays.numverts = c->arrays.numelems = 0;

    /* printf("sky %d %d %d\n", shader, g->r_skybox->numelems, g->r_skybox->numpoints); */

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    /* Center skybox on camera to give the illusion of a larger space */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(c->r_eyepos[0], c->r_eyepos[1], c->r_eyepos[2]);
    glScalef(skyheight, skyheight, skyheight);

    /* FIXME: Need to cull skybox based on face list */
    for (s=0; s < 6; s++)
    {
        elem = g->r_skybox->elems;
        for (i=0; i < g->r_skybox->numelems; i++)
        {
            c->arrays.elems[c->arrays.numelems++] = c->arrays.numverts + *elem++;
        }
        for (i=0; i < g->r_skybox->numpoints; i++)
        {
            vec_copy(g->r_skybox->points[s][i], c->arrays.verts[c->arrays.numverts]);
            c->arrays.verts[c->arrays.numverts][3] = 1.0f;
            vec2_copy(g->r_skybox->tex_st[s][i], c->arrays.tex_st[c->arrays.numverts]);
            c->arrays.numverts++;
        }
    }

    render_flush(c, shader, 0);

    /* Restore world space */
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void
render_backend_mapent(r_context_t *c, int mapent)
{
    int i, j, k;
    mapent_inst_t *inst = &(g->g_mapent_inst[mapent]);
    mapent_class_t *klass = &(g->g_mapent_class[inst->klass]);
    md3model_t *model;
    md3mesh_t *mesh;
    uint_t *elem;
    float funcargs[4];
    float bob;

    c->arrays.numverts = c->arrays.numelems = 0;

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    /* Calculate bob amount */
    funcargs[0] = funcargs[2] = 0.0f;
    funcargs[1] = klass->bobheight;
    funcargs[3] = inst->bobspeed;
    bob = (float)render_func_eval(c->r_frametime, SHADER_FUNC_SIN, funcargs);

    /* Translate to model origin + bob amount */
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glTranslatef(inst->origin[0], inst->origin[1], inst->origin[2] + bob);

    for (i=0; i < klass->numparts; i++)
    {
        model = &(g->r_md3models[klass->parts[i].md3index]);

        /* Scale and rotate part */
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
	// The two calls below are transformations local to this object
        glRotated(klass->parts[i].rotspeed * c->r_frametime, 0.0, 0.0, 1.0);
        glScalef(klass->parts[i].scale, klass->parts[i].scale,
                 klass->parts[i].scale);

        for (j = 0; j < model->nummeshes; j++)
        {
            mesh = &model->meshes[j];

            elem = mesh->elems;
            for (k = 0; k < mesh->numelems; k++)
            {
                c->arrays.elems[c->arrays.numelems++] = c->arrays.numverts + *elem++;
            }

            for (k = 0; k < mesh->numverts; k++)
            {
                vec_copy(mesh->points[k], c->arrays.verts[c->arrays.numverts]);
                c->arrays.verts[c->arrays.numverts][3] = 1.0f;
                /* SG - for envmaps */
                if(g->r_shaders[mesh->shader].flags & SHADER_NEEDNORMALS)
                    vec_copy(mesh->normals[k],
                             c->arrays.normals[c->arrays.numverts]);
                vec2_copy(mesh->tex_st[k], c->arrays.tex_st[c->arrays.numverts]);
                c->arrays.numverts++;
            }

            /* Draw it */
            render_flush(c, mesh->shader, 0);
        }
        /* Restore unrotated state */
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();

#if 0 // was: #ifdef SW_REFLECTION
        mtxObj[0]= mtxNorm[0] = 1.0;
        mtxObj[1]= mtxNorm[1] = 0.0;
        mtxObj[4]= mtxNorm[4] = 0.0;
        mtxObj[5]= mtxNorm[5] = 1.0;
        mtxObj[10]= mtxNorm[10] = 1.0;
        mtxObj[12]=0.0;
        mtxObj[13]=0.0;
        mtxObj[14]=0.0;
#endif
    }

    /* Restore world space */
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

static void
render_pushface(r_context_t *c, face_t *face)
{
    int i, *elem;
    vertex_t *vert;

    elem = &(g->r_elems[face->firstelem]);
    for (i = 0; i < face->numelems; ++i)
    {
        c->arrays.elems[c->arrays.numelems++] = c->arrays.numverts + *elem++;
    }
    
    vert = &(g->r_verts[face->firstvert]);
    for (i = 0; i < face->numverts; ++i)
    {
        vec_copy(vert->v_point, c->arrays.verts[c->arrays.numverts]);
        c->arrays.verts[c->arrays.numverts][3] = 1.0f;
        vec2_copy(vert->tex_st, c->arrays.tex_st[c->arrays.numverts]);
        vec2_copy(vert->lm_st, c->arrays.lm_st[c->arrays.numverts]);    
        if (g->r_shaders[face->shader].flags & SHADER_NEEDCOLOURS)
            colour_copy(vert->colour, c->arrays.colour[c->arrays.numverts]);
        /* SG - For envmaps */
        if (g->r_shaders[face->shader].flags & SHADER_NEEDNORMALS)
            vec_copy(vert->v_norm, c->arrays.normals[c->arrays.numverts]);
        vert++;
        c->arrays.numverts++;
    }       
}

static void
render_pushface_deformed(r_context_t *c, int shadernum, face_t *face)
{
    /* Push the face, deforming each vertex as we go. */
    /* FIXME: Better to deform vertexes after pushing, but where
       does the normal info come from ? */
    /* Only wave deformation supported here */
    shader_t *shader = &(g->r_shaders[shadernum]);
    float args[4], startoff, off, wavesize, deflect;
    int i, *elem;
    vertex_t *vert;
    vec3_t v;

    /* Setup wave function */
    args[0] = shader->deformv_wavefunc.args[0];
    args[1] = shader->deformv_wavefunc.args[1];
    args[3] = shader->deformv_wavefunc.args[3];
    startoff = shader->deformv_wavefunc.args[2];
    wavesize = shader->deformv_wavesize;

    elem = &(g->r_elems[face->firstelem]);
    for (i = 0; i < face->numelems; ++i)
    {
        c->arrays.elems[c->arrays.numelems++] = c->arrays.numverts + *elem++;
    }
        
    vert = &(g->r_verts[face->firstvert]);
    for (i = 0; i < face->numverts; ++i)
    {
        /* FIXME: this clearly isn't the way deform waves are applied to
           world coordinates.  For now, it at least waves the banners :) */
        off = (vert->v_point[0] + vert->v_point[1] + vert->v_point[2]) /
            wavesize;

        /* Evaluate wave function */
        args[2] = startoff + off;
        deflect = render_func_eval(c->r_frametime, shader->deformv_wavefunc.func, args);
        /* Deflect vertex along its normal vector by wave amount */
        vec_copy(vert->v_norm, v);
        vec_scale(v, deflect, v);
        vec_add(v, vert->v_point, v);

        /* Push it */
        vec_copy(v, c->arrays.verts[c->arrays.numverts]);
        c->arrays.verts[c->arrays.numverts][3] = 1.0f;
        /* SG: For envmaps */
        if (g->r_shaders[face->shader].flags & SHADER_NEEDNORMALS)
            vec_copy(vert->v_norm, c->arrays.normals[c->arrays.numverts]);
        vec2_copy(vert->tex_st, c->arrays.tex_st[c->arrays.numverts]);
        vec2_copy(vert->lm_st, c->arrays.lm_st[c->arrays.numverts]);    
        if (g->r_shaders[face->shader].flags & SHADER_NEEDCOLOURS)
            colour_copy(vert->colour, c->arrays.colour[c->arrays.numverts]);
        vert++;
        c->arrays.numverts++;   
    }       
}

static void
render_pushmesh(r_context_t *c, mesh_t *mesh)
{
    int u, v, i, *elem;

    elem = (int *) mesh->elems;
    for (i = 0; i < mesh->numelems; ++i)
    {
        c->arrays.elems[c->arrays.numelems++] = c->arrays.numverts + *elem++;
    }
    
    i = 0;    
    for (v = 0; v < mesh->size[1]; ++v)
    {
        for (u = 0; u < mesh->size[0]; ++u)
        {
            vec_copy(mesh->points[i], c->arrays.verts[c->arrays.numverts]);
            c->arrays.verts[c->arrays.numverts][3] = 1.0f;
            /* SG: For envmaps */
            vec_copy(mesh->normals[i], c->arrays.normals[c->arrays.numverts]);
            vec2_copy(mesh->tex_st[i],  c->arrays.tex_st[c->arrays.numverts]);
            vec2_copy(mesh->lm_st[i], c->arrays.lm_st[c->arrays.numverts]);
            c->arrays.numverts++;
            i++;
        }
    }
}

#ifdef NO_CVA
static void
render_stripmine(int numelems, int *elems)
{
    int toggle;
    uint_t a, b, elem;

    /* Vertexes are in tristrip order where possible.  If we can't lock
     * the vertex arrays (glLockArraysEXT), then it's better to send
     * tristrips instead of triangles (less transformations).
     * This function looks for and sends tristrips.
     */

    /* Tristrip order elems look like this:
     *  0 1 2 2 1 3 2 3 4 4 3 5 4 5 7 7 5 6  <-- elems
     *    b a a b b a b a a b b a b a a b b  <-- ab pattern
     *    \ 1 / \ 2 / \ 3 / \ 4 / \ 5 /      <-- baa/bba groups
     */
    
    elem = 0;
    while (elem < numelems)
    {
        toggle = 1;
        glBegin(GL_TRIANGLE_STRIP);
        /* glBegin(GL_LINE_STRIP); */
        
        glArrayElement(elems[elem++]);
        b = elems[elem++];
        glArrayElement(b);
        a = elems[elem++];
        glArrayElement(a);
        
        while (elem < numelems)
        {
            if (a != elems[elem] || b != elems[elem+1])
                break;
            
            if (toggle)
            {
                b = elems[elem+2];
                glArrayElement(b);
            }
            else
            {
                a = elems[elem+2];
                glArrayElement(a);
            }
            elem += 3;
            toggle = !toggle;
        }
        glEnd();
    }
}
#endif

static void
render_flush(r_context_t *c, int shadernum, int lmtex)
{
    int p;
    shader_t *shader = &(g->r_shaders[shadernum]);
    shaderpass_t *pass = NULL;
    
    if (c->arrays.numverts == 0) return;

    /* Face culling */
    if (shader->flags & SHADER_NOCULL)
        glDisable(GL_CULL_FACE);
    else
        glEnable(GL_CULL_FACE);

    /* FIXME: if compiled vertex arrays supported, lock vertex array here */
    /* glVertexPointer(4, GL_FLOAT, 0, c->arrays.verts); */
    glVertexPointer(3, GL_FLOAT, 16, c->arrays.verts);
    /* like the double negative? :-) */
#ifndef NO_CVA
    glLockArraysEXT(0, c->arrays.numverts);
#endif
 
    if (shader->flags & SHADER_NEEDCOLOURS)
        glColorPointer(4, GL_UNSIGNED_BYTE, 0, c->arrays.colour);

    /* SG - for envmaps */
    if (shader->flags & SHADER_NEEDNORMALS) {
        glEnable(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, c->arrays.normals);
    }
    else
        glDisable(GL_NORMAL_ARRAY);
    
    /* FIXME: Multitexturing, if supported...
     * Multitexturing can be handled by examining the number of passes
     * for this shader, and spreading them amongst available texture
     * units.  E.g. if there are 3 passes and 2 tex units, we do 2 at
     * once and then one -- glDrawElements() is called twice.
     */

    for (p=0; p < shader->numpasses; p++) {

      pass = &shader->pass[p];

      if (!render_setstate(c, &shader->pass[p], lmtex, shader->lmFlag))
        continue;

      if (c->r_tex_units > 1 && shader->lmFlag) {
        p++;
        pass = &shader->pass[p];
#ifndef NO_MULTITEX
        glActiveTextureARB(GL_TEXTURE1_ARB);
        glClientActiveTextureARB(GL_TEXTURE1_ARB);
#endif
        glEnable(GL_TEXTURE_2D);
        glEnableClientState( GL_TEXTURE_COORD_ARRAY );
        glDisableClientState(GL_COLOR_ARRAY);

        render_setstate(c, &shader->pass[p], lmtex, shader->lmFlag);

#ifdef NO_CVA   
        render_stripmine(c->arrays.numelems, c->arrays.elems);
#else
        glDrawElements(GL_TRIANGLES, c->arrays.numelems, GL_UNSIGNED_INT, c->arrays.elems);
#endif

        render_clearstate(c, &shader->pass[p]);
        glDisable(GL_TEXTURE_2D);
#ifndef NO_MULTITEX
        glActiveTextureARB(GL_TEXTURE0_ARB);
        glClientActiveTextureARB(GL_TEXTURE0_ARB);
#endif
        render_clearstate(c, &shader->pass[p-1]);
      }
      else {
#ifdef NO_CVA
        render_stripmine(c->arrays.numelems, c->arrays.elems);
#else
        glDrawElements(GL_TRIANGLES, c->arrays.numelems, GL_UNSIGNED_INT, c->arrays.elems);
#endif
        render_clearstate(c, &shader->pass[p]);
      }
    }
    
    /* Clear arrays */
    c->arrays.numverts = c->arrays.numelems = 0; 
#ifndef NO_CVA
    glUnlockArraysEXT();
#endif
}

static double
render_func_eval(double time, uint_t func, float *args)
{
    double x, y;

    /* Evaluate a number of time based periodic functions */
    /* y = args[0] + args[1] * func( (time + arg[3]) * arg[2] ) */
    
    x = (time + args[2]) * args[3];
    x -= floor(x);

    switch (func)
    {
        case SHADER_FUNC_SIN:
            y = sin(x * TWOPI);
            break;
            
        case SHADER_FUNC_TRIANGLE:
            if (x < 0.5)
                y = 4.0 * x - 1.0;
            else
                y = -4.0 * x + 3.0;
            break;
            
        case SHADER_FUNC_SQUARE:
            if (x < 0.5)
                y = 1.0;
            else
                y = -1.0;
            break;
            
        case SHADER_FUNC_SAWTOOTH:
            y = x;
            break;
            
        case SHADER_FUNC_INVERSESAWTOOTH:
            y = 1.0 - x;
            break;
    }

    return y * args[1] + args[0];
}

static int
render_setstate(r_context_t *c, shaderpass_t *pass, uint_t lmtex, int lmFlag)
{
    if (!lmFlag && (pass->flags & SHADER_BLEND))
    {
        glEnable(GL_BLEND);
        glBlendFunc(pass->blendsrc, pass->blenddst);
    }
    else
        glDisable(GL_BLEND);

    if (pass->flags & SHADER_ALPHAFUNC)
    {
        glEnable(GL_ALPHA_TEST);
        glAlphaFunc(pass->alphafunc, pass->alphafuncref);
    }
    else
        glDisable(GL_ALPHA_TEST);


    if (lmFlag) {
      glDepthFunc(GL_LEQUAL);
      glDepthMask(GL_TRUE);
    }
    else {
      glDepthFunc(pass->depthfunc);
      if (pass->flags & SHADER_DEPTHWRITE)
        glDepthMask(GL_TRUE);
      else
        glDepthMask(GL_FALSE);
    }

    if (pass->rgbgen == SHADER_GEN_IDENTITY)
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    else if (pass->rgbgen == SHADER_GEN_WAVE)
    {
        float rgb = (float)render_func_eval(c->r_frametime, pass->rgbgen_func.func,
                                            pass->rgbgen_func.args);
        glColor4f(rgb, rgb, rgb, 1.0f);
    }
    else if (pass->rgbgen == SHADER_GEN_VERTEX)
        /* FIXME: I don't think vertex colours are what is meant here */
        glEnableClientState(GL_COLOR_ARRAY);

    if (pass->flags & (SHADER_TCMOD | SHADER_TCGEN_ENV)) {
        /* Save identity texture transform */
        glMatrixMode(GL_TEXTURE);
        glPushMatrix();
    }
	
    if (pass->flags & SHADER_TCMOD)
    {
        /* Move center of texture to origin */
        glTranslatef(0.5f, 0.5f, 0.0f);
        
        /* FIXME: Is this the right order for these transforms ? */

        if (pass->tcmod & SHADER_TCMOD_ROTATE)
            glRotated(pass->tcmod_rotate * c->r_frametime, 0.0, 0.0, 1.0);
        if (pass->tcmod & SHADER_TCMOD_SCALE)
            glScalef(pass->tcmod_scale[0], pass->tcmod_scale[1], 1.0f);

        if (pass->tcmod & SHADER_TCMOD_TURB)
        {
            /* Don't know what the exact transform is for turbulance, but
               this seems to do something close: sin wave scaling in s
               and t with a 90 degrees phase difference */
            double x, y1, y2;
            x = (c->r_frametime + pass->tcmod_turb[2]) * pass->tcmod_turb[3];
            x -= floor(x);
            y1 = sin(x * TWOPI) * pass->tcmod_turb[1] + pass->tcmod_turb[0];
            y2 = sin((x+0.25) * TWOPI) * pass->tcmod_turb[1] +
                pass->tcmod_turb[0];
            glScaled(1.0+y1*TURB_SCALE, 1.0+y2*TURB_SCALE, 1.0);
        }

        if (pass->tcmod & SHADER_TCMOD_STRETCH)
        {
            double y = render_func_eval(c->r_frametime, pass->tcmod_stretch.func,
                                        pass->tcmod_stretch.args);
            glScaled(1.0/y, 1.0/y, 1.0);
        }
        
        if (pass->tcmod & SHADER_TCMOD_SCROLL)
            glTranslated(pass->tcmod_scroll[0] * c->r_frametime,
                         pass->tcmod_scroll[1] * c->r_frametime, 0.0);

        /* Replace center of texture */
        glTranslatef(-0.5f, -0.5f, 0.0f);
    }

    if (pass->flags & SHADER_LIGHTMAP)
    {
        /* Select lightmap texture */
        glTexCoordPointer(2, GL_FLOAT, 0, c->arrays.lm_st);
        glBindTexture(GL_TEXTURE_2D, c->r_lightmaptex[lmtex]);
    }
    else if (pass->flags & SHADER_ANIMMAP)
    {
        uint_t texobj;
        int frame;

        /* Animation: get frame for current time */
        frame = (int)(c->r_frametime * pass->anim_fps) % pass->anim_numframes;
        
        glTexCoordPointer(2, GL_FLOAT, 0, c->arrays.tex_st);
        texobj = c->r_textures[pass->anim_frames[frame]];
        if (texobj < 0) return 0;
        glBindTexture(GL_TEXTURE_2D, texobj);
    }
    else if (pass->flags & SHADER_TCGEN_ENV)
    {
        /* SG - environment mapping */
#ifdef SW_REFLECTION
        /* Software generation of reflection vectors - takes some CPU power, but works
           with OpenGL 1.1 without the cubemap texgen extensions, e.g. on an Onyx2.
           Also looks very similar to the original Q3A reflection maps. */
        int i;
        uint_t texobj;
        vec3_t p_e, v_e, n_e, r_e; // Eye space vectors
        texobj = c->r_textures[pass->texref];
        if(texobj < 0) return 0;
        glBindTexture(GL_TEXTURE_2D, texobj);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glMatrixMode(GL_TEXTURE);
        glMultMatrixf(mtxTP);
        glMultMatrixf(mtxS);
        /* mtxCli multiplication is done in software, to flip for z<0 */
	glGetFloatv(GL_MODELVIEW_MATRIX, mtxModelview);
        /* Calculate the reflection vectors in eye space in software */
        for(i=0; i< c->arrays.numverts; i++) {
            mat4_vmult(mtxModelview, c->arrays.verts[i], p_e); // To eye space
            vec_neg(p_e, v_e); // Vector from vertex to eye (eye at origin)
            vec_normalize(v_e); // Normalize view vector
	    /* If non-uniform scaling is used, the inverse of the transpose
	       of the modelview mtx should be used to transform normals */
            mat4_v3mult(mtxModelview, c->arrays.normals[i], n_e); // Transform normals
            vec_normalize(n_e); // Renormalize (modelview mtx might be scaled)
            vec_scale(n_e, 2.0*vec_dot(v_e, n_e), r_e);
            /* It would be nicer to have a separate array for these 3D texcoords. */
            vec_sub(r_e, v_e, r_e); // Final reflection vector r = 2(v dot n)n - v
	    mat4_v3mult(c->mtxCli, r_e, r_e);
	    // Swap coordinates to orient the envmap like in Q3A
            c->arrays.normals[i][0] = r_e[2];
            c->arrays.normals[i][1] = r_e[0];
            // Q3A-mimicking hack: flip reflection vectors with world y<0
            c->arrays.normals[i][2] = (r_e[1]>0.0 ? r_e[1] : -r_e[1]);
        }
        /* Use the generated vectors as 3D texture coordinates instead of normals */
        glTexCoordPointer(3, GL_FLOAT, 0, c->arrays.normals);
        /* Inhibit the vertex normals. (We have actually destroyed them now.) */
        glDisable(GL_NORMAL_ARRAY);
        glMatrixMode(GL_MODELVIEW);
#else
        /* Hardware reflection vectors - different but good looking, and fully
	   hardware accelerated. Requires either of the following extensions:
           GL_EXT_texture_cube_map, GL_ARB_texture_cube_map or GL_NV_texgen_reflection */
        uint_t texobj;
        glDisable(GL_TEXTURE_COORD_ARRAY); /* Overridden by TC generation */
        texobj = c->r_textures[pass->texref];
        if(texobj < 0) return 0;
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_EXT);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_EXT);
        glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_REFLECTION_MAP_EXT);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glEnable(GL_TEXTURE_GEN_R);
        glBindTexture(GL_TEXTURE_2D, texobj);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glMatrixMode(GL_TEXTURE);
        glMultMatrixf(mtxTP);
        glMultMatrixf(mtxSfront);
        glMultMatrixf(c->mtxCli);
        glMatrixMode(GL_MODELVIEW);
#endif
    }
    else
    {
        uint_t texobj;

        /* Set normal texture */
        glTexCoordPointer(2, GL_FLOAT, 0, c->arrays.tex_st);
        if (pass->texref < 0) return 0;
        texobj = c->r_textures[pass->texref];
        if (texobj < 0) return 0;
        glBindTexture(GL_TEXTURE_2D, texobj);
    }

    return 1;
}

static void
render_clearstate(r_context_t *c, shaderpass_t *pass)
{
    if (pass->flags & (SHADER_TCMOD | SHADER_TCGEN_ENV))
    {
        /* Revert to identity texture transform */
        glMatrixMode(GL_TEXTURE);
        glPopMatrix();
    }

    if (pass->rgbgen == SHADER_GEN_VERTEX)
        glDisableClientState(GL_COLOR_ARRAY);

    if (pass->flags | SHADER_TCGEN_ENV) {
        glDisable(GL_TEXTURE_GEN_R);
        glDisable(GL_TEXTURE_GEN_S); /* Disable envmap texture generation */
        glDisable(GL_TEXTURE_GEN_T);
        glEnable(GL_TEXTURE_COORD_ARRAY); /* Enable explicit (s,t) again */
    }
}
