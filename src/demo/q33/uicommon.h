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
#ifndef __UICOMMON_H__
#define __UICOMMON_H__

#include "vec.h"
#include "render.h"

#ifdef __cplusplus
extern "C" {
#endif

void ui_reset();
void ui_read_args(int argc, char **argv);
void ui_init_bsp();
void ui_init_gl(r_context_t *c);
bool ui_move(double time);
void ui_sync(r_context_t *c, double time);
void ui_display(r_context_t *c, double time);
void ui_display_nosync(r_context_t *c, double time);
void ui_mouse_down(int x, int y);
void ui_mouse_motion(int x, int y);
void ui_mouse_relmotion(int relx, int rely);
void ui_key_down(unsigned char key);
void ui_key_up(unsigned char key);
void ui_finish(r_context_t *c);

#ifdef __cplusplus
}
#endif

#endif /*__UICOMMON_H__*/
