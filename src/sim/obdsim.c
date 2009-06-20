/** \file
 \brief OBD Simulator Main Entrypoint
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <unistd.h>
#include <ctype.h>

#include "obdconfig.h"
#include "obdservicecommands.h"

#include "obdsim.h"
#include "simport.h"
#include "datasource.h"

// Adding your plugin involves two edits here.
// First, add an extern like the others
// Second, add it to the available_generators immediately after

#ifdef OBDSIMGEN_RANDOM
extern struct obdsim_generator obdsimgen_random;
#endif //OBDSIMGEN_RANDOM
#ifdef OBDSIMGEN_LOGGER
extern struct obdsim_generator obdsimgen_logger;
#endif //OBDSIMGEN_LOGGER
#ifdef OBDSIMGEN_DBUS
extern struct obdsim_generator obdsimgen_dbus;
#endif //OBDSIMGEN_DBUS

/// A list of all available generators in this build
static struct obdsim_generator *available_generators[] = {
#ifdef OBDSIMGEN_RANDOM
	&obdsimgen_random,
#endif //OBDSIMGEN_RANDOM
#ifdef OBDSIMGEN_LOGGER
	&obdsimgen_logger,
#endif //OBDSIMGEN_LOGGER
#ifdef OBDSIMGEN_DBUS
	&obdsimgen_dbus
#endif //OBDSIMGEN_DBUS
};

/// Default sim generator
#define DEFAULT_SIMGEN "Logger"

/// It's a main loop.
/** \param sp the simport handle
    \param dg the data generator's void *
	\param simgen the obdsim_generator the user has selected
*/
void main_loop(void *sp, void *dg, struct obdsim_generator *simgen);

/// Launch obdgpslogger connected to the pty
int spawnlogger(char *ptyname);

/// Find the generator of the given name
static struct obdsim_generator *find_generator(const char *gen_name);

/// Print the genrators this was linked with
void printgenerator();

int main(int argc, char **argv) {
	// The "seed" passed in. Generator-specific
	char *seedstr = NULL;

	// Whether to launch obdgpslogger attached to this sim
	int launch_logger = 0;

	// Choice of generator
	char *gen_choice = NULL;

	int optc;
	int mustexit = 0;
	while ((optc = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				printhelp(argv[0]);
				printgenerator();
				mustexit = 1;
				break;
			case 'v':
				printversion();
				mustexit = 1;
				break;
			case 's':
				if(NULL != seedstr) {
					free(seedstr);
				}
				seedstr = strdup(optarg);
				break;
			case 'o':
				launch_logger = 1;
				break;
			case 'g':
				if(NULL != gen_choice) {
					free(gen_choice);
				}
				gen_choice = strdup(optarg);
				break;
			default:
				mustexit = 1;
				break;
		}
	}


	if(mustexit) return 0;

	if(NULL == gen_choice) {
		gen_choice = strdup(DEFAULT_SIMGEN);
	}

	struct obdsim_generator *sim_gen = find_generator(gen_choice);
	if(NULL == sim_gen) {
		fprintf(stderr, "Couldn't find generator \"%s\"\n", gen_choice);
		return 1;
	}

	void *dg;

	if(0 != sim_gen->create(&dg, seedstr)) {
		fprintf(stderr,"Couldn't initialise data generator\n");
		return 1;
	}

	void *sp = simport_open();
	if(NULL == sp) {
		fprintf(stderr,"Couldn't open pseudo terminal master\n");
		return 1;
	}

	char *slave_name = simport_getptyslave(sp);
	if(NULL == slave_name) {
		printf("Couldn't get slave name for pty\n");
		simport_close(sp);
		return 1;
	}

	printf("Slave Name for pty: %s\n", slave_name);

	if(launch_logger) {
		spawnlogger(slave_name);
	}

	main_loop(sp, dg, sim_gen);

	sim_gen->destroy(dg);

	simport_close(sp);

	return 0;
}

int spawnlogger(char *ptyname) {
	int pid = fork();

	if(-1 == pid) {
		perror("Couldn't fork");
		return 1;
	}

	if(0 < pid) {
		return 0;
	}

	// In child
	execlp("obdgpslogger", "obdgpslogger",
		"--serial", ptyname,                 // Connect to this pty
		"--db", "./obdgpsloggertmp.db",      // Dump to this database
		"--serial-log", "./serialcomms.txt", // Log serial comms to this file
		"--spam-stdout",                     // Spam stdout
		NULL);
	perror("Couldn't exec obdgpslogger");
	exit(0);
}

void main_loop(void *sp, void *dg, struct obdsim_generator *simgen) {
	char *line; // Single line from the other end of the device

	// Elm327 options go here.
	int e_headers = ELM_HEADERS; // Whether to show headers
	int e_spaces = ELM_SPACES; // Whether to show spaces
	int e_echo = ELM_ECHO; // Whether to echo commands

	while(1) {
		line = simport_readline(sp); // This is the input line
		char response[1024]; // This is the response

		if(NULL == line || 0 == strlen(line)) continue;

		if(e_echo) {
			simport_writeline(sp, line);
		} else {
			simport_writeline(sp, "\n");
		}

		int i;
		for(i=strlen(line)-1;i>=0;i--) { // Strlen is expensive, kids.
			line[i] = toupper(line[i]);
		}

		printf("Got Line: %s", line);

		// If we recognised the command
		int command_recognised = 0;

		if('A' == line[0] && 'T' == line[1]) {
			// This is an AT command
			int atopt_i; // If they pass an integer option

			char *at_cmd = line + 2;
			for(; ' ' == *at_cmd; at_cmd++) { // Find the first non-space character in the AT command
			}

			if(1 == sscanf(at_cmd, "H%i", &atopt_i)) {
				printf("Headers %s\n", atopt_i?"enabled":"disabled");
				e_headers = atopt_i;
				command_recognised = 1;
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
			}

			if(1 == sscanf(at_cmd, "S%i", &atopt_i)) {
				printf("Spaces %s\n", atopt_i?"enabled":"disabled");
				e_spaces = atopt_i;
				command_recognised = 1;
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
			}

			if(1 == sscanf(at_cmd, "E%i", &atopt_i)) {
				printf("Echo %s\n", atopt_i?"enabled":"disabled");
				e_echo = atopt_i;
				command_recognised = 1;
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
			}

			if('Z' == at_cmd[0]) {
				printf("Reset\n");

				e_headers = ELM_HEADERS;
				e_spaces = ELM_SPACES;
				e_echo = ELM_ECHO;

				command_recognised = 1;
				snprintf(response, sizeof(response), "%s\n>", ELM_VERSION_STRING);
			}

			if(0 == command_recognised) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
			}

			simport_writeline(sp, response);

			continue;
		}


		int num_vals_read; // Number of values parsed from the sscanf line
		int vals[3]; // Read up to three vals
		num_vals_read = sscanf(line, "%02x %02x %1x", &vals[0], &vals[1], &vals[2]);

		if(num_vals_read == 0) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
		} else if(num_vals_read == 1) {
				if(0x04 == vals[1]) {
					// TODO: Unset error code
					snprintf(response, sizeof(response), ">");
				} else {
					snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
				}
		} else { // Num_vals_read == 2 or 3

			struct obdservicecmd *cmd = obdGetCmdForPID(vals[1]);
			if(NULL == cmd) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
			} else {

				// Here's the meat & potatoes of the whole application

				// Success!
				unsigned int A,B,C,D;
				int count = simgen->getvalue(dg, vals[1], &A, &B, &C, &D);

				switch(count) {
					case 1:
						snprintf(response, sizeof(response), "41%s%02X%s%02X\n>",
							e_spaces?" ":"", vals[1],
							e_spaces?" ":"", A);
						break;
					case 2:
						snprintf(response, sizeof(response), "41%s%02X%s%02X%s%02X\n>",
							e_spaces?" ":"", vals[1],
							e_spaces?" ":"", A,
							e_spaces?" ":"", B);
						break;
					case 3:
						snprintf(response, sizeof(response), "41%s%02X%s%02X%s%02X%s%02X\n>",
							e_spaces?" ":"", vals[1],
							e_spaces?" ":"", A,
							e_spaces?" ":"", B,
							e_spaces?" ":"", C);
						break;
					case 4:
						snprintf(response, sizeof(response), "41%s%02X%s%02X%s%02X%s%02X%s%02X\n>",
							e_spaces?" ":"", vals[1],
							e_spaces?" ":"", A,
							e_spaces?" ":"", B,
							e_spaces?" ":"", C,
							e_spaces?" ":"", D);
						break;
					default:
						snprintf(response, sizeof(response), "%s", ELM_NODATA_PROMPT);
						break;
				}
			}
		}

		simport_writeline(sp, response);
	}
}

static struct obdsim_generator *find_generator(const char *gen_name) {
	int i;
	for(i=0; i<sizeof(available_generators)/sizeof(available_generators[0]); i++) {
		if(0 == strcmp(gen_name, available_generators[i]->name())) {
			return available_generators[i];
		}
	}
	return NULL;
}

void printhelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-s|--seed=<generator-specific-string>]\n"
		"   [-g|--generator=<name of generator>]\n"
		"   [-o|--launch-logger]\n"
		"   [-v|--version] [-h|--help]\n", argv0);
}

void printversion() {
	printf("Version: %i.%i\n", OBDLOGGER_MAJOR_VERSION, OBDLOGGER_MINOR_VERSION);
}

void printgenerator() {
	printf("The generators built into this sim:\n");

	// If we find the one currently #defined as default
	int found_default = 0;

	int i;
	for(i=0; i<sizeof(available_generators)/sizeof(available_generators[0]); i++) {
		printf(" \"%s\"", available_generators[i]->name());
		if(0 == strcmp(DEFAULT_SIMGEN, available_generators[i]->name())) {
			printf(" (default)");
			found_default = 1;
		}
		printf("\n");
	}


	if(0 == found_default) {
		printf("No default generator\n");
	}
}


