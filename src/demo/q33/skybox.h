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
#ifndef __SKYBOX_H__
#define __SKYBOX_H__

/* The Q3A skybox has 5 sides (no bottom), but we include all 6 */
enum
{
    SKYBOX_TOP    = 0,
    SKYBOX_FRONT,
    SKYBOX_RIGHT,
    SKYBOX_BACK,
    SKYBOX_LEFT,
    SKYBOX_BOTTOM
};

typedef struct
{
    int numpoints;
    vec3_t *points[6];     /* World coords */
    texcoord_t *tex_st[6]; /* Skybox mapped texture coords */
    int numelems;
    uint_t *elems;
} skybox_t;

#ifdef __cplusplus
extern "C" {
#endif

void skybox_create(void);
void skybox_free(void);

#ifdef __cplusplus
}
#endif


#endif /*_SKYBOX_H__*/
