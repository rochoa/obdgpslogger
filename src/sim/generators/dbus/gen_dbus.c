/** \file
 \brief Generate random data
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <dbus/dbus.h>

#include "obdservicecommands.h"

#include "datasource.h"

/* Linked lists woo. O(n) happy fun time. On the other hand, we're not
     gonna be seeing more than, like, 20 values... so who cares? */

/// Mappings and most-recently-seen values are stored in this list
/** DBus messages are expected to come as two numbers; an integer then
   an integer or floating point number. The first is mapped onto a PID
   using this table. The second is the actual value to be passed.
*/
struct dbus_simvals {
	int map_from; //< The first number in the message
	unsigned int pid; //< Is mapped to this OBDII PID
	struct obdservicecmd *pid_cmd; // Cache the cmd for this PID
	float most_recent; //< The most recent value we saw for this PID
	struct dbus_simvals *next; //< Yay linked lists
};

/// This is the void * generator
struct dbus_gen {
        DBusConnection *dbusconn; //< The dbus connection
	struct dbus_simvals *simval_list; //< Head of the linked list
};

/// This is what parses messages
static DBusHandlerResult dbus_simgen_msgfilter
      (DBusConnection *connection, DBusMessage *message, void *gen);
 
/// Find the dbus_simval for this map_from value
struct dbus_simvals *dbus_simgen_findsimval(void *gen, int from);

/// Flush the queue of dbus messages waiting for us
static void dbus_simgen_flushqueue(struct dbus_gen *gen);

const char *dbus_simgen_name() {
	return "DBus";
}

int dbus_simgen_create(void **gen, const char *seed) {
	// Read config file

	char dbusinterface[1024] = "\0"; //< The dbus interface name
	char dbuspath[1024] = "\0"; //< The dbus path
	char dbusbus[1024] = "\0"; //< The dbus bus [system or session]
	char dbusmember[1024] = "\0"; //< The dbus member

	struct dbus_simvals *simval_list = NULL;

	if(NULL == seed || '\0' == *seed) {
		fprintf(stderr, "Must pass a filename to a config file as the seed\n");
		return 1;
	}
	FILE *configfile = fopen(seed, "r");
	if(NULL == configfile) {
		fprintf(stderr,"Couldn't open dbus config file %s\n", seed);
		return 1;
	}

	char line[1024]; // Single line of config file

	int map_from; // Value to map PIDs from
	unsigned int map_to; // Value to map PIDs to

	while(NULL != (fgets(line, sizeof(line), configfile))) {
		if('#' == *line) {
			// Comment
			continue;
		}

		if(0 != sscanf(line, "interface=%1023s", dbusinterface)) {
			printf("DBus Config, interface=%s\n", dbusinterface);
		} else if(0 != sscanf(line, "path=%1023s", dbuspath)) {
			printf("DBus Config, path=%s\n", dbuspath);
		} else if(0 != sscanf(line, "bus=%1023s", dbusbus)) {
			printf("DBus Config, bus=%s\n", dbusbus);
		} else if(0 != sscanf(line, "member=%1023s", dbusmember)) {
			printf("DBus Config, member=%s\n", dbusmember);
		} else if(2 == sscanf(line, "map %i -> %x", &map_from, &map_to)) {
			printf("DBus Config, map %i -> 0x%02X\n", map_from, map_to);

			struct obdservicecmd *cmd = obdGetCmdForPID(map_to);
			if(NULL == cmd) {
				fprintf(stderr, "DBus Config, Cannot find obdservice command for PID %02X\n", map_to);
				continue;
			}

			struct dbus_simvals *v = (struct dbus_simvals *)malloc(sizeof(struct dbus_simvals));
			v->map_from = map_from;
			v->pid = map_to;
			v->pid_cmd = cmd;
			v->most_recent = 0;
			v->next = NULL;
			if(NULL == simval_list) {
				simval_list = v;
			} else {
				v->next = simval_list;
				simval_list = v;
			}
		}
	}

	fclose(configfile);
	// Done reading configfile

	if(NULL == simval_list) {
		fprintf(stderr,"Couldn't find any mappings in DBus config file, cannot continue\n");
		return 1;
	}
	if(0 == strlen(dbusinterface)) {
		fprintf(stderr,"DBus config file must contain interface\n");
		return 1;
	}
	if(0 == strlen(dbuspath)) {
		fprintf(stderr,"DBus config file must contain path\n");
		return 1;
	}
	if(0 == strlen(dbusbus)) {
		fprintf(stderr,"DBus config file must contain bus [system or session]\n");
		return 1;
	}
	if(0 == strlen(dbusmember)) {
		fprintf(stderr,"DBus config file must contain member\n");
		return 1;
	}

	char simgen_match[4096];
	snprintf(simgen_match, sizeof(simgen_match),
		"type='signal',interface='%s',path='%s',member='%s'",
		dbusinterface, dbuspath, dbusmember);


	// Create the void *gen
	struct dbus_gen *g = (struct dbus_gen *)malloc(sizeof(struct dbus_gen));
	if(NULL == g) {
		fprintf(stderr,"Couldn't allocate memory for dbus generator\n");
		return 1;
	}

	// Set up dbus stuff
	DBusConnection *dc;
	DBusError err;

	dbus_error_init(&err);
	dc = dbus_bus_get(0==strcmp(dbusbus, "system")?DBUS_BUS_SYSTEM:DBUS_BUS_SESSION, &err);
	if (NULL == dc) {
		fprintf(stderr, "Error getting dbus %s bus: %s\n", dbusbus, err.message);
		return 1;
	}

	dbus_bus_add_match (dc, simgen_match, &err);
	dbus_connection_add_filter (dc, dbus_simgen_msgfilter, g, NULL);

	// Done setting up dbus


	g->dbusconn = dc;
	g->simval_list = simval_list;

	*gen = g;
	return 0;
}

void dbus_simgen_destroy(void *gen) {
	free(gen);
}

int dbus_simgen_getvalue(void *gen, unsigned int PID, unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D) {
	dbus_simgen_flushqueue(gen);

	if(0x00 == PID) {
		// We're capable of pulling *anything* out of our collective asses!
		*A = 0xFF;
		*B = 0xFF;
		*C = 0xFF;
		*D = 0xFE;
	} else {
		*A = (unsigned int) random();
		*B = (unsigned int) random();
		*C = (unsigned int) random();
		*D = (unsigned int) random();
	}
	return 4;
}

DBusHandlerResult dbus_simgen_msgfilter(DBusConnection *connection,
		DBusMessage *message, void *gen) {

	struct dbus_gen *g = (struct dbus_gen *)gen;

	return DBUS_HANDLER_RESULT_HANDLED;
}

struct dbus_simvals *dbus_simgen_findsimval(void *gen, int from) {
	struct dbus_gen *g = (struct dbus_gen *)gen;
	if(NULL == g) return NULL;

	struct dbus_simvals *s = g->simval_list;

	for(; s!=NULL; s=s->next) {
		if(s->map_from == from) {
			return s;
		}
	}
	return NULL;
}

void dbus_simgen_flushqueue(struct dbus_gen *gen) {
}

// Declare our obdsim_generator. This is pulled in as an extern in obdsim.c
struct obdsim_generator obdsimgen_dbus = {
	dbus_simgen_name,
	dbus_simgen_create,
	dbus_simgen_destroy,
	dbus_simgen_getvalue
};

