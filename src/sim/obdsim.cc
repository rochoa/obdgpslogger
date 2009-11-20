/* Copyright 2009 Gary Briggs, Michael Carpenter

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
 \brief OBD Simulator Main Entrypoint
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <ctype.h>

#include "obdconfig.h"
#include "obdservicecommands.h"

#include "obdsim.h"
#include "simport.h"
#include "datasource.h"

#ifdef OBDPLATFORM_POSIX
#include <unistd.h>
#include <sys/time.h>

#include "posixsimport.h"
#endif //OBDPLATFORM_POSIX

#ifdef OBDPLATFORM_WINDOWS
#include <windows.h>

#include "windowssimport.h"
#endif //OBDPLATFORM_WINDOWS


// Adding your plugin involves two edits here.
// First, add an extern like the others
// Second, add it to the available_generators immediately after

#ifdef OBDSIMGEN_RANDOM
extern struct obdsim_generator obdsimgen_random;
#endif //OBDSIMGEN_RANDOM
#ifdef OBDSIMGEN_CYCLE
extern struct obdsim_generator obdsimgen_cycle;
#endif //OBDSIMGEN_CYCLE
#ifdef OBDSIMGEN_LOGGER
extern struct obdsim_generator obdsimgen_logger;
#endif //OBDSIMGEN_LOGGER
#ifdef OBDSIMGEN_DBUS
extern struct obdsim_generator obdsimgen_dbus;
#endif //OBDSIMGEN_DBUS
#ifdef OBDSIMGEN_DLOPEN
extern struct obdsim_generator obdsimgen_dlopen;
#endif //OBDSIMGEN_DLOPEN
#ifdef OBDSIMGEN_GUI_FLTK
extern struct obdsim_generator obdsimgen_gui_fltk;
#endif //OBDSIMGEN_GUI_FLTK
#ifdef OBDSIMGEN_SOCKET
extern struct obdsim_generator obdsimgen_socket;
#endif //OBDSIMGEN_SOCKET

/// A list of all available generators in this build
static struct obdsim_generator *available_generators[] = {
#ifdef OBDSIMGEN_RANDOM
	&obdsimgen_random,
#endif //OBDSIMGEN_RANDOM
#ifdef OBDSIMGEN_CYCLE
	&obdsimgen_cycle,
#endif //OBDSIMGEN_CYCLE
#ifdef OBDSIMGEN_LOGGER
	&obdsimgen_logger,
#endif //OBDSIMGEN_LOGGER
#ifdef OBDSIMGEN_DBUS
	&obdsimgen_dbus,
#endif //OBDSIMGEN_DBUS
#ifdef OBDSIMGEN_DLOPEN
	&obdsimgen_dlopen,
#endif //OBDSIMGEN_DLOPEN
#ifdef OBDSIMGEN_SOCKET
	&obdsimgen_socket,
#endif //OBDSIMGEN_SOCKET
#ifdef OBDSIMGEN_GUI_FLTK
	&obdsimgen_gui_fltk
#endif //OBDSIMGEN_GUI_FLTK
};

/// Default sim generator
#define DEFAULT_SIMGEN "Cycle"

/// Default windows port
#define DEFAULT_WINPORT "CNCA0"

/// Length of time to sleep between nonblocking reads [us]
#define OBDSIM_SLEEPTIME 10000

/// It's a main loop.
/** \param sp the simport handle
    \param dg the data generator's void *
	\param simgen the obdsim_generator the user has selected
*/
void main_loop(OBDSimPort *sp, void *dg, struct obdsim_generator *simgen);

#ifdef OBDPLATFORM_POSIX
/// Launch obdgpslogger connected to the pty
int spawnlogger(char *ptyname);

/// Launch screen connected to the pty
int spawnscreen(char *ptyname);
#endif // OBDPLATFORM_POSIX

/// Find the generator of the given name
static struct obdsim_generator *find_generator(const char *gen_name);

/// Print the long description provided by the generator
void show_genhelp(struct obdsim_generator *gen);

/// Print the genrators this was linked with
void printgenerator(int verbose);

int main(int argc, char **argv) {
	// The "seed" passed in. Generator-specific
	char *seedstr = NULL;

#ifdef OBDPLATFORM_POSIX
	// Whether to launch obdgpslogger attached to this sim
	int launch_logger = 0;

	// Whether to launch screen attached to this sim
	int launch_screen = 0;
#endif //OBDPLATFORM_POSIX

	// Choice of generator
	char *gen_choice = NULL;

	// The sim generator
	struct obdsim_generator *sim_gen;

#ifdef OBDPLATFORM_WINDOWS
	// Windows port to open
	char *winport = NULL;
#endif //OBDPLATFORM_WINDOWS
	
	int optc;
	int mustexit = 0;
	while ((optc = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				printhelp(argv[0]);
				printgenerator(1);
				mustexit = 1;
				break;
			case 'e': {
				struct obdsim_generator *h_gen = find_generator(optarg);
				show_genhelp(h_gen);
				mustexit = 1;
				break;
			}
			case 'l':
				printgenerator(0);
				mustexit = 1;
				break;
			case 'v':
				printversion();
				mustexit = 1;
				break;
			case 's':
				if(NULL != seedstr) {
					fprintf(stderr, "Warning! Multiple seeds specified. Only last one will be used\n");
					free(seedstr);
				}
				seedstr = strdup(optarg);
				break;
#ifdef OBDPLATFORM_POSIX
			case 'o':
				launch_logger = 1;
				break;
			case 'c':
				launch_screen = 1;
				break;
#endif //OBDPLATFORM_POSIX
#ifdef OBDPLATFORM_WINDOWS
			case 'w':
				if(NULL != winport) {
					fprintf(stderr, "Warning! Multiple com port specified. Only last one will be used\n");
					free(winport);
				}
				winport = strdup(optarg);
				break;
#endif //OBDPLATFORM_WINDOWS
			case 'g':
				if(NULL != gen_choice) {
					fprintf(stderr, "Warning! Multiple generators specified. Only last one will be used\n");
					free(gen_choice);
				}
				gen_choice = strdup(optarg);
				sim_gen = find_generator(gen_choice);
				if(NULL == sim_gen) {
					fprintf(stderr, "Couldn't find generator \"%s\"\n", gen_choice);
					mustexit = 1;
				}
				break;
			default:
				mustexit = 1;
				break;
		}
	}

#ifdef OBDPLATFORM_POSIX
	if(launch_logger && launch_screen) {
		fprintf(stderr, "Error: Cannot attach both screen and logger to same sim session\n");
		mustexit = 1;
	}
#endif // OBDPLATFORM_POSIX

	if(NULL == gen_choice) {
		gen_choice = strdup(DEFAULT_SIMGEN);
		sim_gen = find_generator(gen_choice);
		if(NULL == sim_gen) {
			fprintf(stderr, "Couldn't find default generator \"%s\"\n", gen_choice);
			mustexit = 1;
		}
	}

	if(mustexit) return 0;

	void *dg;

	if(0 != sim_gen->create(&dg, seedstr)) {
		fprintf(stderr,"Couldn't initialise data generator \"%s\"\n", gen_choice);
		return 1;
	}

	// The sim port
	OBDSimPort *sp = NULL;

#ifdef OBDPLATFORM_POSIX
	sp = new PosixSimPort();
#endif //OBDPLATFORM_POSIX

#ifdef OBDPLATFORM_WINDOWS
	if(NULL == winport) {
		winport = strdup(DEFAULT_WINPORT);
	}
	sp = new WindowsSimPort(fullwinport);
#endif //OBDPLATFORM_WINDOWS

	if(NULL == sp || !sp->isUsable()) {
		fprintf(stderr,"Error creating virtual port\n");
		return 1;
	}

	char *slave_name = sp->getPort();
	if(NULL == slave_name) {
		printf("Couldn't get slave name for pty\n");
		delete sp;
		return 1;
	}

	printf("SimPort name: %s\n", slave_name);

#ifdef OBDPLATFORM_POSIX
	if(launch_logger) {
		spawnlogger(slave_name);
	}
	if(launch_screen) {
		spawnscreen(slave_name);
	}
#endif //OBDPLATFORM_POSIX

	printf("Successfully initialised obdsim, entering main loop\n");
	main_loop(sp, dg, sim_gen);

	sim_gen->destroy(dg);

	delete sp;

	return 0;
}

#ifdef OBDPLATFORM_POSIX
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

int spawnscreen(char *ptyname) {
	int pid = fork();

	if(-1 == pid) {
		perror("Couldn't fork");
		return 1;
	}

	if(0 < pid) {
		return 0;
	}

	// In child
	execlp("screen", "screen",
		ptyname,                 // Connect to this pty
		NULL);
	perror("Couldn't exec screen");
	exit(0);
}
#endif //OBDPLATFORM_POSIX

void main_loop(OBDSimPort *sp, void *dg, struct obdsim_generator *simgen) {
	char *line; // Single line from the other end of the device

	// Elm327 options go here.
	int e_headers = ELM_HEADERS; // Whether to show headers
	int e_spaces = ELM_SPACES; // Whether to show spaces
	int e_echo = ELM_ECHO; // Whether to echo commands
	sp->setEcho(e_echo);

	int mustexit = 0;
	while(!mustexit) {
		// Begin main loop by idling for OBDSIM_SLEEPTIME ms
		struct timeval starttime; // start time through loop
		struct timeval endtime; // end time through loop
		struct timeval selecttime; // =endtime-starttime [for select()]
		if(0 != gettimeofday(&starttime,NULL)) {
			perror("Couldn't gettimeofday for sim mainloop starttime");
			break;
		}

		if(NULL != simgen->idle) {
			if(0 != simgen->idle(dg,OBDSIM_SLEEPTIME/1000)) {
				mustexit = 1;
				break;
			}
		}

		if(0 != gettimeofday(&endtime,NULL)) {
			perror("Couldn't gettimeofday for sim mainloop endtime");
			break;
		}

		selecttime.tv_sec = endtime.tv_sec - starttime.tv_sec;
		if (selecttime.tv_sec != 0) {
				endtime.tv_usec += 1000000*selecttime.tv_sec;
				selecttime.tv_sec = 0;
		}
		selecttime.tv_usec = (OBDSIM_SLEEPTIME) - (endtime.tv_usec - starttime.tv_usec);
		if(selecttime.tv_usec > 0) {
			select(0,NULL,NULL,NULL,&selecttime);
		}


		// Now the actual choise-response thing
		line = sp->readLine(); // This is the input line
		char response[1024]; // This is the response

		if(NULL == line) continue;
		if(0 == strlen(line)) {
			snprintf(response, sizeof(response), ">");
			sp->writeData(response);
			continue;
		}

		int i;
		for(i=strlen(line)-1;i>=0;i--) { // Strlen is expensive, kids.
			line[i] = toupper(line[i]);
		}

		// printf("obdsim got request: %s\n", line);

		if(NULL != strstr(line, "EXIT")) {
			printf("Received EXIT via serial port. Sim Exiting\n");
			mustexit=1;
			continue;
		}

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
				sp->setEcho(e_echo);
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
			}

			if('Z' == at_cmd[0]) {
				printf("Reset\n");

				e_headers = ELM_HEADERS;
				e_spaces = ELM_SPACES;
				e_echo = ELM_ECHO;

				command_recognised = 1;
				snprintf(response, sizeof(response), ELM_NEWLINE "%s" ELM_NEWLINE ">", ELM_VERSION_STRING);
			}

			if(0 == command_recognised) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
			}

			sp->writeData(response);

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
					snprintf(response, sizeof(response), ELM_PROMPT);
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
				unsigned int abcd[4];
				int count = simgen->getvalue(dg, vals[0], vals[1],
								abcd+0, abcd+1, abcd+2, abcd+3);

				if(-1 == count) {
					mustexit = 1;
					break;
				}

				if(0 == count) {
					snprintf(response, sizeof(response), "%s", ELM_NODATA_PROMPT);
					sp->writeData(response);
					continue;
				}

				int i;
				snprintf(response, sizeof(response), ELM_NEWLINE "%02X%s%02X",
							vals[0]+0x40, e_spaces?" ":"", vals[1]);
				for(i=0;i<count;i++) {
					char shortbuf[10];
					snprintf(shortbuf, sizeof(shortbuf), "%s%02X",
							e_spaces?" ":"", abcd[i]);
					// printf("shortbuf: '%s'   i: %i\n", shortbuf, abcd[i]);
					strcat(response, shortbuf);
				}
				strcat(response, ELM_PROMPT);
			}
		}

		sp->writeData(response);
	}
}

void show_genhelp(struct obdsim_generator *gen) {
	if(NULL == gen) {
		fprintf(stderr, "Cannot print help for nonexistent generator\n");
		return;
	}

	printf("Long help for generator \"%s\":\n\n%s\n\n", gen->name(),
		NULL==gen->longdesc?
			"Generator doesn't offer long description":
			gen->longdesc());
}

static struct obdsim_generator *find_generator(const char *gen_name) {
	int i;
	if(NULL == gen_name) return NULL;

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
#ifdef OBDPLATFORM_POSIX
		"   [-o|--launch-logger]\n"
		"   [-c|--launch-screen] [use ctrl-a,k to exit screen]\n"
#endif //OBDPLATFORM_POSIX
#ifdef OBDPLATFORM_WINDOWS
		"   [-w|--com-port=<windows COM port>]\n"
#endif //OBDPLATFORM_WINDOWS
		"   [-e|--genhelp=<name of generator>]\n"
		"   [-l|--list-generators]\n"
		"   [-v|--version] [-h|--help]\n", argv0);
}

void printversion() {
	printf("Version: %i.%i\n", OBDGPSLOGGER_MAJOR_VERSION, OBDGPSLOGGER_MINOR_VERSION);
}

void printgenerator(int verbose) {
	if(verbose) {
		printf("The generators built into this sim:\n");
	}

	// If we find the one currently #defined as default
	int found_default = 0;

	int i;
	for(i=0; i<sizeof(available_generators)/sizeof(available_generators[0]); i++) {
		int is_default = !strcmp(DEFAULT_SIMGEN, available_generators[i]->name());
		if(is_default) found_default = 1;
		if(verbose) {
			printf(" \"%s\"%s\n", 
				available_generators[i]->name(),
				is_default?" (default)":"");
		} else {
			printf("%s%s\n", 
				is_default?"* ":"  ",
				available_generators[i]->name());
		}
	}


	if(0 == found_default && verbose) {
		printf("No default generator\n");
	}
}


