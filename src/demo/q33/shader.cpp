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
#include "pak.h"
#include "bsp.h"
#include "shader.h"
#include "glinc.h"
#include <stdio.h>
#include <string.h>

#define SHADERBUF_SIZE  (1024*1024-8)
#define MAX_NUM_TEXTURES 512
#define SHADER_ARGS_MAX (SHADER_ANIM_FRAMES_MAX+1)

#define LOWERCASE(c) ((c) <= 'Z' && (c) >= 'A' ? (c) + ('a'-'A') : (c))

#include "globalshared.h"
extern global_shared_t *g;

/* Maps shader keywords to functions */
typedef struct
{
    char *keyword;
    int minargs, maxargs;
    void (* func)(shader_t *shader, shaderpass_t *pass,
		  int numargs, char **args);
} shaderkey_t;

static int shader_lookup(const char *name);
static void shader_skip(void);
static void shader_read(void);
static void shader_readpass(shader_t *shader, shaderpass_t *pass);
static void shader_parsetok(shader_t *shader, shaderpass_t *pass, shaderkey_t *keys, char *tok);
static void shader_makedefaults(void);
/* static int shader_gettexref(const char *fname); */
static char* nexttok(void);
static char* nextarg(void);
static void Syntax(void);
static void shader_parsefunc(char **args, shaderfunc_t *func);

static char *shaderbuf, *curpos, *endpos;
static int *shaderfound;  /* Shader found in shader script files */

/****************** shader keyword functions ************************/

static void
shader_cull(shader_t *shader, shaderpass_t *pass, int numargs, char **args)
{
    if (!strcmp(args[0], "disable") || !strcmp(args[0], "none"))
	shader->flags |= SHADER_NOCULL;
}

static void
shader_surfaceparm(shader_t *shader, shaderpass_t *pass, int numargs,
		   char **args)
{
    if (!strcmp(args[0], "trans"))
	shader->flags |= SHADER_TRANSPARENT;
    else if (!strcmp(args[0], "sky"))
	shader->flags |= SHADER_SKY;
}

static void
shader_skyparms(shader_t *shader, shaderpass_t *pass, int numargs,
		char **args)
{
    shader->skyheight = atof(args[1]);
}

static void
shader_nomipmaps(shader_t *shader, shaderpass_t *pass, int numargs,
		 char **args)
{
    shader->flags |= SHADER_NOMIPMAPS;
}

static void
shader_deformvertexes(shader_t *shader, shaderpass_t *pass, int numargs,
		      char **args)
{
    if (!strcmp(args[0], "wave"))
    {
	if (numargs != 7)
	    Syntax();
	shader->flags |= SHADER_DEFORMVERTS;
	shader->deformv_wavesize = atof(args[1]);
	shader_parsefunc(&args[2], &shader->deformv_wavefunc);
    }
}

static shaderkey_t shaderkeys[] =
{
    {"cull", 1, 1, shader_cull},
    {"surfaceparm", 1, 1, shader_surfaceparm},
    {"skyparms", 3, 3, shader_skyparms},
    {"nomipmaps", 0, 0, shader_nomipmaps},
    {"deformvertexes", 1, 9, shader_deformvertexes},
    {NULL, 0, 0, NULL}  /* Sentinel */
};

/****************** shader pass keyword functions *******************/

static void
shaderpass_map(shader_t *shader, shaderpass_t *pass, int numargs, char **args)
{
    if (!strcmp(args[0], "$lightmap"))
	pass->flags |= SHADER_LIGHTMAP;
    else
    {
	pass->texref = shader_gettexref(args[0]);
	if (shader->flags & SHADER_NOMIPMAPS)
	    g->r_texfiles[pass->texref].flags |= TEXFILE_NOMIPMAPS;
    }
}

static void
shaderpass_rgbgen(shader_t *shader, shaderpass_t *pass, int numargs,
		  char **args)
{
    if (!strcmp(args[0], "identity"))
	return; /* Default */
    else if (!strcmp(args[0], "wave"))
    {
	if (numargs != 6)
	    Syntax();

	pass->rgbgen = SHADER_GEN_WAVE;
	shader_parsefunc(&args[1], &pass->rgbgen_func);
    }
    else if (!strcmp(args[0], "vertex") || !strcmp(args[0], "lightingdiffuse"))
    {
	shader->flags |= SHADER_NEEDCOLOURS;
	pass->rgbgen = SHADER_GEN_VERTEX;
    }
}

static void
shaderpass_blendfunc(shader_t *shader, shaderpass_t *pass, int numargs,
		     char **args)
{
    pass->flags |= SHADER_BLEND;
    
    if (numargs == 1)
    {
	if (!strcmp(args[0], "blend"))
	{
	    pass->blendsrc = GL_SRC_ALPHA;
	    pass->blenddst = GL_ONE_MINUS_SRC_ALPHA;
	}
	else if (!strcmp(args[0], "filter"))
	{
	    pass->blendsrc = GL_DST_COLOR;
	    pass->blenddst = GL_ZERO;
	}
	else if (!strcmp(args[0], "add"))
	{
	    pass->blendsrc = pass->blenddst = GL_ONE;
	}
	else
	    Syntax();
    }
    else
    {
	int i;
	uint_t *blend;
	for (i=0; i < 2; ++i)
	{
	    blend = i == 0 ? &pass->blendsrc : &pass->blenddst;
	    if (!strcmp(args[i], "gl_zero"))
		*blend = GL_ZERO;
	    else if (!strcmp(args[i], "gl_one"))
		*blend = GL_ONE;
	    else if (!strcmp(args[i], "gl_dst_color"))
		*blend = GL_DST_COLOR;
	    else if (!strcmp(args[i], "gl_one_minus_src_alpha"))
		*blend = GL_ONE_MINUS_SRC_ALPHA;
	    else if (!strcmp(args[i], "gl_src_alpha"))
		*blend = GL_SRC_ALPHA;
	    else if (!strcmp(args[i], "gl_src_color"))
		*blend = GL_SRC_COLOR;
	    else if (!strcmp(args[i], "gl_one_minus_dst_color"))
		*blend = GL_ONE_MINUS_DST_COLOR;
	    else if (!strcmp(args[i], "gl_one_minus_src_color"))
		*blend = GL_ONE_MINUS_SRC_COLOR;
	    else if (!strcmp(args[i], "gl_dst_alpha"))
		*blend = GL_DST_ALPHA;
	    else if (!strcmp(args[i], "gl_one_minus_dst_alpha"))
		*blend = GL_ONE_MINUS_DST_ALPHA;
	    else
		Syntax();
	}
    }
}

static void
shaderpass_depthfunc(shader_t *shader, shaderpass_t *pass, int numargs,
		     char **args)
{
    if (!strcmp(args[0], "equal"))
	pass->depthfunc = GL_EQUAL;
    else
	Syntax();
}

static void
shaderpass_depthwrite(shader_t *shader, shaderpass_t *pass, int numargs,
		      char **args)
{
    /* FIXME: Why oh why is depthwrite enabled in the sky shaders ???? */
    if (shader->flags & SHADER_SKY) return;
    
    shader->flags |= SHADER_DEPTHWRITE;
    pass->flags |= SHADER_DEPTHWRITE;
}

static void
shaderpass_alphafunc(shader_t *shader, shaderpass_t *pass, int numargs,
		     char **args)
{
    pass->flags |= SHADER_ALPHAFUNC;
    
    if (!strcmp(args[0], "gt0"))
    {
	pass->alphafunc = GL_GREATER;
	pass->alphafuncref = 0.0f;
    }
    else if (!strcmp(args[0], "ge128"))
    {
	pass->alphafunc = GL_GEQUAL;
	pass->alphafuncref = 0.5f;
    }
    else
	Syntax();
}

static void
shaderpass_tcmod(shader_t *shader, shaderpass_t *pass, int numargs,
		 char **args)
{
    pass->flags |= SHADER_TCMOD;
    
    if (!strcmp(args[0], "scale"))
    {
	if (numargs != 3) Syntax();
	pass->tcmod |= SHADER_TCMOD_SCALE;
	pass->tcmod_scale[0] = atof(args[1]);
	pass->tcmod_scale[1] = atof(args[2]);
    }
    else if (!strcmp(args[0], "rotate"))
    {
	pass->tcmod |= SHADER_TCMOD_ROTATE;
	pass->tcmod_rotate = atof(args[1]);
    }
    else if (!strcmp(args[0], "scroll"))
    {
	if (numargs != 3) Syntax();
	pass->tcmod |= SHADER_TCMOD_SCROLL;
	pass->tcmod_scroll[0] = atof(args[1]);
	pass->tcmod_scroll[1] = atof(args[2]);
    }
    else if (!strcmp(args[0], "stretch"))
    {
	if (numargs != 6) Syntax();
	pass->tcmod |= SHADER_TCMOD_STRETCH;
	shader_parsefunc(&args[1], &pass->tcmod_stretch);
    }
    else if (!strcmp(args[0], "transform"))
    {
	int i;
	if (numargs != 7) Syntax();
	pass->tcmod |= SHADER_TCMOD_TRANSFORM;
	for (i=0; i < 6; ++i)
	    pass->tcmod_transform[i] = atof(args[i+1]);
    }
    else if (!strcmp(args[0], "turb"))
    {
	int i, a1;
	if (numargs == 5)
	    a1 = 1;
	else if (numargs == 6)
	    a1 = 2;
	else
	    Syntax();
	pass->tcmod |= SHADER_TCMOD_TURB;
	for (i=0; i < 4; ++i)
	    pass->tcmod_turb[i] = atof(args[i+a1]);
    }
    else
	Syntax();
}

static void
shaderpass_animmap(shader_t *shader, shaderpass_t *pass, int numargs,
		   char **args)
{
    int i;
    pass->flags |= SHADER_ANIMMAP;
    pass->anim_fps = atof(args[0]);
    pass->anim_numframes = numargs - 1;
    for (i=1; i < numargs; ++i)
	pass->anim_frames[i-1] = shader_gettexref(args[i]);
}

static void
shaderpass_clampmap(shader_t *shader, shaderpass_t *pass, int numargs,
		    char **args)
{
    pass->texref = shader_gettexref(args[0]);
    g->r_texfiles[pass->texref].flags |= TEXFILE_CLAMP;
    if (shader->flags & SHADER_NOMIPMAPS)
	g->r_texfiles[pass->texref].flags |= TEXFILE_NOMIPMAPS;    
}

static void
shaderpass_tcgen(shader_t *shader, shaderpass_t *pass, int numargs,
		 char **args)
{
    if (!strcmp(args[0], "environment"))
    {
	if (numargs != 1)
	    Syntax();
	pass->flags |= SHADER_TCGEN_ENV;
        /* SG: For envmaps */
        shader->flags |= SHADER_NEEDNORMALS;
    }
}

static shaderkey_t shaderpasskeys[] =
{
    {"map", 1, 1, shaderpass_map},
    {"rgbgen", 1, 6, shaderpass_rgbgen},
    {"blendfunc", 1, 2, shaderpass_blendfunc},
    {"depthfunc", 1, 1, shaderpass_depthfunc},
    {"depthwrite", 0, 0, shaderpass_depthwrite},
    {"alphafunc", 1, 1, shaderpass_alphafunc},
    {"tcmod", 2, 7, shaderpass_tcmod},
    {"animmap", 3, SHADER_ARGS_MAX, shaderpass_animmap},
    {"clampmap", 1, 1, shaderpass_clampmap},
    {"tcgen", 1, 10, shaderpass_tcgen},
    {NULL, 0, 0, NULL}  /* Sentinel */
};

/* *************************************************************** */

void
shader_readall(void)
{
    int numfiles, len;
    char shaderlist[4096], *fname;

    /* printf("Initializing Shaders\n"); */

    g->r_numtextures = 0;
    g->r_shaders = (shader_t*) gc_malloc(g->r_numshaders * sizeof(shader_t));
    g->r_texfiles = (texfile_t*) gc_malloc(MAX_NUM_TEXTURES * sizeof(texfile_t));
    shaderfound = (int*) gc_malloc(g->r_numshaders * sizeof(int));    
    shaderbuf = (char*) gc_malloc(SHADERBUF_SIZE);
    memset(shaderfound, 0, g->r_numshaders * sizeof(int));

    /* Get a list of shader script files */
    numfiles = pak_listshaders(4096, shaderlist);

    /* Parse each file */
    fname = shaderlist;
    while (numfiles--)
    {
      len = pak_readfile(fname, SHADERBUF_SIZE, (byte_t *) shaderbuf);
      curpos = shaderbuf;
      endpos = curpos + len;
//      printf("...loading '%s...'", fname);
      shader_read();
//      printf("done.\n");
      fname += strlen(fname) + 1;
    }
    gc_free(shaderbuf);

    /* Make default shaders for those that weren't found */
//    printf("Making defaults for shaders not found...\n");
    shader_makedefaults();
//    printf("Freeing...\n");
    gc_free(shaderfound);
   
//    printf("done.\n");
}

void
shader_freeall(void)
{
    int i;

    for (i=0; i < g->r_numtextures; i++)
	gc_free(g->r_texfiles[i].fname);
    gc_free(g->r_texfiles);
    gc_free(g->r_shaders);
}

static int
shader_lookup(const char *name)
{
    int i, id = -1;

    /* FIXME: This should be done with a hash table! */

    for (i=0; i < g->r_numshaders; ++i)
    {
	if (!strcmp(name, g->r_shaderrefs[i].name))
	{
	    id = i;
	    break;
	}
    }
    return id;
}

static void
shader_read(void)
{
    int id;
    char *tok;

    while ((tok = nexttok()) != NULL)
    {
	id = shader_lookup(tok);
	if (id < 0)
	{
	    shader_skip();
	    continue;
	}

	/* printf("shader_read %d\n", id); */

	/* Mark shaderref as 'found' */
	shaderfound[id] = 1;
	
	/* Set defaults */
	g->r_shaders[id].flags = 0;
	g->r_shaders[id].numpasses = 0;
	
	/* Opening brace */
	tok = nexttok();
	if (tok[0] != '{') Syntax();

	while ((tok = nexttok()) != NULL)
	{
	    if (tok[0] == '{') /* Start new pass */
	    {
		int pass = g->r_shaders[id].numpasses++;
		shader_readpass(&(g->r_shaders[id]), &(g->r_shaders[id].pass[pass]));
	    }

	    else if (tok[0] == '}') /* End of shader */
		break;

	    else
		shader_parsetok(&(g->r_shaders[id]), NULL, shaderkeys, tok);
	}

	/* Explicit depth write for first pass */
	/* FIXME: is this how we handle transparent ? */
	if (! (g->r_shaders[id].flags & SHADER_DEPTHWRITE) &&
	    ! (g->r_shaders[id].flags & SHADER_TRANSPARENT) &&
	    ! (g->r_shaders[id].flags & SHADER_SKY) &&
	    g->r_shaders[id].numpasses > 0)
	{
	    g->r_shaders[id].pass[0].flags |= SHADER_DEPTHWRITE;
	}
    }
}

static void
shader_skip(void)
{
    char *tok;
    int brace_count;

    /* Opening brace */
    tok = nexttok();
    if (tok[0] != '{') Syntax();

    for (brace_count = 1; brace_count > 0 && curpos < endpos; curpos++)
    {
	if (*curpos == '{')
	    brace_count++;
	else if (*curpos == '}')
	    brace_count--;
    }
}

static void
shader_readpass(shader_t *shader, shaderpass_t *pass)
{
    char *tok;

    /* Set defaults */
    pass->flags = 0;
    pass->texref = -1;
    pass->depthfunc = GL_LEQUAL;
    pass->rgbgen = SHADER_GEN_IDENTITY;
    pass->tcmod = 0;
    
    while ((tok = nexttok()) != NULL)
    {
	if (tok[0] == '}') /* End of pass */
	    break;

	else
	    shader_parsetok(shader, pass, shaderpasskeys, tok);
    }
}

static void
shader_parsetok(shader_t *shader, shaderpass_t *pass, shaderkey_t *keys,
		char *tok)
{
    shaderkey_t *key;
    char *c, *args[SHADER_ARGS_MAX];
    int numargs;

    /* Lowercase the token */
    c = tok;
    while (*c++) *c =  LOWERCASE(*c);
    
    /* FIXME: This should be done with a hash table! */

    for (key = keys; key->keyword != NULL; key++)
    {
	if (strcmp(tok, key->keyword) == 0)
	{
	    for (numargs=0; (c = nextarg()) != NULL; numargs++)
	    {
		/* Lowercase the argument */
		args[numargs] = c;
		while (*c) {*c = LOWERCASE(*c); c++;}
	    }
	    if (numargs < key->minargs || numargs > key->maxargs)
		Syntax();
	    
	    if (key->func)
		key->func(shader, pass, numargs, args);
	    return;
	}
    }

    /* Unidentified keyword: no error for now, just advance to end of line */
    while (*curpos != '\n')
	if (++curpos == endpos) break;    
}

static void checkLMflag(shader_t *s) {

  int okBlend = 0;

#ifdef NO_MULTITEX
  s->lmFlag = 0;
  return;
#endif

  if (s->numpasses < 2) {
    s->lmFlag = 0;
    return;
  }

  if (s->pass[1].blendsrc == GL_DST_COLOR && s->pass[1].blenddst == GL_ZERO)
    okBlend =1;
  if (s->pass[1].blendsrc == GL_ZERO && s->pass[1].blenddst == GL_SRC_COLOR)
    okBlend =1;

  if (s->numpasses == 2 &&
	  (s->pass[0].flags & SHADER_LIGHTMAP) &&
	  (s->pass[1].flags & SHADER_BLEND) &&    
	  okBlend)
    s->lmFlag = 1;
  else
    s->lmFlag = 0;
}

static void
shader_makedefaults(void)
{
    int i, f, firsttrisurf, lasttrisurf, trisurf, md3;
    char fname[128];

    /* Find first and last trisurf */
    firsttrisurf = -1;
    for (f = 0; f < g->r_numfaces; f++)
    {
	if (g->r_faces[f].facetype == FACETYPE_TRISURF)
	{
	    firsttrisurf = f;
	    break;
	}
    }
    if (firsttrisurf >= 0)
    {
      lasttrisurf = g->r_numfaces-1;
	for (f = firsttrisurf; f < g->r_numfaces; f++)
	{
	    if (g->r_faces[f].facetype != FACETYPE_TRISURF)
	    {
		lasttrisurf = f-1;
		//lasttrisurf = f;
		break;
	    }
	}
    }

    for (i=0; i < g->r_numshaders; ++i)
    {
	if (shaderfound[i]) {
//printf("Calling checkLMflag( %ud )...\n", &(g->r_shaders[i]));
		checkLMflag(&(g->r_shaders[i]));     
		continue;
	}

	/* Special exception: noshader */
	if (!strcmp(g->r_shaderrefs[i].name, "noshader"))
	{
	    g->r_shaders[i].numpasses = 0;
	    continue;
	}
	
	/* Append tga to get file name */
	strcpy(fname, g->r_shaderrefs[i].name);
	strcat(fname, ".tga");

//printf("fname: %s\n",fname);

	/* Check if shader is for an md3 */
	md3 = 0;
	trisurf = 0;
	if (i >= g->r_addshaderstart)
	{
	    md3 = 1;
	}
	else
	{
	    /* Check if shader is for a trisurf */
	    if (firsttrisurf >= 0)
	    {
		/*for (f=firsttrisurf; f <= lasttrisurf; f++)*/
		for (f=firsttrisurf; f <= lasttrisurf; f++)
		{
		    if (g->r_faces[f].shader == i)
		    {
			trisurf = 1;
			break;
		    }
		}
	    }
	}
	if (md3)
	{
	    g->r_shaders[i].flags = SHADER_NOCULL;
	    g->r_shaders[i].numpasses = 1;
	    g->r_shaders[i].pass[0].flags = SHADER_DEPTHWRITE;
	    g->r_shaders[i].pass[0].texref = shader_gettexref(fname);
	    g->r_shaders[i].pass[0].depthfunc = GL_LEQUAL;
	    g->r_shaders[i].pass[0].rgbgen = SHADER_GEN_IDENTITY;
		g->r_shaders[i].lmFlag = 0;
	}
	else if (trisurf)
	{
	    g->r_shaders[i].flags = SHADER_NOCULL | SHADER_NEEDCOLOURS;
	    g->r_shaders[i].numpasses = 1;
	    g->r_shaders[i].pass[0].flags = SHADER_DEPTHWRITE;
	    g->r_shaders[i].pass[0].texref = shader_gettexref(fname);
	    g->r_shaders[i].pass[0].depthfunc = GL_LEQUAL;
	    g->r_shaders[i].pass[0].rgbgen = SHADER_GEN_VERTEX;
		g->r_shaders[i].lmFlag = 0;
	}
	else
	{
	    g->r_shaders[i].flags = 0;
	    g->r_shaders[i].numpasses = 2;
	    g->r_shaders[i].pass[0].flags = SHADER_LIGHTMAP | SHADER_DEPTHWRITE;
	    g->r_shaders[i].pass[0].texref = -1;
	    g->r_shaders[i].pass[0].depthfunc = GL_LEQUAL;
	    g->r_shaders[i].pass[0].rgbgen = SHADER_GEN_IDENTITY;
	    
	    g->r_shaders[i].pass[1].flags = SHADER_BLEND;
	    g->r_shaders[i].pass[1].texref = shader_gettexref(fname);
	    g->r_shaders[i].pass[1].blendsrc = GL_DST_COLOR;
	    g->r_shaders[i].pass[1].blenddst = GL_ZERO;
	    g->r_shaders[i].pass[1].depthfunc = GL_LEQUAL;
	    g->r_shaders[i].pass[1].rgbgen = SHADER_GEN_IDENTITY;
#ifdef NO_MULTITEX
 	      g->r_shaders[i].lmFlag = 0;
#else
 	      g->r_shaders[i].lmFlag = 1;
#endif
	}
    }
}

/* expose to outside */
int
shader_gettexref(const char *fname)
{
    int i;
    
    /* FIXME: hash table again! */
    for (i=0; i < g->r_numtextures; ++i)
    {
	if (!strcmp(fname, g->r_texfiles[i].fname))
	    return i;
    }
    
    if (g->r_numtextures == MAX_NUM_TEXTURES)
	Error("Texture count exceeded");
    g->r_texfiles[g->r_numtextures].flags = 0;
    g->r_texfiles[g->r_numtextures].fname = strdup(fname);
    return g->r_numtextures++;
}

static char*
nexttok(void)
{
    char *tok;
    
    while (curpos < endpos)
    {
	/* Skip leading whitespace */
	while (*curpos == ' ' || *curpos == '\t' || *curpos == '\n' ||
	      *curpos == '\r')
	    if (++curpos == endpos) return NULL;

	/* Check for comment */
	if (curpos[0] == '/' && curpos[1] == '/')
	{
	    /* Skip to end of comment line */
	    while (*curpos++ != '\n')
		if (curpos == endpos) return NULL;
	    /* Restart with leading whitespace */
	    continue;
	}

	/* Seek to end of token */
	tok = curpos;
	while (*curpos != ' ' && *curpos != '\t' && *curpos != '\n' &&
	      *curpos != '\r')
	    if (++curpos == endpos) break;

	/* Zero whitespace character and advance by one */
	*curpos++ = '\0';
	return tok;
    }
    return NULL;
}

static char *
nextarg(void)
{
    char *arg;

    while (curpos < endpos)
    {
	/* Skip leading whitespace */
	while (*curpos == ' ' || *curpos == '\t')
	    if (++curpos == endpos) return NULL;

	/* Check for newline or comment */
	if (*curpos == '\n' || *curpos == '\r' ||
	    (curpos[0] == '/' && curpos[1] == '/'))
	    return NULL;
	
	/* Seek to end of token */
	arg = curpos;
	while (*curpos != ' ' && *curpos != '\t' && *curpos != '\n' &&
	      *curpos != '\r')
	    if (++curpos == endpos) break;

	/* Zero whitespace character and advance by one */
	*curpos++ = '\0';
	return arg;
    }
    return NULL;
}


void
Syntax(void)
{
    /* Error("Syntax error\n"); */
    fprintf(stderr, "syntax error...");
}

static void
shader_parsefunc(char **args, shaderfunc_t *func)
{
	if (!strcmp(args[0], "sin"))
	    func->func = SHADER_FUNC_SIN;
	else if (!strcmp(args[0], "triangle"))
	    func->func = SHADER_FUNC_TRIANGLE;
	else if (!strcmp(args[0], "square"))
	    func->func = SHADER_FUNC_SQUARE;
	else if (!strcmp(args[0], "sawtooth"))
	    func->func = SHADER_FUNC_SAWTOOTH;
	else if (!strcmp(args[0], "inversesawtooth"))
	    func->func = SHADER_FUNC_INVERSESAWTOOTH;
	else
	    Syntax();

	func->args[0] = atof(args[1]);
	func->args[1] = atof(args[2]);
	func->args[2] = atof(args[3]);
	func->args[3] = atof(args[4]);
}

