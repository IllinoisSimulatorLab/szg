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
#ifndef __RENDER_BACK_H__
#define __RENDER_BACK_H__

#include "rendercontext.h"

#ifdef __cplusplus
extern "C" {
#endif

void render_backend_init(r_context_t *c);
void render_backend_finalize(r_context_t *c);
void render_backend(r_context_t *c, facelist_t *facelist);
void render_backend_sky(r_context_t *c);
void render_backend_mapent(r_context_t *c, int mapent);

#ifdef __cplusplus
}
#endif


/* Preferentially sort by shader number, then lightmap */
/* FIXME: other things that could go in the sort key include transparency
 * and 'sort' directives from the shader scripts */
#define SORTKEY(face)  (((face)->shader << 16) + (face)->lm_texnum+1)


#endif /*__RENDER_BACK_H__*/
