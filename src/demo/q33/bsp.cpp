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
#include <stdio.h>
#include <string.h>
#include "util.h"
#include "pak.h"
#include "bsp.h"
#include "entity.h"

#include <sys/stat.h>

#include "globalshared.h"
extern global_shared_t *g;

#define BSPHEADERID  (*(int*)"IBSP")
#define BSPVERSION 46

#define SHADERS_ADD 100  /* Room for additional shaders */

/* BSP lumps in the order they appear in the header */
enum
{
    ENTITIES,
    SHADERREFS,
    Q_PLANES,
    NODES,
    LEAFS,
    LFACES,
    UNKNOWN_LUMP_7,
    MODELS,
    UNKNOWN_LUMP_9,
    UNKNOWN_LUMP_10,
    VERTS,
    ELEMS,
    UNKNOWN_LUMP_13,
    FACES,
    LIGHTMAPS,
    UNKNOWN_LUMP_16,
    VISIBILITY,
    NUM_LUMPS
};

static struct header
{
    int id, ver;
    struct { int fileofs, filelen; } lump[NUM_LUMPS];
} *bspheader;

static byte_t *bspdata;

static int readlump(int lump, void** mem, size_t elem);

static void swaplump(int lump, void *mem);
static void swapverts(int lump, void *mem);

#define READLUMP(lump,val) \
      g->r_num##val = readlump(lump, (void**)&(g->r_##val), sizeof(*(g->r_##val)))

/* examines the given file and returns it's size */
int file_size(const char *fname)
{
	struct stat s;
	stat(fname, &s);
	return(s.st_size);
}

void
bsp_read(const char *fname)
{
    int len, i;

    if (!pak_open(fname))
      Error("Could not read bsp file %s", fname);
    len = pak_getlen();
    bspdata = (byte_t*)malloc(len);
    pak_read(bspdata, len, 1);
    pak_close();

    bspheader = (struct header*)bspdata;

    BYTESWAP(bspheader->ver);
    for (i=0; i<NUM_LUMPS; i++) {
      BYTESWAP(bspheader->lump[i].fileofs);
      BYTESWAP(bspheader->lump[i].filelen);
    }
   
    if (bspheader->id != BSPHEADERID)
	Error("Not a bsp file: %s", fname);
    if (bspheader->ver != BSPVERSION)
	Error("Bad bsp file version");

    /* Make additional room for shader refs to be added later */
    g->r_numshaders = bspheader->lump[SHADERREFS].filelen / sizeof(shaderref_t);
    g->r_shaderrefs = (shaderref_t*)gc_malloc((g->r_numshaders + SHADERS_ADD) *
					sizeof(shaderref_t));
    memcpy(g->r_shaderrefs, bspdata + bspheader->lump[SHADERREFS].fileofs,
	   g->r_numshaders * sizeof(shaderref_t));
    for (i=0; i<g->r_numshaders; i++) {
      BYTESWAP(g->r_shaderrefs[i].unknown[0]);
      BYTESWAP(g->r_shaderrefs[i].unknown[1]);
    }
    g->r_addshaderstart = g->r_numshaders;
					    
    READLUMP(Q_PLANES, planes);
    READLUMP(NODES, nodes);
    READLUMP(LEAFS, leafs);
    READLUMP(LFACES, lfaces);
    READLUMP(MODELS, models);
    READLUMP(VERTS, verts);
    READLUMP(ELEMS, elems);
    READLUMP(FACES, faces);
    g->r_lightmapsize = readlump(LIGHTMAPS, (void**)&(g->r_lightmapdata), 1);
    (void)readlump(VISIBILITY, (void**)&(g->r_visibility), 1);

    /* Now byte-swap the lumps if we're on a big-endian machine... */
    swaplump(Q_PLANES, g->r_planes);
    swaplump(NODES, g->r_nodes);
    swaplump(LEAFS, g->r_leafs);
    swaplump(LFACES, g->r_lfaces);
    swaplump(MODELS, g->r_models);
    /* swaplump(VERTS, g->r_verts); */
    swapverts(VERTS, g->r_verts);
    swaplump(ELEMS, g->r_elems);
    swaplump(FACES, g->r_faces);
    BYTESWAP(g->r_visibility->numclusters);
    BYTESWAP(g->r_visibility->rowsize);

    entity_parse(bspheader->lump[ENTITIES].filelen,
		 (char*)(bspdata + bspheader->lump[ENTITIES].fileofs));
    
    free(bspdata);
}

void
bsp_list(void)
{
    printf("Contents of BSP file:\n\n");
    printf("num entities     %d\n", g->g_numentities);
    printf("num models       %d\n", g->r_nummodels);
    printf("num shaders      %d\n", g->r_numshaders);
    printf("num planes       %d\n", g->r_numplanes);
    printf("num verts        %d\n", g->r_numverts);
    printf("num vertex elems %d\n", g->r_numelems);
    printf("num leafs        %d\n", g->r_numleafs);
    printf("num nodes        %d\n", g->r_numnodes);
    printf("num faces        %d\n", g->r_numfaces);    
    printf("num lfaces       %d\n", g->r_numlfaces);
    printf("vis. clusters    %d\n", g->r_visibility->numclusters);
    
}

void
bsp_free(void)
{
    gc_free(g->r_models);
    gc_free(g->r_shaderrefs);
    gc_free(g->r_verts);
    gc_free(g->r_planes);
    gc_free(g->r_leafs);
    gc_free(g->r_nodes);
    gc_free(g->r_faces);
    gc_free(g->r_lfaces);
    gc_free(g->r_elems);
    gc_free(g->r_visibility);

    entity_free();
}

/* Add shader to the shaderref list (from md3 loads) */
int
bsp_addshaderref(const char *shadername)
{
    int i;
    static int calls = 0;

    /* Check for shader already in the list */
    for (i=0; i < g->r_numshaders; i++)
    {
	if (!strcmp(g->r_shaderrefs[i].name, shadername))
	    return i;
    }
    
    if (++calls > SHADERS_ADD)
	Error("Too many additional shaders");
        
    strcpy(g->r_shaderrefs[g->r_numshaders++].name, shadername);    
    return g->r_numshaders-1;
}

static int
readlump(int lump, void** mem, size_t elem)
{
    int len = bspheader->lump[lump].filelen;
    int num = len / elem;
    *mem = gc_malloc(len);

    memcpy(*mem, bspdata + bspheader->lump[lump].fileofs, num * elem);

    return num;
}

static void
swaplump(int lump, void *mem)
{
#ifdef ASHOCK_BIG_ENDIAN
  int i, len=bspheader->lump[lump].filelen;
  for (i=0; i<len>>2; i++)
  {
    unsigned *ptr = (unsigned *)mem;
    BYTESWAP(ptr[i]);
  }
#endif /*ASHOCK_BIG_ENDIAN*/    
}

/* vertex_t has 40 bytes + color (4 bytes) */
/* so do not swap every 11th word */
static void
swapverts(int lump, void *mem)
{
#ifdef ASHOCK_BIG_ENDIAN
  int i, len=bspheader->lump[lump].filelen;
  for (i=0; i<len>>2; i++)
  {
    unsigned *ptr = (unsigned *)mem;
    if ((i+1)%11 != 0) /* do not swap color */
      BYTESWAP(ptr[i]);
  }
#endif
}
