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
#ifndef __RENDER_H__
#define __RENDER_H__

#include "rendercontext.h"

#ifdef __cplusplus
extern "C" {
#endif

int find_cluster(vec3_t pos);

void render_init(r_context_t *c);
void render_init_mapents();
void render_clear(void);
void render_draw(r_context_t *c);
void render_finalize(r_context_t *c);

#ifdef __cplusplus
}
#endif

#endif /*__RENDER_H__*/
