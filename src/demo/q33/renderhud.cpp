/* CAVElib interface
 * Copyright (C) 2001 Paul Rajlich
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
#include "glinc.h"
#include "globalshared.h"
#include "rendercontext.h"
#include "renderhud.h"
#include "shader.h"

extern global_shared_t *g;

static int titleref, charref, iconref, helpref=-1;

static void render_title(r_context_t *c);
static void render_fps(r_context_t *c);
static void render_digit(r_context_t *c, int d, float x1, float y1, float size);
static void render_icon(r_context_t *c, float x, float y, float s);
static void begin2D(int width, int height);
static void end2D(void);

void hud_init() {

  titleref = shader_gettexref("paul/title.jpg");
  charref = shader_gettexref("gfx/2d/bigchars.tga");
  iconref = shader_gettexref("gfx/2d/defer.tga");
  helpref = shader_gettexref(g->r_help_fname);

  /* ignore r_lodbias flag (do not scale these textures) */
  g->r_texfiles[titleref].flags |= TEXFILE_FULL_LOD;
  g->r_texfiles[charref].flags |= TEXFILE_FULL_LOD;
  g->r_texfiles[iconref].flags |= TEXFILE_FULL_LOD;
  g->r_texfiles[helpref].flags |= TEXFILE_FULL_LOD;
}

void hud_render(r_context_t *c) {

  if (!g->r_drawhud)
    return;

  begin2D(100,100);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_ALPHA_TEST);
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);

  render_fps(c);
  if (!g->g_collision)
    render_icon(c, 7.0, 1.0, 2.0f);
  if (g->r_title)
    render_title(c);

  end2D();
  glEnable(GL_DEPTH_TEST);
}

void render_title(r_context_t *c) {
  glBindTexture(GL_TEXTURE_2D, c->r_textures[titleref]);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(10.0f, 50.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(10.0f, 90.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(90.0f, 90.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(90.0f, 50.0f, 0.0f);
  glEnd();

  if (helpref >= 0)
    glBindTexture(GL_TEXTURE_2D, c->r_textures[helpref]);
  else
    glBindTexture(GL_TEXTURE_2D, c->r_textures[charref]);

  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(10.0f, 10.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(10.0f, 50.0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(90.0f, 50.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(90.0f, 10.0f, 0.0f);
  glEnd();
}

void render_fps(r_context_t *c) {
  int fps = (int) g->r_fps;
  int digit1 = fps/100;
  int digit2 = (fps/10)%10;
  int digit3 = fps%10;

  glBindTexture(GL_TEXTURE_2D, c->r_textures[charref]);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_QUADS);

  if (digit1)
    render_digit(c, digit1, 0.0f, 1.0f, 2.0f);
  if (digit1 || digit2)
    render_digit(c, digit2, 2.0f, 1.0f, 2.0f);
  render_digit(c, digit3, 4.0f, 1.0f, 2.0f);

  glEnd();
}


static void render_digit(r_context_t *c, int d, float x, float y, float s) {
  /* coords of digit in char-map texture */
  float s1 = 0.0625f * d     + 0.008;
  float s2 = 0.0625f * (d+1) - 0.008;
  float t1 = 0.0625f * 3     + 0.006;
  float t2 = 0.0625f * 4     - 0.006;
  glTexCoord2f(s1, t2); glVertex3f(x,   y,   0.0f);
  glTexCoord2f(s1, t1); glVertex3f(x,   y+s, 0.0f);
  glTexCoord2f(s2, t1); glVertex3f(x+s, y+s, 0.0f);
  glTexCoord2f(s2, t2); glVertex3f(x+s, y,   0.0f);
}

void render_icon(r_context_t *c, float x, float y, float s) {
  glBindTexture(GL_TEXTURE_2D, c->r_textures[iconref]);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(x,   y,   0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(x,   y+s, 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(x+s, y+s, 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(x+s, y,   0.0f);
  glEnd();
}

static void
begin2D(int width, int height)
{
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, width, 0, height, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
}

static void
end2D(void)
{
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}

