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
#ifndef __SHADER_H__
#define __SHADER_H__

#define SHADERPASS_MAX 5
#define SHADER_ANIM_FRAMES_MAX 10

/* Shader flags */
enum
{
    SHADER_NOCULL        = 1 << 0,
    SHADER_TRANSPARENT   = 1 << 1,
    SHADER_DEPTHWRITE    = 1 << 2,  /* Also used for pass flag */
    SHADER_SKY           = 1 << 3,
    SHADER_NOMIPMAPS     = 1 << 4,
    SHADER_NEEDCOLOURS   = 1 << 5,
    SHADER_DEFORMVERTS   = 1 << 6,
    SHADER_NEEDNORMALS   = 1 << 7  /* SG - for envmaps */
};

/* Shaderpass flags */
enum
{
    SHADER_LIGHTMAP   = 1 << 0,
    SHADER_BLEND      = 1 << 1,
    SHADER_ALPHAFUNC  = 1 << 3,
    SHADER_TCMOD      = 1 << 4,
    SHADER_ANIMMAP    = 1 << 5,
    SHADER_TCGEN_ENV  = 1 << 6
};	

/* Transform functions */
enum
{
    SHADER_FUNC_SIN             = 1,
    SHADER_FUNC_TRIANGLE        = 2,
    SHADER_FUNC_SQUARE          = 3,
    SHADER_FUNC_SAWTOOTH        = 4,
    SHADER_FUNC_INVERSESAWTOOTH = 5
};

/* *Gen functions */
enum
{
    SHADER_GEN_IDENTITY = 0,
    SHADER_GEN_WAVE     = 1,
    SHADER_GEN_VERTEX   = 2
};

/* tcmod functions */
enum
{
    SHADER_TCMOD_SCALE               = 1 << 1,
    SHADER_TCMOD_ROTATE              = 1 << 2,
    SHADER_TCMOD_SCROLL              = 1 << 3,
    SHADER_TCMOD_TRANSFORM           = 1 << 4,
    SHADER_TCMOD_TURB                = 1 << 5,
    SHADER_TCMOD_STRETCH             = 1 << 6
};

/* Periodic functions */
typedef struct
{
    uint_t func;     /* SHADER_FUNC enum */
    float args[4];   /* offset, amplitude, phase_offset, rate */
} shaderfunc_t;

/* Per-pass rendering state information */
typedef struct
{
    uint_t flags;
    int texref;                 /* Texture ref (if not lightmap) */
    uint_t blendsrc, blenddst;  /* glBlend args */
    uint_t depthfunc;           /* glDepthFunc arg */
    uint_t alphafunc;           /* glAlphaFunc arg1 */
    float alphafuncref;         /* glAlphaFunc arg2 */
    uint_t rgbgen;              /* SHADER_GEN enum */
    shaderfunc_t rgbgen_func;
    uint_t tcmod;               /* SHADER_TCMOD enum */
    float tcmod_scale[2];       /* TCMOD args */
    float tcmod_rotate;
    float tcmod_scroll[2];
    float tcmod_transform[6];
    float tcmod_turb[4];
    shaderfunc_t tcmod_stretch;
    float anim_fps;             /* Animation frames per sec */
    int anim_numframes;
    int anim_frames[SHADER_ANIM_FRAMES_MAX];  /* Texture refs */
} shaderpass_t;

/* Shader info */
typedef struct
{
    uint_t flags;
    int numpasses;
    shaderpass_t pass[SHADERPASS_MAX];
    float skyheight;          /* Height for skybox */
    float deformv_wavesize;   /* DeformVertexes wavelength in world units */
    shaderfunc_t deformv_wavefunc;
    int lmFlag; 
} shader_t;

/* Special texture loading requirements */
enum
{
    TEXFILE_NOMIPMAPS  = 1 << 0,
    TEXFILE_CLAMP      = 1 << 1,
    TEXFILE_FULL_LOD    = 1 << 2
};

/* Gathers texture file names prior to texture loading */
typedef struct
{
    uint_t flags;
    char *fname;
} texfile_t;

#ifdef __cplusplus
extern "C" {
#endif

void shader_readall(void);
void shader_freeall(void);

#ifdef __cplusplus
}
#endif


int shader_gettexref(const char *fname);

#endif /*__SHADER_H__*/
