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
#ifndef __PAK_H__
#define __PAK_H__

#ifdef __cplusplus
extern "C" {
#endif

void pak_openpak(const char *pakfile);
void pak_closepak(void);
int pak_open(const char *path);
void pak_close(void);
int pak_read(void *buf, uint_t size, uint_t nmemb);
uint_t pak_getlen(void);
uint_t pak_readfile(const char *path, uint_t bufsize, byte_t *buf);
uint_t pak_listshaders(uint_t bufsize, char *buf);
/* int pak_checkfile(const char *path); */ /* not used */

#ifdef __cplusplus
}
#endif

#endif /*__PAK_H__*/
