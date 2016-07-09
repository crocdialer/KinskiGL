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

#include <stdlib.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/l2cap.h>

#include "uuid.h"
#include "util.h"
#include "att.h"
#include "gattrib.h"
#include "gatt.h"

struct discover_primary {
	int ref;
	GAttrib *attrib;
	unsigned int id;
	bt_uuid_t uuid;
	uint16_t start;
	gatt_primary_list primaries;
	gatt_cb_t cb;
	void *user_data;
};

/* Used for the Included Services Discovery (ISD) procedure */
struct included_discovery {
	GAttrib		*attrib;
	unsigned int	id;
	int		refs;
	int		err;
	uint16_t	start_handle;
	uint16_t	end_handle;
	gatt_included_list includes;
	gatt_cb_t	cb;
	void		*user_data;
};

struct included_uuid_query {
	struct included_discovery	*isd;
	struct gatt_included		*included;
};

struct discover_char {
	int ref;
	GAttrib *attrib;
	unsigned int id;
	bt_uuid_t *uuid;
	uint16_t end;
	uint16_t start;
	gatt_char_list characteristics;
	gatt_cb_t cb;
	void *user_data;
};

struct discover_desc {
	int ref;
	GAttrib *attrib;
	unsigned int id;
	bt_uuid_t *uuid;
	uint16_t start;
	uint16_t end;
	std::list<std::shared_ptr<gatt_desc>> descriptors;
	gatt_cb_t cb;
	void *user_data;
};

static void discover_primary_unref(void *data)
{
	discover_primary* dp = (discover_primary*)data;

	dp->ref--;

	if (dp->ref > 0)
		return;

	dp->primaries.clear();

	g_attrib_unref(dp->attrib);
	free(dp);
}

static struct discover_primary *discover_primary_ref(
						struct discover_primary *dp)
{
	dp->ref++;

	return dp;
}

static struct included_discovery *isd_ref(struct included_discovery *isd)
{
	__sync_fetch_and_add(&isd->refs, 1);

	return isd;
}

static void isd_unref(struct included_discovery *isd)
{
	if (__sync_sub_and_fetch(&isd->refs, 1) > 0)
		return;

	if (isd->err)
		isd->cb(isd->err, {}, isd->user_data);
	else
		isd->cb(isd->err, isd->includes, isd->user_data);

	isd->includes.clear();
	g_attrib_unref(isd->attrib);
	free(isd);
}

static void discover_char_unref(void *data)
{
	discover_char* dc = (discover_char*)data;

	dc->ref--;

	if (dc->ref > 0)
		return;

	dc->characteristics.clear();
	g_attrib_unref(dc->attrib);
	free(dc->uuid);
	free(dc);
}

static struct discover_char *discover_char_ref(struct discover_char *dc)
{
	dc->ref++;

	return dc;
}

static void discover_desc_unref(void *data)
{
	discover_desc* dd = (discover_desc*)data;

	dd->ref--;

	if (dd->ref > 0)
		return;

	dd->descriptors.clear();
	g_attrib_unref(dd->attrib);
	free(dd->uuid);
	free(dd);
}

static struct discover_desc *discover_desc_ref(struct discover_desc *dd)
{
	dd->ref++;

	return dd;
}

static void put_uuid_le(const bt_uuid_t *uuid, void *dst)
{
	if (uuid->type == bt_uuid_t::BT_UUID16)
		put_le16(uuid->value.u16, dst);
	else
		/* Convert from 128-bit BE to LE */
		bswap_128(&uuid->value.u128, dst);
}

static void get_uuid128(uint8_t type, const void *val, bt_uuid_t *uuid)
{
	if (type == bt_uuid_t::BT_UUID16) {
		bt_uuid_t uuid16;

		bt_uuid16_create(&uuid16, get_le16(val));
		bt_uuid_to_uuid128(&uuid16, uuid);
	} else {
		uint128_t u128;

		/* Convert from 128-bit LE to BE */
		bswap_128(val, &u128);
		bt_uuid128_create(uuid, u128);
	}
}

static uint16_t encode_discover_primary(uint16_t start, uint16_t end,
				bt_uuid_t *uuid, uint8_t *pdu, size_t len)
{
	bt_uuid_t prim;
	uint16_t plen;

	bt_uuid16_create(&prim, GATT_PRIM_SVC_UUID);

	if (uuid == NULL) {
		/* Discover all primary services */
		plen = enc_read_by_grp_req(start, end, &prim, pdu, len);
	} else {
		uint8_t value[16];
		size_t vlen;

		/* Discover primary service by service UUID */
		put_uuid_le(uuid, value);
		vlen = bt_uuid_len(uuid);

		plen = enc_find_by_type_req(start, end, &prim, value, vlen,
								pdu, len);
	}

	return plen;
}

static void primary_by_uuid_cb(uint32_t status, const uint8_t *ipdu,
							   uint16_t iplen, void* user_data)

{
	discover_primary* dp = (discover_primary*)user_data;
	uint8_t *buf;
	uint16_t oplen;
	int err = 0;
	size_t buflen;

	if(status) 
	{
		err = status == ATT_ECODE_ATTR_NOT_FOUND ? 0 : status;
		goto done;
	}

	auto ranges = dec_find_by_type_resp(ipdu, iplen);

	if(ranges.empty())
		goto done;

	dp->primaries.insert(dp->primaries.end(), ranges.begin(), ranges.end());

	// last = g_slist_last(ranges);
	std::shared_ptr<att_range> range = ranges.back();

	if (range->end == 0xffff)
		goto done;

	/*
	 * If last handle is lower from previous start handle then it is smth
	 * wrong. Let's stop search, otherwise we might enter infinite loop.
	 */
	if (range->end < dp->start) {
		err = ATT_ECODE_UNLIKELY;
		goto done;
	}

	dp->start = range->end + 1;

	buf = g_attrib_get_buffer(dp->attrib, &buflen);
	oplen = encode_discover_primary(dp->start, 0xffff, &dp->uuid,
								buf, buflen);

	if (oplen == 0)
		goto done;

	// TODO: use Gattrib c++ class
	// g_attrib_send(dp->attrib, dp->id, buf, oplen, primary_by_uuid_cb,
	// 		discover_primary_ref(dp), discover_primary_unref);
	// return;

done:
	dp->cb(err, dp->primaries, dp->user_data);
}

static void primary_all_cb(uint32_t8 status, const uint32_t8 *ipdu, uint16_t iplen,
							void* user_data)
{
	struct discover_primary *dp = user_data;
	struct att_data_list *list;
	unsigned int i, err;
	uint16_t start, end;
	uint8_t type;

	if (status) {
		err = status == ATT_ECODE_ATTR_NOT_FOUND ? 0 : status;
		goto done;
	}

	list = dec_read_by_grp_resp(ipdu, iplen);
	if (list == NULL) {
		err = ATT_ECODE_IO;
		goto done;
	}

	if (list->len == 6)
		type = bt_uuid_t::BT_UUID16;
	else if (list->len == 20)
		type = bt_uuid_t::BT_UUID128;
	else {
		att_data_list_free(list);
		err = ATT_ECODE_INVALID_PDU;
		goto done;
	}

	for (i = 0, end = 0; i < list->num; i++) {
		const uint8_t *data = list->data[i];
		struct gatt_primary *primary;
		bt_uuid_t uuid128;

		start = get_le16(&data[0]);
		end = get_le16(&data[2]);

		get_uuid128(type, &data[4], &uuid128);

		primary = g_try_new0(struct gatt_primary, 1);
		if (!primary) {
			att_data_list_free(list);
			err = ATT_ECODE_INSUFF_RESOURCES;
			goto done;
		}
		primary->range.start = start;
		primary->range.end = end;
		bt_uuid_to_string(&uuid128, primary->uuid, sizeof(primary->uuid));
		dp->primaries = g_slist_append(dp->primaries, primary);
	}

	att_data_list_free(list);
	err = 0;

	/*
	 * If last handle is lower from previous start handle then it is smth
	 * wrong. Let's stop search, otherwise we might enter infinite loop.
	 */
	if (end < dp->start) {
		err = ATT_ECODE_UNLIKELY;
		goto done;
	}

	dp->start = end + 1;

	if (end != 0xffff) {
		size_t buflen;
		uint8_t *buf = g_attrib_get_buffer(dp->attrib, &buflen);
		uint16_t oplen = encode_discover_primary(dp->start, 0xffff, NULL,
								buf, buflen);


		g_attrib_send(dp->attrib, dp->id, buf, oplen, primary_all_cb,
						discover_primary_ref(dp),
						discover_primary_unref);

		return;
	}

done:
	dp->cb(err, dp->primaries, dp->user_data);
}

uint32_t gatt_discover_primary(GAttrib *attrib, bt_uuid_t *uuid, gatt_cb_t func,
							void* user_data)
{
	struct discover_primary *dp;
	size_t buflen;
	uint8_t *buf = g_attrib_get_buffer(attrib, &buflen);
	GAttribResultFunc cb;
	uint16_t plen;

	plen = encode_discover_primary(0x0001, 0xffff, uuid, buf, buflen);
	if (plen == 0)
		return 0;

	dp = g_try_new0(struct discover_primary, 1);
	if (dp == NULL)
		return 0;

	dp->attrib = g_attrib_ref(attrib);
	dp->cb = func;
	dp->user_data = user_data;
	dp->start = 0x0001;

	if (uuid) {
		dp->uuid = *uuid;
		cb = primary_by_uuid_cb;
	} else
		cb = primary_all_cb;

	dp->id = g_attrib_send(attrib, 0, buf, plen, cb,
					discover_primary_ref(dp),
					discover_primary_unref);

	return dp->id;
}

static void resolve_included_uuid_cb(uint8_t status, const uint8_t *pdu,
					uint16_t len, void* user_data)
{
	struct included_uuid_query *query = user_data;
	struct included_discovery *isd = query->isd;
	struct gatt_included *incl = query->included;
	unsigned int err = status;
	bt_uuid_t uuid128;
	size_t buflen;
	uint8_t *buf;

	if (err)
		goto done;

	buf = g_attrib_get_buffer(isd->attrib, &buflen);
	if (dec_read_resp(pdu, len, buf, buflen) != 16) {
		err = ATT_ECODE_IO;
		goto done;
	}

	get_uuid128(bt_uuid_t::BT_UUID128, buf, &uuid128);

	bt_uuid_to_string(&uuid128, incl->uuid, sizeof(incl->uuid));
	isd->includes = g_slist_append(isd->includes, incl);
	query->included = NULL;

done:
	if (isd->err == 0)
		isd->err = err;
}

static void inc_query_free(void *data)
{
	struct included_uuid_query *query = data;

	isd_unref(query->isd);
	free(query->included);
	free(query);
}

static uint32_t resolve_included_uuid(struct included_discovery *isd,
					struct gatt_included *incl)
{
	struct included_uuid_query *query;
	size_t buflen;
	uint8_t *buf = g_attrib_get_buffer(isd->attrib, &buflen);
	uint16_t oplen = enc_read_req(incl->range.start, buf, buflen);

	query = g_new0(struct included_uuid_query, 1);
	query->isd = isd_ref(isd);
	query->included = incl;

	return g_attrib_send(isd->attrib, query->isd->id, buf, oplen,
				resolve_included_uuid_cb, query,
				inc_query_free);
}

static struct gatt_included *included_from_buf(const uint8_t *buf, gsize len)
{
	struct gatt_included *incl = g_new0(struct gatt_included, 1);

	incl->handle = get_le16(&buf[0]);
	incl->range.start = get_le16(&buf[2]);
	incl->range.end = get_le16(&buf[4]);

	if (len == 8) {
		bt_uuid_t uuid128;

		get_uuid128(bt_uuid_t::BT_UUID16, &buf[6], &uuid128);
		bt_uuid_to_string(&uuid128, incl->uuid, sizeof(incl->uuid));
	}

	return incl;
}

static void find_included_cb(uint8_t status, const uint8_t *pdu, uint16_t len,
							void* user_data);

static uint32_t find_included(struct included_discovery *isd, uint16_t start)
{
	bt_uuid_t uuid;
	size_t buflen;
	uint8_t *buf = g_attrib_get_buffer(isd->attrib, &buflen);
	uint16_t oplen;

	bt_uuid16_create(&uuid, GATT_INCLUDE_UUID);
	oplen = enc_read_by_type_req(start, isd->end_handle, &uuid,
							buf, buflen);

	/* If id != 0 it means we are in the middle of include search */
	if (isd->id)
		return g_attrib_send(isd->attrib, isd->id, buf, oplen,
				find_included_cb, isd_ref(isd),
				(GDestroyNotify) isd_unref);

	/* This is first call from the gattrib user */
	isd->id = g_attrib_send(isd->attrib, 0, buf, oplen, find_included_cb,
				isd_ref(isd), (GDestroyNotify) isd_unref);

	return isd->id;
}

static void find_included_cb(uint8_t status, const uint8_t *pdu, uint16_t len,
							void* user_data)
{
	struct included_discovery *isd = user_data;
	uint16_t last_handle = isd->end_handle;
	unsigned int err = status;
	struct att_data_list *list;
	int i;

	if (err == ATT_ECODE_ATTR_NOT_FOUND)
		err = 0;

	if (status)
		goto done;

	list = dec_read_by_type_resp(pdu, len);
	if (list == NULL) {
		err = ATT_ECODE_IO;
		goto done;
	}

	if (list->len != 6 && list->len != 8) {
		err = ATT_ECODE_IO;
		att_data_list_free(list);
		goto done;
	}

	for (i = 0; i < list->num; i++) {
		struct gatt_included *incl;

		incl = included_from_buf(list->data[i], list->len);
		last_handle = incl->handle;

		/* 128 bit UUID, needs resolving */
		if (list->len == 6) {
			resolve_included_uuid(isd, incl);
			continue;
		}

		isd->includes = g_slist_append(isd->includes, incl);
	}

	att_data_list_free(list);

	/*
	 * If last handle is lower from previous start handle then it is smth
	 * wrong. Let's stop search, otherwise we might enter infinite loop.
	 */
	if (last_handle < isd->start_handle) {
		isd->err = ATT_ECODE_UNLIKELY;
		goto done;
	}

	isd->start_handle = last_handle + 1;

	if (last_handle < isd->end_handle)
		find_included(isd, isd->start_handle);

done:
	if (isd->err == 0)
		isd->err = err;
}

uint32_t gatt_find_included(GAttrib *attrib, uint16_t start, uint16_t end,
							gatt_cb_t func, void* user_data)
{
	struct included_discovery *isd;

	isd = g_new0(struct included_discovery, 1);
	isd->attrib = g_attrib_ref(attrib);
	isd->start_handle = start;
	isd->end_handle = end;
	isd->cb = func;
	isd->user_data = user_data;

	return find_included(isd, start);
}

static void char_discovered_cb(uint32_t8 status, const uint32_t8 *ipdu, uint16_t iplen,
							   void* user_data)
{
	struct discover_char *dc = user_data;
	struct att_data_list *list;
	unsigned int i, err = 0;
	uint16_t last = 0;
	uint8_t type;

	/* We have all the characteristic now, lets send it up */
	if (status == ATT_ECODE_ATTR_NOT_FOUND) {
		err = dc->characteristics ? 0 : status;
		goto done;
	}

	if (status) {
		err = status;
		goto done;
	}

	list = dec_read_by_type_resp(ipdu, iplen);
	if (list == NULL) {
		err = ATT_ECODE_IO;
		goto done;
	}

	if (list->len == 7)
		type = bt_uuid_t::BT_UUID16;
	else
		type = bt_uuid_t::BT_UUID128;

	for (i = 0; i < list->num; i++) {
		uint8_t *value = list->data[i];

		bt_uuid_t uuid128;

		last = get_le16(value);

		get_uuid128(type, &value[5], &uuid128);

		if (dc->uuid && bt_uuid_cmp(dc->uuid, &uuid128))
			continue;

		auto chars = std::make_shared<gatt_char>();

		// if (!chars)
		// {
		// 	att_data_list_free(list);
		// 	err = ATT_ECODE_INSUFF_RESOURCES;
		// 	goto done;
		// }

		chars->handle = last;
		chars->properties = value[2];
		chars->value_handle = get_le16(&value[3]);
		bt_uuid_to_string(&uuid128, chars->uuid, sizeof(chars->uuid));
		dc->characteristics = g_slist_append(dc->characteristics,
									chars);
	}

	att_data_list_free(list);

	/*
	 * If last handle is lower from previous start handle then it is smth
	 * wrong. Let's stop search, otherwise we might enter infinite loop.
	 */
	if (last < dc->start) {
		err = ATT_ECODE_UNLIKELY;
		goto done;
	}

	dc->start = last + 1;

	if (last != 0 && (dc->start < dc->end)) {
		bt_uuid_t uuid;
		uint16_t oplen;
		size_t buflen;
		uint8_t *buf;

		buf = g_attrib_get_buffer(dc->attrib, &buflen);

		bt_uuid16_create(&uuid, GATT_CHARAC_UUID);

		oplen = enc_read_by_type_req(dc->start, dc->end, &uuid, buf,
									buflen);

		if (oplen == 0)
			return;

		g_attrib_send(dc->attrib, dc->id, buf, oplen,
				char_discovered_cb, discover_char_ref(dc),
				discover_char_unref);

		return;
	}

done:
	dc->cb(err, dc->characteristics, dc->user_data);
}

uint32_t gatt_discover_char(GAttrib *attrib, uint16_t start, uint16_t end,
						bt_uuid_t *uuid, gatt_cb_t func,
						void* user_data)
{
	size_t buflen;
	uint8_t *buf = g_attrib_get_buffer(attrib, &buflen);
	struct discover_char *dc;
	bt_uuid_t type_uuid;
	uint16_t plen;

	bt_uuid16_create(&type_uuid, GATT_CHARAC_UUID);

	plen = enc_read_by_type_req(start, end, &type_uuid, buf, buflen);
	if (plen == 0)
		return 0;

	dc = g_try_new0(struct discover_char, 1);
	if (dc == NULL)
		return 0;

	dc->attrib = g_attrib_ref(attrib);
	dc->cb = func;
	dc->user_data = user_data;
	dc->end = end;
	dc->start = start;
	dc->uuid = g_memdup(uuid, sizeof(bt_uuid_t));

	dc->id = g_attrib_send(attrib, 0, buf, plen, char_discovered_cb,
				discover_char_ref(dc), discover_char_unref);

	return dc->id;
}

uint32_t gatt_read_char_by_uuid(GAttrib *attrib, uint16_t start, uint16_t end,
					bt_uuid_t *uuid, GAttribResultFunc func,
					void* user_data)
{
	size_t buflen;
	uint8_t *buf = g_attrib_get_buffer(attrib, &buflen);
	uint16_t plen;

	plen = enc_read_by_type_req(start, end, uuid, buf, buflen);
	if (plen == 0)
		return 0;

	return g_attrib_send(attrib, 0, buf, plen, func, user_data, NULL);
}

struct read_long_data {
	GAttrib *attrib;
	GAttribResultFunc func;
	void* user_data;
	uint32_t8 *buffer;
	uint16_t size;
	uint16_t handle;
	uint32_t id;
	int ref;
};

static void read_long_destroy(void* user_data)
{
	struct read_long_data *long_read = user_data;

	if (__sync_sub_and_fetch(&long_read->ref, 1) > 0)
		return;

	g_attrib_unref(long_read->attrib);

	if (long_read->buffer != NULL)
		free(long_read->buffer);

	free(long_read);
}

static void read_blob_helper(uint32_t8 status, const uint32_t8 *rpdu, uint16_t rlen,
							void* user_data)
{
	struct read_long_data *long_read = user_data;
	uint8_t *buf;
	size_t buflen;
	uint32_t8 *tmp;
	uint16_t plen;
	uint32_t id;

	if (status != 0 || rlen == 1) {
		status = 0;
		goto done;
	}

	tmp = g_try_realloc(long_read->buffer, long_read->size + rlen - 1);

	if (tmp == NULL) {
		status = ATT_ECODE_INSUFF_RESOURCES;
		goto done;
	}

	memcpy(&tmp[long_read->size], &rpdu[1], rlen - 1);
	long_read->buffer = tmp;
	long_read->size += rlen - 1;

	buf = g_attrib_get_buffer(long_read->attrib, &buflen);
	if (rlen < buflen)
		goto done;

	plen = enc_read_blob_req(long_read->handle, long_read->size - 1,
								buf, buflen);
	id = g_attrib_send(long_read->attrib, long_read->id, buf, plen,
				read_blob_helper, long_read, read_long_destroy);

	if (id != 0) {
		__sync_fetch_and_add(&long_read->ref, 1);
		return;
	}

	status = ATT_ECODE_IO;

done:
	long_read->func(status, long_read->buffer, long_read->size,
							long_read->user_data);
}

static void read_char_helper(uint32_t8 status, const uint32_t8 *rpdu,
					uint16_t rlen, void* user_data)
{
	struct read_long_data *long_read = user_data;
	size_t buflen;
	uint8_t *buf = g_attrib_get_buffer(long_read->attrib, &buflen);
	uint16_t plen;
	uint32_t id;

	if (status != 0 || rlen < buflen)
		goto done;

	long_read->buffer = g_malloc(rlen);
	if (long_read->buffer == NULL) {
		status = ATT_ECODE_INSUFF_RESOURCES;
		goto done;
	}

	memcpy(long_read->buffer, rpdu, rlen);
	long_read->size = rlen;

	plen = enc_read_blob_req(long_read->handle, rlen - 1, buf, buflen);

	id = g_attrib_send(long_read->attrib, long_read->id, buf, plen,
				read_blob_helper, long_read, read_long_destroy);
	if (id != 0) {
		__sync_fetch_and_add(&long_read->ref, 1);
		return;
	}

	status = ATT_ECODE_IO;

done:
	long_read->func(status, rpdu, rlen, long_read->user_data);
}

uint32_t gatt_read_char(GAttrib *attrib, uint16_t handle, GAttribResultFunc func,
							void* user_data)
{
	uint8_t *buf;
	size_t buflen;
	uint16_t plen;
	uint32_t id;
	struct read_long_data *long_read;

	long_read = g_try_new0(struct read_long_data, 1);

	if (long_read == NULL)
		return 0;

	long_read->attrib = g_attrib_ref(attrib);
	long_read->func = func;
	long_read->user_data = user_data;
	long_read->handle = handle;

	buf = g_attrib_get_buffer(attrib, &buflen);
	plen = enc_read_req(handle, buf, buflen);
	id = g_attrib_send(attrib, 0, buf, plen, read_char_helper,
						long_read, read_long_destroy);
	if (id == 0) {
		g_attrib_unref(long_read->attrib);
		free(long_read);
	} else {
		__sync_fetch_and_add(&long_read->ref, 1);
		long_read->id = id;
	}

	return id;
}

struct write_long_data {
	GAttrib *attrib;
	GAttribResultFunc func;
	void* user_data;
	uint16_t handle;
	uint16_t offset;
	uint8_t *value;
	size_t vlen;
};

static uint32_t execute_write(GAttrib *attrib, uint8_t flags,
				GAttribResultFunc func, void* user_data)
{
	uint8_t *buf;
	size_t buflen;
	uint16_t plen;

	buf = g_attrib_get_buffer(attrib, &buflen);
	plen = enc_exec_write_req(flags, buf, buflen);
	if (plen == 0)
		return 0;

	return g_attrib_send(attrib, 0, buf, plen, func, user_data, NULL);
}

static uint32_t prepare_write(struct write_long_data *long_write);

static void prepare_write_cb(uint32_t8 status, const uint32_t8 *rpdu, uint16_t rlen,
							void* user_data)
{
	struct write_long_data *long_write = user_data;

	if (status != 0) {
		long_write->func(status, rpdu, rlen, long_write->user_data);
		return;
	}

	/* Skip Prepare Write Response PDU header (5 bytes) */
	long_write->offset += rlen - 5;

	if (long_write->offset == long_write->vlen) {
		execute_write(long_write->attrib, ATT_WRITE_ALL_PREP_WRITES,
				long_write->func, long_write->user_data);
		free(long_write->value);
		free(long_write);

		return;
	}

	prepare_write(long_write);
}

static uint32_t prepare_write(struct write_long_data *long_write)
{
	GAttrib *attrib = long_write->attrib;
	uint16_t handle = long_write->handle;
	uint16_t offset = long_write->offset;
	uint8_t *buf, *value = long_write->value + offset;
	size_t buflen, vlen = long_write->vlen - offset;
	uint16_t plen;

	buf = g_attrib_get_buffer(attrib, &buflen);

	plen = enc_prep_write_req(handle, offset, value, vlen, buf, buflen);
	if (plen == 0)
		return 0;

	return g_attrib_send(attrib, 0, buf, plen, prepare_write_cb, long_write,
									NULL);
}

uint32_t gatt_write_char(GAttrib *attrib, uint16_t handle, const uint8_t *value,
			size_t vlen, GAttribResultFunc func, void* user_data)
{
	uint8_t *buf;
	size_t buflen;
	struct write_long_data *long_write;

	buf = g_attrib_get_buffer(attrib, &buflen);

	/* Use Write Request if payload fits on a single transfer, including 3
	 * bytes for the header. */
	if (vlen <= buflen - 3) {
		uint16_t plen;

		plen = enc_write_req(handle, value, vlen, buf, buflen);
		if (plen == 0)
			return 0;

		return g_attrib_send(attrib, 0, buf, plen, func, user_data,
									NULL);
	}

	/* Write Long Characteristic Values */
	long_write = g_try_new0(struct write_long_data, 1);
	if (long_write == NULL)
		return 0;

	long_write->attrib = attrib;
	long_write->func = func;
	long_write->user_data = user_data;
	long_write->handle = handle;
	long_write->value = g_memdup(value, vlen);
	long_write->vlen = vlen;

	return prepare_write(long_write);
}

uint32_t gatt_execute_write(GAttrib *attrib, uint8_t flags,
				GAttribResultFunc func, void* user_data)
{
	return execute_write(attrib, flags, func, user_data);
}

uint32_t gatt_reliable_write_char(GAttrib *attrib, uint16_t handle,
					const uint8_t *value, size_t vlen,
					GAttribResultFunc func,
					void* user_data)
{
	uint8_t *buf;
	uint16_t plen;
	size_t buflen;

	buf = g_attrib_get_buffer(attrib, &buflen);

	plen = enc_prep_write_req(handle, 0, value, vlen, buf, buflen);
	if (!plen)
		return 0;

	return g_attrib_send(attrib, 0, buf, plen, func, user_data, NULL);
}

uint32_t gatt_exchange_mtu(GAttrib *attrib, uint16_t mtu, GAttribResultFunc func,
							void* user_data)
{
	uint8_t *buf;
	size_t buflen;
	uint16_t plen;

	buf = g_attrib_get_buffer(attrib, &buflen);
	plen = enc_mtu_req(mtu, buf, buflen);
	return g_attrib_send(attrib, 0, buf, plen, func, user_data, NULL);
}

static void desc_discovered_cb(uint32_t8 status, const uint32_t8 *ipdu,
					uint16_t iplen, void* user_data)
{
	struct discover_desc *dd = user_data;
	struct att_data_list *list;
	unsigned int i, err = 0;
	uint32_t8 format;
	uint16_t last = 0xffff;
	uint8_t type;
	bool uuid_found = false;

	if (status == ATT_ECODE_ATTR_NOT_FOUND) {
		err = dd->descriptors ? 0 : status;
		goto done;
	}

	if (status) {
		err = status;
		goto done;
	}

	list = dec_find_info_resp(ipdu, iplen, &format);
	if (!list) {
		err = ATT_ECODE_IO;
		goto done;
	}

	if (format == ATT_FIND_INFO_RESP_FMT_16BIT)
		type = bt_uuid_t::BT_UUID16;
	else
		type = bt_uuid_t::BT_UUID128;

	for (i = 0; i < list->num; i++) {
		uint8_t *value = list->data[i];
		struct gatt_desc *desc;
		bt_uuid_t uuid128;

		last = get_le16(value);

		get_uuid128(type, &value[2], &uuid128);

		if (dd->uuid) {
			if (bt_uuid_cmp(dd->uuid, &uuid128))
				continue;
			else
				uuid_found = true;
		}

		desc = g_try_new0(struct gatt_desc, 1);
		if (!desc) {
			att_data_list_free(list);
			err = ATT_ECODE_INSUFF_RESOURCES;
			goto done;
		}

		bt_uuid_to_string(&uuid128, desc->uuid, sizeof(desc->uuid));
		desc->handle = last;

		if (type == bt_uuid_t::BT_UUID16)
			desc->uuid16 = get_le16(&value[2]);

		dd->descriptors = g_slist_append(dd->descriptors, desc);

		if (uuid_found)
			break;
	}

	att_data_list_free(list);

	/*
	 * If last handle is lower from previous start handle then it is smth
	 * wrong. Let's stop search, otherwise we might enter infinite loop.
	 */
	if (last < dd->start) {
		err = ATT_ECODE_UNLIKELY;
		goto done;
	}

	dd->start = last + 1;

	if (last < dd->end && !uuid_found) {
		uint16_t oplen;
		size_t buflen;
		uint8_t *buf;

		buf = g_attrib_get_buffer(dd->attrib, &buflen);

		oplen = enc_find_info_req(dd->start, dd->end, buf, buflen);
		if (oplen == 0)
			return;

		g_attrib_send(dd->attrib, dd->id, buf, oplen,
				desc_discovered_cb, discover_desc_ref(dd),
				discover_desc_unref);

		return;
	}

done:
	dd->cb(err, dd->descriptors, dd->user_data);
}

uint32_t gatt_discover_desc(GAttrib *attrib, uint16_t start, uint16_t end,
						bt_uuid_t *uuid, gatt_cb_t func,
						void* user_data)
{
	size_t buflen;
	uint8_t *buf = g_attrib_get_buffer(attrib, &buflen);
	struct discover_desc *dd;
	uint16_t plen;

	plen = enc_find_info_req(start, end, buf, buflen);
	if (plen == 0)
		return 0;

	dd = g_try_new0(struct discover_desc, 1);
	if (dd == NULL)
		return 0;

	dd->attrib = g_attrib_ref(attrib);
	dd->cb = func;
	dd->user_data = user_data;
	dd->start = start;
	dd->end = end;
	dd->uuid = g_memdup(uuid, sizeof(bt_uuid_t));

	dd->id = g_attrib_send(attrib, 0, buf, plen, desc_discovered_cb,
				discover_desc_ref(dd), discover_desc_unref);

	return dd->id;
}

uint32_t gatt_write_cmd(GAttrib *attrib, uint16_t handle, const uint8_t *value,
			int vlen, GDestroyNotify notify, void* user_data)
{
	uint8_t *buf;
	size_t buflen;
	uint16_t plen;

	buf = g_attrib_get_buffer(attrib, &buflen);
	plen = enc_write_cmd(handle, value, vlen, buf, buflen);
	return g_attrib_send(attrib, 0, buf, plen, NULL, user_data, notify);
}

uint32_t gatt_signed_write_cmd(GAttrib *attrib, uint16_t handle,
						const uint8_t *value, int vlen,
						struct bt_crypto *crypto,
						const uint8_t csrk[16],
						uint32_t sign_cnt,
						GDestroyNotify notify,
						void* user_data)
{
	uint8_t *buf;
	size_t buflen;
	uint16_t plen;

	buf = g_attrib_get_buffer(attrib, &buflen);
	plen = enc_signed_write_cmd(handle, value, vlen, crypto, csrk, sign_cnt,
								buf, buflen);
	if (plen == 0)
		return 0;

	return g_attrib_send(attrib, 0, buf, plen, NULL, user_data, notify);
}

static sdp_data_t *proto_seq_find(sdp_list_t *proto_list)
{
	sdp_list_t *list;
	uuid_t proto;

	sdp_uuid16_create(&proto, ATT_UUID);

	for (list = proto_list; list; list = list->next) {
		sdp_list_t *p;
		for (p = list->data; p; p = p->next) {
			sdp_data_t *seq = p->data;
			if (seq && seq->dtd == SDP_UUID16 &&
				sdp_uuid16_cmp(&proto, &seq->val.uuid) == 0)
				return seq->next;
		}
	}

	return NULL;
}

static bool parse_proto_params(sdp_list_t *proto_list, uint16_t *psm,
						uint16_t *start, uint16_t *end)
{
	sdp_data_t *seq1, *seq2;

	if (psm)
		*psm = sdp_get_proto_port(proto_list, L2CAP_UUID);

	/* Getting start and end handle */
	seq1 = proto_seq_find(proto_list);
	if (!seq1 || seq1->dtd != SDP_UINT16)
		return false;

	seq2 = seq1->next;
	if (!seq2 || seq2->dtd != SDP_UINT16)
		return false;

	if (start)
		*start = seq1->val.uint16;

	if (end)
		*end = seq2->val.uint16;

	return true;
}

bool gatt_parse_record(const sdp_record_t *rec,
					uuid_t *prim_uuid, uint16_t *psm,
					uint16_t *start, uint16_t *end)
{
	sdp_list_t *list;
	uuid_t uuid;
	bool ret;

	if (sdp_get_service_classes(rec, &list) < 0)
		return false;

	memcpy(&uuid, list->data, sizeof(uuid));
	sdp_list_free(list, free);

	if (sdp_get_access_protos(rec, &list) < 0)
		return false;

	ret = parse_proto_params(list, psm, start, end);

	sdp_list_foreach(list, (sdp_list_func_t) sdp_list_free, NULL);
	sdp_list_free(list, NULL);

	/* FIXME: replace by bt_uuid_t after uuid_t/sdp code cleanup */
	if (ret && prim_uuid)
		memcpy(prim_uuid, &uuid, sizeof(uuid_t));

	return ret;
}
