/* CAVElib interface
 * Copyright (C) 2001 Paul Rajlich
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
#include <string.h>
#include "globalshared.h"

/* defaults - see ui_read_args() in uicommon.c */
void init_global_shared(global_shared_t *g) {

  g->r_fullscreen = 0;
  g->r_notextures = 0;
  g->r_drawitems = 1;
  g->r_drawhud = 1;

  g->r_texture_data = NULL;
  g->r_lockpvs = 0;
  g->r_gamma = 2.8f; // originally 1.7;  brighter for cube.
  g->r_brightness = 0.0f;
  g->r_setup_projection = 1;
  g->r_viewport_w = 640;
  g->r_viewport_h = 480;
  g->r_eyefov = 90.0f;
  g->r_sky = 0;
  g->r_title = 1;
  g->r_calc_fps = 1;
  g->r_fps = 0.0;

  /* performance */
  g->r_lodbias = 0;        /* raise to decrease texture quality */
  g->r_maxmeshlevel = 5;   /* lower to decrease geometric complexity */
  g->r_subdivisiontol = 5;

  g->g_collision = 1;
  g->g_collision_buffer = 36.0f;
  g->g_move = 0;
  g->g_acceleration = 1000.0;
  g->g_velocity = 0.0f;
  g->g_maxvel = 400.0f;
  g->g_keylook = 0;

  strcpy(g->g_bsp_fname, "maps/q3tourney2.bsp");
  strcpy(g->g_pak_fnames[0], "pak0.pk3");
  strcpy(g->g_pak_fnames[1], "paul.pk3");
  g->g_pak_num = 2;

  strcpy(g->r_help_fname, "paul/helpGL.jpg");
}
