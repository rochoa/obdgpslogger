/* Copyright 2009 Gary Briggs

This file is part of obdgpslogger.

obdgpslogger is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

obdgpslogger is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with obdgpslogger.  If not, see <http://www.gnu.org/licenses/>.
*/

/** \file
 \brief For sending OBD data via dbus
 */
#ifdef HAVE_DBUS

#include <stdio.h>
#include <dbus/dbus.h>
#include "obdservicecommands.h"
#include "obddbus.h"

// Not for you
static DBusConnection* obddbusconn = NULL;

int obdinitialisedbus() {
	DBusError err;

	dbus_error_init(&err);
	obddbusconn = dbus_bus_get(DBUS_BUS_SYSTEM, &err);
	if (obddbusconn == NULL) {
		fprintf(stderr, "Error getting dbus system bus: %s\n", err.message);
		return 1;
	}
	return 0;
}

void obddbussignalpid(struct obdservicecmd *cmd, float value) {
	DBusMessage *msg;
	dbus_uint32_t serial;
	
	if(NULL == obddbusconn) return;

	msg = dbus_message_new_signal("/obd", "org.icculus.obdgpslogger", "value");
	if(NULL == msg) return;

	double val = value; // DBus lacks a single float type

	// printf("Sending msg %u %f\n", cmd->cmdid, value);
	dbus_message_append_args (msg,
					DBUS_TYPE_UINT32, &(cmd->cmdid), // PID
					DBUS_TYPE_DOUBLE, &val, // Actual value
					DBUS_TYPE_STRING, &(cmd->db_column), // Short name
					DBUS_TYPE_STRING, &(cmd->human_name), // Long name
					DBUS_TYPE_INVALID // Sentinel
					);

	dbus_message_set_no_reply(msg, TRUE);
	dbus_connection_send(obddbusconn, msg, &serial);
	dbus_message_unref(msg);

}


#endif //HAVE_DBUS

