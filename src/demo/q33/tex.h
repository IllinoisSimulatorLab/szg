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
#ifndef __TEX_H__
#define __TEX_H__

#define TEX_MISSING ((uint_t)-1)

#include "rendercontext.h"

#ifdef __cplusplus
extern "C" {
#endif

void tex_loadall();
void tex_freeall();
void tex_bindobjs(r_context_t *c);
void tex_freeobjs(r_context_t *c);

#ifdef __cplusplus
}
#endif

#endif /*__TEX_H__*/
