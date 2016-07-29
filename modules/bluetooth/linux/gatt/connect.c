/*
 *
 *  GattLib - GATT Library
 *
 *  Copyright (C) 2016  Olivier Martin <olivier@labapart.org>
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

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <bluetooth/bluetooth.h>
#include "internal.h"
#include "uuid.h"
#include "att.h"
#include "btio.h"
#include "gattrib.h"

#define CONNECTION_TIMEOUT    2

struct gattlib_thread_t g_gattlib_thread = { 0 };

typedef struct {
	gatt_connection_t* conn;
	gatt_connect_cb_t  connect_cb;
	int                connected;
	int                timeout;
	GError*            error;
} io_connect_arg_t;

static void events_handler(const uint8_t *pdu, uint16_t len, gpointer user_data) {
	gatt_connection_t *conn = user_data;
	uint8_t opdu[ATT_MAX_MTU];
	uint16_t handle, i, olen = 0;

	handle = att_get_u16(&pdu[1]);
	// printf("0x%02x\n", handle);

	switch (pdu[0]) {
	case ATT_OP_HANDLE_NOTIFY:
		if (conn->notification_handler) {
			conn->notification_handler(handle, &pdu[3], len, conn->notification_user_data);
		}
		break;
	case ATT_OP_HANDLE_IND:
		if (conn->indication_handler) {
			conn->indication_handler(handle, &pdu[3], len, conn->indication_user_data);
		}
		break;
	default:
		g_print("Invalid opcode\n");
		return;
	}

	if (pdu[0] == ATT_OP_HANDLE_NOTIFY)
		return;

	olen = enc_confirmation(opdu, sizeof(opdu));

	if (olen > 0)
		g_attrib_send(conn->attrib, 0, opdu[0], opdu, olen, NULL, NULL, NULL);
}

static void task_dec()
{
	//TODO: Add a mutex around this code to avoid a race condition
	/* Decrease the reference counter of the loop */
	// g_gattlib_thread.ref--;

	/* Check if we are the last one */
	if (g_atomic_int_dec_and_test(&g_gattlib_thread.ref))
	{
		g_main_loop_quit(g_gattlib_thread.loop);
		// g_main_loop_unref(g_gattlib_thread.loop);
		// g_main_context_unref(g_gattlib_thread.loop_context);

		// Detach the thread
		if(pthread_detach(g_gattlib_thread.thread))
		{
			printf("pthread_detach failed\n");
		}
	}
}

static gboolean io_listen_cb(gpointer user_data) {
	gatt_connection_t *conn = user_data;

	g_attrib_register(conn->attrib, ATT_OP_HANDLE_NOTIFY, events_handler, conn, NULL);
	g_attrib_register(conn->attrib, ATT_OP_HANDLE_IND, events_handler, conn, NULL);

	return FALSE;
}

static void io_connect_cb(GIOChannel *io, GError *err, gpointer user_data) {
	io_connect_arg_t* io_connect_arg = user_data;

	if (err) {
		io_connect_arg->error = err;

		// Call callback if defined
		if (io_connect_arg->connect_cb) {
			io_connect_arg->connect_cb(NULL);
		}
	} else {
		io_connect_arg->conn->attrib = g_attrib_new(io);

		//
		// Register the listener callback
		//
		GSource *source = g_idle_source_new ();
		assert(source != NULL);

		g_source_set_callback(source, io_listen_cb, io_connect_arg->conn, NULL);

		// Attaches the listener to the main loop context
		guint id = g_source_attach(source, g_gattlib_thread.loop_context);
		g_source_unref (source);
		assert(id != 0);

		//
		// Call callback if defined
		//
		if (io_connect_arg->connect_cb) {
			io_connect_arg->connect_cb(io_connect_arg->conn);
		}

		io_connect_arg->connected = TRUE;
	}
	if (io_connect_arg->connect_cb) {
		free(io_connect_arg);
	}
}

static void *connection_thread(void* arg) {
	printf("connection thread started\n");
	struct gattlib_thread_t* loop_thread = arg;

	loop_thread->loop_context = g_main_context_new();
	loop_thread->loop = g_main_loop_new(loop_thread->loop_context, FALSE);
	g_main_loop_run(loop_thread->loop);
	g_main_loop_unref(loop_thread->loop);
	g_main_context_unref(loop_thread->loop_context);
	printf("connection thread ended\n");
	assert(0);
}

static gatt_connection_t *initialize_gattlib_connection(const gchar *src, const gchar *dst,
		uint8_t dest_type, BtIOSecLevel sec_level, int psm, int mtu,
		gatt_connect_cb_t connect_cb,
		io_connect_arg_t* io_connect_arg)
{
	bdaddr_t sba, dba;
	GError *err = NULL;

	/* Check if the GattLib thread has been started */
	if(g_atomic_int_compare_and_exchange(&g_gattlib_thread.ref, 0, 1))
	{
		/* Create a thread that will handle Bluetooth events */
		int error = pthread_create(&g_gattlib_thread.thread, NULL, &connection_thread, &g_gattlib_thread);
		if (error != 0) {
			fprintf(stderr, "Cannot create connection thread: %s", strerror(error));
			return NULL;
		}

		/* Wait for the loop to be started */
		while (!g_gattlib_thread.loop || !g_main_loop_is_running (g_gattlib_thread.loop)) {
			usleep(1000);
		}
	} else {
		/* Increase the reference to know how many GATT connection use the loop */
		g_atomic_int_inc(&g_gattlib_thread.ref);
	}

	/* Remote device */
	if (dst == NULL) {
		fprintf(stderr, "Remote Bluetooth address required\n");
		return NULL;
	}
	str2ba(dst, &dba);

	/* Local adapter */
	if (src != NULL) {
		if (!strncmp(src, "hci", 3))
			hci_devba(atoi(src + 3), &sba);
		else
			str2ba(src, &sba);
	} else
		bacpy(&sba, BDADDR_ANY);

	/* Not used for BR/EDR */
	if ((dest_type != BDADDR_LE_PUBLIC) && (dest_type != BDADDR_LE_RANDOM)) {
		return NULL;
	}

	if ((sec_level != BT_IO_SEC_LOW) && (sec_level != BT_IO_SEC_MEDIUM) && (sec_level != BT_IO_SEC_HIGH)) {
		return NULL;
	}

	gatt_connection_t* conn = calloc(sizeof(gatt_connection_t), 1);
	if (conn == NULL) {
		return NULL;
	}

	/* Intialize bt_io_connect argument */
	io_connect_arg->conn       = conn;
	io_connect_arg->connect_cb = connect_cb;
	io_connect_arg->connected  = FALSE;
	io_connect_arg->timeout    = FALSE;
	io_connect_arg->error      = NULL;

	if (psm == 0)
		conn->io = bt_io_connect(BT_IO_L2CAP, io_connect_cb, io_connect_arg, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &sba,
				BT_IO_OPT_DEST_BDADDR, &dba,
				BT_IO_OPT_DEST_TYPE, dest_type,
				BT_IO_OPT_CID, ATT_CID,
				BT_IO_OPT_SEC_LEVEL, sec_level,
				BT_IO_OPT_TIMEOUT, CONNECTION_TIMEOUT,
				BT_IO_OPT_INVALID);
	else
		conn->io = bt_io_connect(BT_IO_L2CAP, io_connect_cb, io_connect_arg, NULL, &err,
				BT_IO_OPT_SOURCE_BDADDR, &sba,
				BT_IO_OPT_DEST_BDADDR, &dba,
				BT_IO_OPT_PSM, psm,
				BT_IO_OPT_IMTU, mtu,
				BT_IO_OPT_SEC_LEVEL, sec_level,
				BT_IO_OPT_TIMEOUT, CONNECTION_TIMEOUT,
				BT_IO_OPT_INVALID);

	if (err) {
		fprintf(stderr, "%s\n", err->message);
		g_error_free(err);
		return NULL;
	} else {
		return conn;
	}
}

gatt_connection_t *gattlib_connect_async(const gchar *src, const gchar *dst,
				uint8_t dest_type, BtIOSecLevel sec_level, int psm, int mtu,
				gatt_connect_cb_t connect_cb)
{
	io_connect_arg_t* io_connect_arg = malloc(sizeof(io_connect_arg_t));

	return initialize_gattlib_connection(src, dst, dest_type, sec_level,
			psm, mtu, connect_cb, io_connect_arg);
}

static gboolean connection_timeout(gpointer user_data) {
	io_connect_arg_t* io_connect_arg = user_data;

	io_connect_arg->timeout = TRUE;

	return FALSE;
}

/**
 * @param src		Local Adaptater interface
 * @param dst		Remote Bluetooth address
 * @param dst_type	Set LE address type (either BDADDR_LE_PUBLIC or BDADDR_LE_RANDOM)
 * @param sec_level	Set security level (either BT_IO_SEC_LOW, BT_IO_SEC_MEDIUM, BT_IO_SEC_HIGH)
 * @param psm       Specify the PSM for GATT/ATT over BR/EDR
 * @param mtu       Specify the MTU size
 */
gatt_connection_t *gattlib_connect(const gchar *src, const gchar *dst,
				uint8_t dest_type, BtIOSecLevel sec_level, int psm, int mtu)
{
	printf("gattlib_connect\n");
	io_connect_arg_t io_connect_arg;
	GSource* timeout;

	gatt_connection_t *conn = initialize_gattlib_connection(src, dst, dest_type, sec_level,
			psm, mtu, NULL, &io_connect_arg);
	if (conn == NULL) {
		if (io_connect_arg.error) {
			fprintf(stderr, "Error: gattlib_connect - initialization error:%s\n", io_connect_arg.error->message);
		} else {
			fprintf(stderr, "Error: gattlib_connect - initialization\n");
		}
		return NULL;
	}

	// Timeout of 'CONNECTION_TIMEOUT+4' seconds
	timeout = gattlib_timeout_add_seconds(CONNECTION_TIMEOUT + 4, connection_timeout, &io_connect_arg);

	// Wait for the connection to be done
	while ((io_connect_arg.connected == FALSE) && (io_connect_arg.timeout == FALSE))
	{
		g_main_context_iteration(g_gattlib_thread.loop_context, FALSE);
	}
	printf("gattlib_connect passed wait loop\n");

	// Disconnect the timeout source
	g_source_destroy(timeout);

	if (io_connect_arg.timeout)
	{
		printf("connection timeout\n");
		task_dec();
		return NULL;
	}

	if (io_connect_arg.error)
	{
		fprintf(stderr, "gattlib_connect - connection error:%s\n", io_connect_arg.error->message);
		task_dec();
		return NULL;
	}
	else{ return conn; }
}

int gattlib_disconnect(gatt_connection_t* connection) {
	// Stop the I/O Channel
	GIOStatus status = g_io_channel_shutdown(connection->io, FALSE, NULL);
	assert(status == G_IO_STATUS_NORMAL);
	g_io_channel_unref(connection->io);
	g_attrib_unref(connection->attrib);
	free(connection);
	task_dec();
	return 0;
}

GSource* gattlib_watch_connection_full(GIOChannel* io, GIOCondition condition,
								 GIOFunc func, gpointer user_data, GDestroyNotify notify)
{
	// Create a main loop source
	GSource *source = g_io_create_watch (io, condition);
	assert(source != NULL);

	g_source_set_callback (source, (GSourceFunc)func, user_data, notify);

	// Attaches it to the main loop context
	guint id = g_source_attach(source, g_gattlib_thread.loop_context);
	g_source_unref (source);
	assert(id != 0);

	return source;
}

GSource* gattlib_timeout_add_seconds(guint interval, GSourceFunc function, gpointer data) {
	GSource *source = g_timeout_source_new_seconds(interval);
	assert(source != NULL);

	g_source_set_callback(source, function, data, NULL);

	// Attaches it to the main loop context
	guint id = g_source_attach(source, g_gattlib_thread.loop_context);
	g_source_unref (source);
	assert(id != 0);

	return source;
}

void gattlib_register_notification(gatt_connection_t* connection, gattlib_event_handler_t notification_handler, void* user_data) {
	connection->notification_handler = notification_handler;
	connection->notification_user_data = user_data;
}

void gattlib_register_indication(gatt_connection_t* connection, gattlib_event_handler_t indication_handler, void* user_data) {
	connection->indication_handler = indication_handler;
	connection->indication_user_data = user_data;
}

int gattlib_uuid_to_string(const bt_uuid_t *uuid, char *str, size_t n) {
	return bt_uuid_to_string(uuid, str, n);
}

int gattlib_string_to_uuid(bt_uuid_t *uuid, const char *str) {
	return bt_string_to_uuid(uuid, str);
}
