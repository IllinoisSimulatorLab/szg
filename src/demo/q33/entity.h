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
#ifndef __ENTITY_H__
#define __ENTITY_H__

/* Entities groups of key-value pairs stored in the BSP file */

#ifdef __cplusplus
extern "C" {
#endif

void entity_parse(int buflen, char *buf);
void entity_free(void);
const char *entity_value(int entity, const char *key);
float entity_float(int entity, const char *key);
void entity_vec3(int entity, const char *key, vec3_t vec);

#ifdef __cplusplus
}
#endif

#endif /* __ENTITY_H__ */
