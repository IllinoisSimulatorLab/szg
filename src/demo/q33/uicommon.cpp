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

/* Extended keyboard navigation to enable keyboard look.
 * Everything in the previous interface was left intact.
 * The rotation motion has a small inertia added to it, much
 * like the forward/backward thrust, but more responsive.
 * Keyboard commands added:
 * 4 - rotate view left
 * 6 - rotate view right
 * 2 - tilt view down
 * 8 - tilt view up
 * 5 - center view vertically
 * (These make perfect sense if you use the numeric keypad.)
 *
 * Also, find_start_pos() now cycles through the available
 * starting positions for maps with multiple spawn points.
 * Keyboard command added:
 * x - warp to the next spawn point
 *
 * Stefan Gustavson (stegu@itn.liu.se) 2001-06-26
 */
// precompiled header include MUST appear as the first non-comment line
#include "arPrecompiled.h"
#include "util.h"
#include "pak.h"
#include "bsp.h"
#include "render.h"
#include "tex.h"
#include "lightmap.h"
#include "shader.h"
#include "mesh.h"
#include "skybox.h"
#include "entity.h"
#include "mapent.h"
#include "uicommon.h"
#include "renderhud.h"
#include "glinc.h"
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include "glext.h"
#endif

#ifdef DARWIN
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#endif

#include "globalshared.h"
extern global_shared_t *g;
extern int global_start_type;

#define AZ_SCALE 0.1
#define EL_SCALE 0.1

/* SG - keyboard look start */
#define AZ_THRUST 240.0 
#define EL_THRUST 240.0
#define AZ_BRAKE 360.0
#define EL_BRAKE 360.0
#define AZ_MAXSPEED 120.0
#define EL_MAXSPEED 120.0 /* max speed is 120 degrees per second */

#define FSIGN(x) ((x > 0.001) ? 1.0 : ((x < -0.001) ? -1.0 : 0.0))
#define FZERO(x) ((x < 0.001) && (x > -0.001))
/* SG - keyboard look end */

static int ox, oy;
static double fpstime;
static int fpsframes;
static int invert_mouse = 0;

static int lastspawnpoint = 0; /* SG - For spawn point cycling */
static double az_speed=0.0, el_speed=0.0; /* SG - For smooth keyboard look */
static double az_accel=0.0, el_accel=0.0;

static void
find_start_pos(void)
{
    /* Find the next spawn point in the entities list */
    int i,ii;
    const char *cname;
    
    g->r_eye_el = g->r_eye_az = 0.0;
    g->r_eyepos[0] = g->r_eyepos[1] = g->r_eyepos[2] = 0.0;
    for (i = 0; i < g->g_numentities; i++)
    {
        ii = (lastspawnpoint+i+1) % g->g_numentities; /* SG - wrap */
	cname = entity_value(ii, "classname");
  if (((global_start_type == START_TYPE_DEATHMATCH)&&(!strcmp(cname, "info_player_deathmatch"))) ||
    ((global_start_type == START_TYPE_START)&&(!strcmp(cname, "info_player_start")))) // JAC 10/15/03
	{
	  lastspawnpoint = ii;
	  g->r_eye_az = entity_float(ii, "angle");
//printf("Initial direction == (%f,%f)\n",g->r_eye_az,g->r_eye_el);
	  entity_vec3(ii, "origin", g->r_eyepos);
	  break;
	}
    }
    vec_copy(g->r_eyepos, g->r_eyepos_sav)
}

void ui_reset()
{
  find_start_pos();
  g->r_title = 1;
}

void updateVelocity(float *vel, int mov, double etime, float accel, float max) {
    if (etime > 0.1) etime = 0.1; /* cap */

    *vel += mov * accel * etime;

    if (*vel > max)  *vel = max;
    if (*vel < -max) *vel = -max;

    /* slow down */
    if (*vel > 0.0) {
      *vel -= max/2 * etime;
      if (*vel < 0.0) *vel = 0.0;
    }
    else if (*vel < 0.0) {
      *vel += max/2 * etime;
      if (*vel > 0.0) *vel = 0.0;
    }
}

// Returns true iff a collision occured.
bool
move_eye(double etime)
{
    vec3_t delta;
    vec3_t aheadPos, aheadDelta;
    int aheadCluster;
    float aheadD;

    /* SG - keyboard look start */
    double newspeed;

    if (g->g_keylook) {
      if(!FZERO(az_accel)) {
        az_speed += az_accel*etime; /* Accelerate rotation */
        if(az_speed > AZ_MAXSPEED) az_speed = AZ_MAXSPEED;
        if(az_speed < -AZ_MAXSPEED) az_speed = -AZ_MAXSPEED;
      } else {
        newspeed = az_speed - FSIGN(az_speed) * AZ_BRAKE*etime; /* Settle down*/
        if(FSIGN(newspeed) != FSIGN(az_speed)) az_speed = 0.0;
        else az_speed = newspeed;
      }

      if(!FZERO(el_accel)) {
        el_speed += el_accel*etime; /* Accelerate rotation */
        if(el_speed > EL_MAXSPEED) el_speed = EL_MAXSPEED;
        if(el_speed < -EL_MAXSPEED) el_speed = -EL_MAXSPEED;
      } else {
        newspeed = el_speed - FSIGN(el_speed) * EL_BRAKE*etime; /* Settle down*/
        if(FSIGN(newspeed) != FSIGN(el_speed)) el_speed = 0.0;
        else el_speed = newspeed;
      }

      g->r_eye_az += az_speed*etime;
      g->r_eye_el += el_speed*etime;
      if(g->r_eye_el < -90.0) g->r_eye_el = -90.0;
      if(g->r_eye_el > 90.0) g->r_eye_el = 90.0;
      vec_point(g->r_movedir, g->r_eye_az, g->r_eye_el);
    }
    /* SG - keyboard look end */

    updateVelocity(&(g->g_velocity), g->g_move, etime, g->g_acceleration, g->g_maxvel);
//    if (g->r_setup_projection)
//      updateVelocity(&(g->g_velocity), g->g_move, etime, 1000.0, 400.0);
//    else
//      updateVelocity(&(g->g_velocity), g->g_move, etime, 200.0, 200.0);

    if (g->g_velocity > 0)
      aheadD = g->g_velocity * etime + g->g_collision_buffer;
    else
      aheadD = g->g_velocity * etime - g->g_collision_buffer;

    vec_copy(g->r_eyepos, aheadPos);
    vec_copy(g->r_movedir, aheadDelta);
    vec_scale(aheadDelta, aheadD, aheadDelta);
    vec_add(aheadPos, aheadDelta, aheadPos);

    aheadCluster = find_cluster(aheadPos);
    if (g->g_collision && aheadCluster < 0) {
      g->g_velocity *= -0.8;
      return true;
    }

    vec_copy(g->r_movedir, delta);
    vec_scale(delta, g->g_velocity * etime, delta);
    vec_add(g->r_eyepos, delta, g->r_eyepos);
    return false;
}

static float
calc_fov(float fov_x, float width, float height)
{
    /* Borrowed from the Quake 1 source distribution */
    float   a;
    float   x;
    
    if (fov_x < 1 || fov_x > 179)
	Error("Bad fov: %f", fov_x);
    
    x = width/tan(fov_x/360.0*M_PI);
    a = atan(height/x);
    a = a*360.0/M_PI;
    
    return a;
}

/* reads the command line arguments */
void
ui_read_args(int argc, char **argv)
{
	int i;
	int bsp_specified=FALSE;
	char *arg;

	for(i=1; i < argc; i++) {
		arg=argv[i];
		if (arg[0] == '-')
		{
		/* this is a switch argument */
		if(!strcmp(arg,"-f") || !strcmp(arg,"--fullscreen"))
			g->r_fullscreen=TRUE;
		else if(!strcmp(arg,"--notex"))
			g->r_notextures=TRUE;
		else if(!strcmp(arg,"--noitems"))
			g->r_drawitems=FALSE;
		else if(!strcmp(arg,"--nohud"))
			g->r_drawhud=FALSE;
		else if(!strcmp(arg,"--gamma"))
			g->r_gamma = atof(argv[++i]);
		else if(!strcmp(arg,"--width"))
			g->r_viewport_w = atoi(argv[++i]);
		else if(!strcmp(arg,"--height"))
			g->r_viewport_h = atoi(argv[++i]);
		else if(!strcmp(arg,"--fov"))
			g->r_eyefov = atof(argv[++i]);
		else if(!strcmp(arg,"--drawsky"))
			g->r_sky = TRUE;
		else if(!strcmp(arg,"--lodbias"))
			g->r_lodbias = atoi(argv[++i]);
		else if(!strcmp(arg,"--maxmeshlevel"))
			g->r_maxmeshlevel = atoi(argv[++i]);
		else if(!strcmp(arg,"--subdivisiontol"))
			g->r_subdivisiontol = atoi(argv[++i]);
		else if(!strcmp(arg,"--collidebuffer"))
			g->g_collision_buffer = atof(argv[++i]);
		else if(!strcmp(arg,"--keylook"))
			g->g_keylook = TRUE;
	} else {
		/* BSP or pak file specified */
		if(bsp_specified)
		{
			strcpy(g->g_pak_fnames[g->g_pak_num], arg);
			g->g_pak_num++;
		} else {
			bsp_specified=TRUE;
			strcpy(g->g_bsp_fname, arg);
		}
	  }
    }
}

void ui_init_gl(r_context_t *context)
{
    float fov_y;

    /* fprintf(stderr, " Initializing rendering context... \n"); */

    /* My GL driver (linux/TNT2) has problems with glEnableClientState(),
       but this seems to clear it up.  Go figure. */
    glFinish();
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);

    context->r_tex_units = 1; /* default - 1 texture unit */

    context->range[0] = 0.0;
    context->range[1] = 1.0;
    
#ifndef NO_MULTITEX
    glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &(context->r_tex_units));
#endif

    render_init(context);
    lightmap_bindobjs(context);
    tex_bindobjs(context);

    glDisable(GL_DITHER);
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glColor3f(1.0, 1.0, 1.0);

    glEnable(GL_DEPTH_TEST);
    glCullFace(GL_FRONT);

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    if (g->r_setup_projection) {
      float aspect = ((float) g->r_viewport_w)/((float) g->r_viewport_h);
      glViewport(0, 0, g->r_viewport_w, g->r_viewport_h);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      fov_y = calc_fov(g->r_eyefov, g->r_viewport_w, g->r_viewport_h);
      gluPerspective(fov_y, aspect, 5.0, 3000.0); /* was 10.0, 3000.0 */
      glMatrixMode(GL_MODELVIEW);
    }
}

void print_info()
{
  printf("\n");
  printf("    C A V E   Q U A K E   I I I   A R E N A \n");
  printf("    --------------------------------------- \n");
  printf("               by Paul Rajlich \n");
  printf("\n");
  printf("               paul@visbox.com \n");
  printf("         http://www.visbox.com/cq3a/ \n");
  printf("\n");
  printf(" Based on the Aftershock engine by Steve Taylor.\n");
  printf("   Quake3 Arena is a trademark of Id Software.\n");
  printf("\n");
}

void ui_init_bsp()
{
    int i;
    fpstime = 0;
    fpsframes = 0;

    print_info();

    fprintf(stderr, " Loading bsp...");
    /* load paks */
    for (i=0; i < g->g_pak_num; i++)
      pak_openpak(g->g_pak_fnames[i]);
    bsp_read(g->g_bsp_fname);
    fprintf(stderr, " done\n");

    fprintf(stderr, " Creating meshes...");
    mesh_create_all();
    fprintf(stderr, " done\n");

    skybox_create();
 
    fprintf(stderr, " Loading entities...");
    mapent_loadall();
    render_init_mapents();
    fprintf(stderr, " done\n");

    fprintf(stderr, " Parsing shaders...");
    shader_readall();
    fprintf(stderr, " done\n");

    hud_init();

    fprintf(stderr, " Loading lightmaps...");
    lightmap_loadall();
    fprintf(stderr, " done\n");

    fprintf(stderr, "\n");
    fprintf(stderr, " Loading %d textures\n\n", g->r_numtextures);
    tex_loadall();
    fprintf(stderr, "\n");

    find_start_pos();
    vec_point(g->r_movedir, g->r_eye_az, g->r_eye_el);
}

static void print_stats() {
  fprintf(stderr, " FPS: %.1f  collision: ", g->r_fps);
  if (g->g_collision)
    fprintf(stderr, "on  ");
  else
    fprintf(stderr, "off ");
  fprintf(stderr, " sky: ");
  if (g->r_sky)
    fprintf(stderr, "on  ");
  else
    fprintf(stderr, "off ");
  fprintf(stderr, "   \r");
}

void ui_sync(r_context_t *c, double time)
{
    c->r_frametime  = time;
    c->r_eyecluster = g->r_eyecluster;
    c->r_eyepos[0]  = g->r_eyepos[0];
    c->r_eyepos[1]  = g->r_eyepos[1];
    c->r_eyepos[2]  = g->r_eyepos[2];
    c->r_eye_az     = g->r_eye_az;
    c->r_eye_el     = g->r_eye_el;
}

// Returns true iff a collision occured.
bool
ui_move(double time)
{
    static double lastTime = 0.0f;
    double etime = time - lastTime;

    /* cap */
    if (etime <= 0.0) etime = 0.0;
    if (etime > 0.1) etime = 0.1;

    /* move! */
    bool fCollided = move_eye(etime);

    if (!(g->r_lockpvs))
      g->r_eyecluster = find_cluster(g->r_eyepos);

    if (g->g_collision) {
      if (g->r_eyecluster >= 0) {
        vec_copy(g->r_eyepos, g->r_eyepos_sav)
      }
      else {
        vec_copy(g->r_eyepos_sav, g->r_eyepos)
        g->r_eyecluster = find_cluster(g->r_eyepos);
      }
    }

    /* FPS counter */
    fpsframes++;
    if (time - fpstime > 2.5)
    {
        if (g->r_calc_fps)	    	
          g->r_fps = fpsframes / (time - fpstime);
	print_stats();
        /* fprintf(stderr, " FPS: %.1f  \r", g->r_fps); */
        fpstime = time;
        fpsframes = 0;
    }

    lastTime = time;
    return fCollided;
}

void
ui_display(r_context_t *c, double time)
{
    ui_sync(c, time);
    render_clear();
    render_draw(c);
    glFlush();
}

//**********************************************************************
// adding this function is desirable for Syzygy operation
//**********************************************************************

void ui_display_nosync(r_context_t* c, double /*time*/){
  render_clear();
  //printf("YAARGH!\n");
  render_draw(c);
  //printf("YAARGH2!\n");
  glFlush();
}

void
ui_mouse_down(int x, int y)
{
    ox = x; oy = y;
}

void
ui_mouse_motion(int x, int y)
{
    g->r_eye_az += -(x - ox) * AZ_SCALE;
    g->r_eye_el += -(y - oy) * EL_SCALE * (invert_mouse ? -1.0f : 1.0f);

    if (g->r_eye_el >  90.0) g->r_eye_el =  90.0;
    if (g->r_eye_el < -90.0) g->r_eye_el = -90.0;

    vec_point(g->r_movedir, g->r_eye_az, g->r_eye_el);
    
    ox = x; oy = y;    
}

void
ui_mouse_relmotion(int relx, int rely)
{
    g->r_eye_az += -relx * AZ_SCALE;
    g->r_eye_el += -rely * EL_SCALE * (invert_mouse ? -1.0f : 1.0f);

    if (g->r_eye_el >  90.0) g->r_eye_el =  90.0;
    if (g->r_eye_el < -90.0) g->r_eye_el = -90.0;

    vec_point(g->r_movedir, g->r_eye_az, g->r_eye_el);
}
    


void
ui_key_down(unsigned char key)
{
    switch (key)
      {
      /* SG - keyboard look start */
      case '4':
	az_accel = AZ_THRUST;
	//ui_mouse_relmotion(-20,0);
	break;
      case '6':
	az_accel = -AZ_THRUST;
	//ui_mouse_relmotion(20,0);
	break;
      case '2':
	el_accel = -EL_THRUST;
	//ui_mouse_relmotion(0,-20);
	break;
      case '8':
	el_accel = EL_THRUST;
	//ui_mouse_relmotion(0,20);
	break;
      case '5':
	el_accel = el_speed = g->r_eye_el = 0.0;
	break;
      /* SG - keyboard look end */

	case 'w':
	    if (g->r_title)
              g->r_title = 0;
	    g->g_move = 1;
	    break;
	case 's':
	    g->g_move = -1;
	    break;
	case 'h':
	    g->r_title = 1;
	    break;	
	case 'b':
	    if (g->r_sky)
	      g->r_sky = 0;
	    else
	      g->r_sky = 1;
	    break;
	case 'c':
	    if (g->g_collision)
	      g->g_collision = 0;
	    else	    
	      g->g_collision = 1;
	    break;	
        case 'p':
         printf("\rpos: %.1f %.1f %.1f az: %.1f el: %.1f dir: %.3f %.3f %.3f\n",
			    g->r_eyepos[0], g->r_eyepos[1], g->r_eyepos[2],
			    g->r_eye_az, g->r_eye_el,
			    g->r_movedir[0], g->r_movedir[1], g->r_movedir[2]);
	    break;
		/***
	case 'l':
	    g->r_lockpvs = !(g->r_lockpvs);
	    if (g->r_lockpvs) printf("cluster: %d\n", g->r_eyecluster);
	    break;
	case 'i':
	    invert_mouse = !invert_mouse;
	    printf("invert_mouse = %d\n", invert_mouse);
	    break;
		***/
	case 'q':
		exit(0);
		break;
	case 'x':
		find_start_pos();
		break;
    }
}

void
ui_key_up(unsigned char key)
{
    switch (key)
    {
    /* SG - keyboard look start */
      case '4':
	az_accel = 0.0;
	break;
      case '6':
	az_accel = 0.0;
	break;
      case '2':
	el_accel = 0.0;
	break;
      case '8':
	el_accel = 0.0;
	break;
    /* SG - keyboard look end */

	case 'w':
	case 's':
	    g->g_move = 0;
	    break;
    }
}

void
ui_finish(r_context_t *c)
{
    shader_freeall();
    mesh_free_all();
    skybox_free();
    mapent_freeall();
    lightmap_freeobjs(c);
    tex_freeobjs(c);
    render_finalize(c);
    pak_closepak();
    fprintf(stderr, "\n\n");
}
