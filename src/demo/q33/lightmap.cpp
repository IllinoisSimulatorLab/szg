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
#include "lightmap.h"
#include "tex.h"
#include "glinc.h"

#include "globalshared.h"
extern global_shared_t *g;

// copied from tex.cpp
static inline int gammaCorrect(int intensity, float gamma) {
  return int(pow((intensity/255.0f), 1.0f/gamma) * 255);
}

void
lightmap_loadall()
{
    int i, texsize = (128*128*3);

    g->r_numlightmaptex = g->r_lightmapsize / texsize;
    
    for (i=0; i < g->r_numlightmaptex; ++i)
    {
	/* It would be better to modify the gamma settings of the graphics card
	   than to use these variables. (see tex.cpp's use of r_brightness) */
	if (g->r_gamma != 1.0 || g->r_brightness != 0)
	{
	    int j, val;
	    byte_t *c;

	    c = &(g->r_lightmapdata[i * texsize]);
	    for (j=0; j < texsize; j++, c++)
	    {
		/*val = (*c + g->r_brightness) * g->r_gamma;*/
                val = gammaCorrect(*c, g->r_gamma);
		if (val > 255) val = 255;
		*c = val;
	    }
	}
    }
}

void
lightmap_bindobjs(r_context_t *c)
{
  int i, texsize = (128*128*3);

  c->r_lightmaptex = (uint_t *) rc_malloc(g->r_numlightmaptex * sizeof(uint_t));
  glGenTextures(g->r_numlightmaptex, c->r_lightmaptex);

  for (i=0; i < g->r_numlightmaptex; ++i)
  {
	glBindTexture(GL_TEXTURE_2D, c->r_lightmaptex[i]);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 128, 128, 0, GL_RGB,
		     GL_UNSIGNED_BYTE, &(g->r_lightmapdata[i * texsize]));
  }
}

void
lightmap_freeall()
{
  gc_free(g->r_lightmapdata);
}

void
lightmap_freeobjs(r_context_t *c)
{
    glDeleteTextures(g->r_numlightmaptex, c->r_lightmaptex);
    rc_free(c->r_lightmaptex);
}
