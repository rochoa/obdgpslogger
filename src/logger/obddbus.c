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

	double val = value; // DBus lacks a single float type

	dbus_message_append_args (msg,
					DBUS_TYPE_UINT32, &(cmd->cmdid), // PID
					DBUS_TYPE_DOUBLE, &val, // Actual value
					DBUS_TYPE_STRING, cmd->db_column, // Short name
					DBUS_TYPE_STRING, cmd->human_name, // Long name
					DBUS_TYPE_INVALID // Sentinel
					);

	dbus_message_set_no_reply(msg, TRUE);
	dbus_connection_send(obddbusconn, msg, &serial);
	dbus_message_unref(msg);

}


#endif //HAVE_DBUS

