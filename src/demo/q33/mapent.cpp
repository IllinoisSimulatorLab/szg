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
#include "entity.h"
#include "bsp.h"
#include "md3.h"
#include "mapent.h"
#include <stdio.h>
#include <string.h>

#include "globalshared.h"
extern global_shared_t *g;

#define DEFBOB 5.0f, 1.0f
#define DEFNOBOB 1.0f, 1.0f
#define DEFROT 180.0f
#define DEFSCALE 1.0f

/* This information should come from somewhere outside the rendering
 * engine.  I think Q3A puts it in one of the .qvm files...
 * So, these values are just guesses.
 */  
static mapent_class_t mapent_classinit[] =
{
    {"weapon_shotgun", 0, 1,
     {{"models/weapons2/shotgun/shotgun.md3", DEFROT, 2.0f, 0}, },
     DEFBOB},
    
    {"weapon_plasmagun", 0, 1,
     {{"models/weapons2/plasma/plasma.md3", DEFROT, 1.5f, 0}, },
     DEFBOB},

    {"weapon_railgun", 0, 1,
     {{"models/weapons2/railgun/railgun.md3", DEFROT, 1.5f, 0}, },
     DEFBOB},

    {"weapon_lightning", 0, 1,
     {{"models/weapons2/lightning/lightning.md3", DEFROT, 1.5f, 0}, },
     DEFBOB},

    {"weapon_rocketlauncher", 0, 1,
     {{"models/weapons2/rocketl/rocketl.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},

    {"ammo_bullets", 0, 1,
     {{"models/powerups/ammo/machinegunam.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},

    {"ammo_shells", 0, 1,
     {{"models/powerups/ammo/shotgunam.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},
    
    {"ammo_cells", 0, 1,
     {{"models/powerups/ammo/plasmaam.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},

    {"ammo_slugs", 0, 1,
     {{"models/powerups/ammo/railgunam.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},

    {"ammo_lightning", 0, 1,
     {{"models/powerups/ammo/lightningam.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},

    {"ammo_rockets", 0, 1,
     {{"models/powerups/ammo/rocketam.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},    

    {"item_armor_body", 0, 1,
     {{"models/powerups/armor/armor_red.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},    

    {"item_armor_combat", 0, 1,
     {{"models/powerups/armor/armor_yel.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},    

    {"item_armor_shard", 0, 1,
     {{"models/powerups/armor/shard.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},    

    {"item_health_mega", 0, 2,
     {{"models/powerups/health/mega_cross.md3", 2.0*DEFROT, DEFSCALE, 0},
      {"models/powerups/health/mega_sphere.md3", 0.0f, DEFSCALE, 0} },
     DEFBOB},       

    {"item_health_large", 0, 2,
     {{"models/powerups/health/large_cross.md3", 2.0*DEFROT, DEFSCALE, 0},
      {"models/powerups/health/large_sphere.md3", 0.0f, DEFSCALE, 0} },
     DEFBOB},       

    {"item_health", 0, 2,
     {{"models/powerups/health/medium_cross.md3", 2.0*DEFROT, DEFSCALE, 0},
      {"models/powerups/health/medium_sphere.md3", 0.0f, DEFSCALE, 0} },
     DEFBOB},       

    {"item_health_small", 0, 2,
     {{"models/powerups/health/small_cross.md3", 2.0*DEFROT, DEFSCALE, 0},
      {"models/powerups/health/small_sphere.md3", 0.0f, DEFSCALE, 0} },
     DEFBOB},       

    {"item_quad", 0, 2,
     {{"models/powerups/instant/quad.md3", DEFROT, DEFSCALE, 0},
      {"models/powerups/instant/quad_ring.md3", -DEFROT, 1.2f, 0} },
     DEFBOB},       

    {"holdable_teleporter", 0, 1,
     {{"models/powerups/holdable/teleporter.md3", DEFROT, DEFSCALE, 0}, },
     DEFBOB},       
   
    {NULL} /* Sentinel */
};
    
static void mapent_newinst(int klass, int entity);
static void mapent_loadclass(int klass);

void
mapent_loadall(void)
{
    int i, j, totparts;
    const char *cname;

    /* printf("Initializing Map Models\n"); */
    
    /* Count classes */
    g->g_mapent_numinst = 0;
    g->g_mapent_numclasses = 0;
    while (mapent_classinit[g->g_mapent_numclasses].name)
	g->g_mapent_numclasses++;

    /* Count parts and init md3 loader */
    totparts = 0;
    for (i=0; i < g->g_mapent_numclasses; i++)
	totparts += mapent_classinit[i].numparts;
    md3_init(totparts);
    
    /* Alloc arrays */
    g->g_mapent_class = (mapent_class_t*) gc_malloc(g->g_mapent_numclasses *
					     sizeof(mapent_class_t));
    g->g_mapent_inst = (mapent_inst_t*) gc_malloc(g->g_numentities *
					   sizeof(mapent_inst_t));
    memcpy(g->g_mapent_class, mapent_classinit, g->g_mapent_numclasses *
	   sizeof(mapent_class_t));

    /* Look for mapents in entities list */
    for (i=0; i < g->g_numentities; i++)
    {
	/* Reject notfree instances */
	if (entity_float(i, "notfree"))
	    continue;
		
	cname = entity_value(i, "classname");
	for (j=0; j < g->g_mapent_numclasses; j++)
	{
	    if (!strcmp(cname, g->g_mapent_class[j].name))
	    {
		/* Load class if necessary */
		if (!(g->g_mapent_class[j].loaded))
		    mapent_loadclass(j);

		/* Make new mapent instance */
		mapent_newinst(j, i);
		break;
	    }
	}
    }
}

void
mapent_freeall(void)
{
    md3_free();
    gc_free(g->g_mapent_class);
    gc_free(g->g_mapent_inst);
}

static void
mapent_loadclass(int klass)
{
    int i;

    for (i=0; i < g->g_mapent_class[klass].numparts; i++)
    {
	g->g_mapent_class[klass].parts[i].md3index =
	    md3_load(g->g_mapent_class[klass].parts[i].md3name);
    }
    g->g_mapent_class[klass].loaded = 1;
}

static void
mapent_newinst(int klass, int entity)
{
    int inst;

    inst = g->g_mapent_numinst;
    g->g_mapent_numinst++;

    g->g_mapent_inst[inst].klass = klass;
    entity_vec3(entity, "origin", g->g_mapent_inst[inst].origin);
    g->g_mapent_inst[inst].cluster = -1;

    /* FIXME: Q3A mapents don't all bob at the same rate.  Is it random? */
    g->g_mapent_inst[inst].bobspeed = g->g_mapent_class[klass].bobspeed;
}
