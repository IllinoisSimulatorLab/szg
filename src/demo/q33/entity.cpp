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
#include <stdio.h>
#include <string.h>

#include "globalshared.h"
extern global_shared_t *g;

/* Key-value pair */
typedef struct
{
    char *key;
    char *val;
} epair_t;

/* Group of epairs */
typedef struct
{
    int firstpair;
    int numpairs;
} entity_t;

static char *entity_buf;
static epair_t *epairs;
static entity_t *entities;
static int numepairs;

void
entity_parse(int buflen, char *buf)
{
    int i, newlines, pair;
    char *c;
    
    /* Save local copy of buf */
    entity_buf = (char*)malloc(buflen);
    memcpy(entity_buf, buf, buflen);

    /* Count entities and pairs */
    newlines = g->g_numentities = 0;
    for (i = 0; i < buflen; i++)
    {
	if (entity_buf[i] == '{') g->g_numentities++;
	if (entity_buf[i] == '\n') newlines++;
    }
    numepairs = newlines - (2 * g->g_numentities);

    /* Alloc structures */
    epairs = (epair_t*)malloc(numepairs * sizeof(epair_t));
    entities = (entity_t*)malloc(g->g_numentities * sizeof(entity_t));

    c = entity_buf;
    pair = 0;
    for (i = 0; i < g->g_numentities; i++)
    {
	entities[i].firstpair = pair;
	entities[i].numpairs = 0;
	
	/* Skip to leading quote */
	while (*c != '"') c++;

	while (*c != '}')
	{
	    epairs[pair].key = ++c;
	    while (*c != '"') c++;
	    *c = '\0';
	    c += 3;

	    epairs[pair].val = c;
	    while (*c != '"') c++;
	    *c = '\0';
	    c += 2;
	    pair++;
	    entities[i].numpairs++;
	}
    }
}

void
entity_free(void)
{
    free(entities);
    free(epairs);
    free(entity_buf);
}

const char *
entity_value(int entity, const char *key)
{
    epair_t *pair;
    int i;

    pair = &epairs[entities[entity].firstpair];
    for (i = 0; i < entities[entity].numpairs; i++, pair++)
    {
	if (!strcmp(key, pair->key))
	    return pair->val;
    }
    return "";
}

float
entity_float(int entity, const char *key)
{
    return atof(entity_value(entity, key));
}

void
entity_vec3(int entity, const char *key, vec3_t vec)
{
    const char *val;

    val = entity_value(entity, key);
    sscanf(val, "%f %f %f", &vec[0], &vec[1], &vec[2]);
}
