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

#ifndef __GLOBAL_CONTEXT_H__
#define __GLOBAL_CONTEXT_H__

#include "util.h"
#include "bsp.h"
#include "render.h"
#include "tex.h"
#include "mesh.h"
#include "shader.h"
#include "skybox.h"
#include "entity.h"
#include "md3.h"
#include "mapent.h"
#include "uicommon.h"

#define START_TYPE_DEATHMATCH 0
#define START_TYPE_START 1

typedef struct {
  byte_t *rgb;
  int w, h, format;
} tex_data_t;

typedef struct {

  int r_fullscreen;
  int r_notextures;
  int r_drawitems;
  int r_drawhud;

  int r_nummodels, r_numverts, r_numplanes, r_numleafs, r_numnodes;
  int r_numshaders, r_numfaces, r_numlfaces, r_numelems;
  int r_lightmapsize;
  int r_addshaderstart;

  model_t *r_models;
  vertex_t *r_verts;
  plane_t *r_planes;
  leaf_t *r_leafs;
  node_t *r_nodes;
  shaderref_t *r_shaderrefs;
  face_t *r_faces;
  int *r_lfaces;
  int *r_elems;
  byte_t *r_lightmapdata;
  visibility_t *r_visibility;

  vec3_t r_eyepos, r_eyepos_sav;
  vec3_t r_movedir;
  float r_eye_az, r_eye_el;

  float r_eyefov;
  int r_eyecluster;
  int r_lockpvs; 

  int r_lodbias;
  float r_gamma;      // approximated gamma function
  float r_brightness; // approximated gamma function

  int r_setup_projection;
  int r_viewport_w;
  int r_viewport_h;
  int r_sky;
  int r_title;
  int r_calc_fps;
  float r_fps;
  int g_collision;
  float g_collision_buffer;
  float g_velocity;
  float g_acceleration;
  float g_maxvel;
  int g_move;
  int g_keylook;

  float r_subdivisiontol;
  int r_maxmeshlevel;
  int r_nummeshes;
  mesh_t *r_meshes;

  int r_numlightmaptex;
  int r_numtextures;
  tex_data_t *r_texture_data;
  shader_t *r_shaders;
  texfile_t *r_texfiles;

  skybox_t *r_skybox;

  int g_numentities;

  int r_nummd3models;
  md3model_t *r_md3models;

  int g_mapent_numclasses;
  int g_mapent_numinst;
  mapent_class_t *g_mapent_class;
  mapent_inst_t *g_mapent_inst;

  char r_help_fname[256];
  char g_bsp_fname[256];
  char g_pak_fnames[10][256];
  int g_pak_num;

} global_shared_t;

#ifdef __cplusplus
extern "C" {
#endif

void init_global_shared(global_shared_t *g);
void *gc_malloc(size_t size); /* for allocating global data */
void gc_free(void *mem);      /* for deallocating global shared data */

#ifdef __cplusplus
}
#endif


#endif
