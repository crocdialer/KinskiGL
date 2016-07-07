/*
 *
 *  BlueZ - Bluetooth protocol stack for Linux
 *
 *  Copyright (C) 2010  Nokia Corporation
 *  Copyright (C) 2010  Marcel Holtmann <marcel@holtmann.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */
#ifndef __GATTRIB_H
#define __GATTRIB_H

#ifdef __cplusplus
extern "C" {
#endif

#define GATTRIB_ALL_REQS 0xFE
#define GATTRIB_ALL_HANDLES 0x0000

struct bt_att;  /* Forward declaration for compatibility */
struct _GAttrib;
typedef struct _GAttrib GAttrib;

typedef void (*GAttribResultFunc) (uint8_t status, const uint8_t *pdu,
					uint16_t len, void* user_data);
typedef void (*GAttribDisconnectFunc)(void* user_data);
typedef void (*GAttribDebugFunc)(const char *str, void* user_data);
typedef void (*GAttribNotifyFunc)(const uint8_t *pdu, uint16_t len,
							void* user_data);

GAttrib *g_attrib_new(int io, uint16_t mtu, bool ext_signed);
GAttrib *g_attrib_ref(GAttrib *attrib);
void g_attrib_unref(GAttrib *attrib);

int g_attrib_get_channel(GAttrib *attrib);

struct bt_att *g_attrib_get_att(GAttrib *attrib);

int g_attrib_set_destroy_function(GAttrib *attrib,
		GDestroyNotify destroy, void* user_data);

uint32_t g_attrib_send(GAttrib *attrib, uint32_t id, const uint8_t *pdu, uint16_t len,
			GAttribResultFunc func, void* user_data,
			GDestroyNotify notify);

int g_attrib_cancel(GAttrib *attrib, uint32_t id);
int g_attrib_cancel_all(GAttrib *attrib);

uint32_t g_attrib_register(GAttrib *attrib, uint8_t opcode, uint16_t handle,
				GAttribNotifyFunc func, void* user_data,
				GDestroyNotify notify);

uint8_t *g_attrib_get_buffer(GAttrib *attrib, size_t *len);
int g_attrib_set_mtu(GAttrib *attrib, int mtu);

int g_attrib_unregister(GAttrib *attrib, uint32_t id);
int g_attrib_unregister_all(GAttrib *attrib);

#ifdef __cplusplus
}
#endif
#endif