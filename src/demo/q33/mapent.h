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
#ifndef __MAPENT_H__
#define __MAPENT_H__

/* A "mapent" is an entity from the BSP file that is displayed on the map,
 * such as a rocketlauncher.  The mapent class ("rocketlauncher") contains
 * 1 or 2 md3 model "parts".  The mapent instance (a particular rocketlauncher)
 * contains location and visibility info.
 */

#define MAPENT_MAX_PARTS 2

typedef struct
{
    char *md3name;
    float rotspeed;   /* Degrees per sec. (signed) */
    float scale;      /* Extra scale info */
    int md3index;     /* Index to md3models array */
} mapent_part_t;    

typedef struct
{
    char *name;       /* Class name */
    int loaded;       /* Flag: this class has been loaded */
    int numparts;
    mapent_part_t parts[MAPENT_MAX_PARTS];
    float bobheight;  /* Amplitude of bob */
    float bobspeed;   /* Bobs per second */
} mapent_class_t;

typedef struct
{
    int klass;       /* mapent class index */
    int cluster;     /* PVS cluster */
    vec3_t origin;   /* World coords */
    float bobspeed;  /* Override class value */
} mapent_inst_t;

#ifdef __cplusplus
extern "C" {
#endif

void mapent_loadall(void);
void mapent_freeall(void);

#ifdef __cplusplus
}
#endif

#endif /*__MAPENT_H__*/
