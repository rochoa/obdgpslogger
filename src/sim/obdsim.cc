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

#ifdef HAVE_BLUETOOTH
#include "bluetoothsimport.h"
#endif //HAVE_BLUETOOTH


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
#ifdef OBDSIMGEN_MEGASQUIRT
extern struct obdsim_generator obdsimgen_megasquirt;
#endif //OBDSIMGEN_MEGASQUIRT
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
#ifdef OBDSIMGEN_MEGASQUIRT
	&obdsimgen_megasquirt,
#endif //OBDSIMGEN_MEGASQUIRT
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

/// Hardcode maximum number of ECUs/generators
#define OBDSIM_MAXECUS 6

/// An array of these is created, each for a different ECU
struct obdgen_ecu {
	struct obdsim_generator *simgen; //< The actual data generator
	unsigned int ecu_num; //< The ECU that this will respond as
	char *seed; //< The seed used to create this simgen
	void *dg; //< The generator created by this ecu
};

/// It's a main loop.
/** \param sp the simport handle
    \param elm_version claim to be one of these on reset
    \param elm_device claim to be one of these on AT@1
    \param ecus the obdsim_generators the user has selected
    \param ecucount the number of generators in the stack
*/
void main_loop(OBDSimPort *sp,
	const char *elm_version, const char *elm_device,
	struct obdgen_ecu *ecus, int ecucount);

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

	// If you should open a real device instead of a pty
	char *tty_device = NULL;
#endif //OBDPLATFORM_POSIX

#ifdef HAVE_BLUETOOTH
	// Set if they wanted a bluetooth connection
	int bluetooth_requested = 0;
#endif //HAVE_BLUETOOTH

	// The sim generators
	struct obdgen_ecu ecus[OBDSIM_MAXECUS];
	memset(ecus, 0, sizeof(ecus));
	int ecu_count = 0;

	// Logfilen name
	char *logfile_name = NULL;
	
	// Pretend to be this on ATZ
	char *elm_version = strdup(ELM_VERSION_STRING);

	// Pretend to be this on AT@1
	char *elm_device = strdup(ELM_DEVICE_STRING);

#ifdef OBDPLATFORM_WINDOWS
	// Windows port to open
	char *winport = NULL;
#endif //OBDPLATFORM_WINDOWS
	
	// Iterator/index into ecus
	int current_ecu = 0;

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
			case 'V':
				if(NULL != elm_version) {
					free(elm_version);
				}
				elm_version = strdup(optarg);
				break;
			case 'D':
				if(NULL != elm_device) {
					free(elm_device);
				}
				elm_device = strdup(optarg);
				break;
			case 'v':
				printversion();
				mustexit = 1;
				break;
			case 'g':
				if(OBDSIM_MAXECUS <= current_ecu) {
					fprintf(stderr, "Only support %i ECUs in this build\n", OBDSIM_MAXECUS);
					mustexit = 1;
				} else {
					ecus[current_ecu].simgen = find_generator(optarg);
					ecus[current_ecu].ecu_num = current_ecu; // FIXME - need better naming scheme
					if(NULL == ecus[current_ecu].simgen) {
						fprintf(stderr, "Couldn't find generator \"%s\"\n", optarg);
						mustexit = 1;
					}
					current_ecu++;
				}
				break;
			case 's':
				if(current_ecu == 0) {
					fprintf(stderr, "The seed must come after the generator\n");
					mustexit=1;
				}
				if(NULL != ecus[current_ecu-1].seed) {
					fprintf(stderr, "Already provided a seed for generator %i\n", current_ecu);
					mustexit=1;
				}
				ecus[current_ecu-1].seed = strdup(optarg);
				break;
			case 'q':
				if(NULL != logfile_name) {
					fprintf(stderr, "Warning! Multiple logs specified. Only last one will be used\n");
					free(seedstr);
				}
				logfile_name = strdup(optarg);
				break;
#ifdef HAVE_BLUETOOTH
			case 'b':
				bluetooth_requested = 1;
				break;
#endif //HAVE_BLUETOOTH
#ifdef OBDPLATFORM_POSIX
			case 'o':
				launch_logger = 1;
				break;
			case 'c':
				launch_screen = 1;
				break;
			case 't':
				if(NULL != tty_device) {
					free(tty_device);
				}
				tty_device = strdup(optarg);
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
			default:
				mustexit = 1;
				break;
		}
	}

	ecu_count = current_ecu;

#ifdef OBDPLATFORM_POSIX
	if(launch_logger && launch_screen) {
		fprintf(stderr, "Error: Cannot attach both screen and logger to same sim session\n");
		mustexit = 1;
	}
#endif // OBDPLATFORM_POSIX

	if(0 == ecu_count) {
		ecus[0].simgen = find_generator(DEFAULT_SIMGEN);
		if(NULL == ecus[0].simgen) {
			fprintf(stderr, "Couldn't find default generator \"%s\"\n", DEFAULT_SIMGEN);
			mustexit = 1;
		}
		ecu_count++;
	}

	if(mustexit) return 0;

	void *dg;

	int i;
	for(i=0;i<ecu_count;i++) {
		if(0 != ecus[i].simgen->create(&ecus[i].dg, ecus[i].seed)) {
			fprintf(stderr,"Couldn't initialise data generator \"%s\"\n", ecus[i].simgen->name());
			return 1;
		}
	}

	// The sim port
	OBDSimPort *sp = NULL;

#ifdef HAVE_BLUETOOTH
	if(bluetooth_requested) {
		sp = new BluetoothSimPort();
	} else {
#endif //HAVE_BLUETOOTH

#ifdef OBDPLATFORM_POSIX
		sp = new PosixSimPort(tty_device);
#endif //OBDPLATFORM_POSIX

#ifdef OBDPLATFORM_WINDOWS
		if(NULL == winport) {
			winport = strdup(DEFAULT_WINPORT);
		}
		sp = new WindowsSimPort(winport);
#endif //OBDPLATFORM_WINDOWS

#ifdef HAVE_BLUETOOTH
	}
#endif //HAVE_BLUETOOTH

	if(NULL == sp || !sp->isUsable()) {
		fprintf(stderr,"Error creating virtual port\n");
		return 1;
	}

	if(NULL != logfile_name) {
		sp->startLog(logfile_name);
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
	main_loop(sp, elm_version, elm_device, ecus, ecu_count);

	for(i=0;i<ecu_count;i++) {
		ecus[i].simgen->destroy(ecus[i].dg);
	}

	delete sp;

	if(NULL != logfile_name) {
		free(logfile_name);
	}

	if(NULL != seedstr) {
		free(seedstr);
	}

	if(NULL != elm_version) {
		free(elm_version);
	}

	if(NULL != elm_device) {
		free(elm_device);
	}

#ifdef OBDPLATFORM_WINDOWS
	if(NULL != winport) {
		free(winport);
	}
#endif //OBDPLATFORM_WINDOWS

#ifdef OBDPLATFORM_POSIX
	if(NULL != tty_device) {
		free(tty_device);
	}
#endif //OBDPLATFORM_POSIX

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
		"--baud", "-1",                      // Don't modify baudrate
		"--modifybaud", "-1",                // Don't upgrade baudrate
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

void main_loop(OBDSimPort *sp,
		const char *elm_version, const char *elm_device,
	 	struct obdgen_ecu *ecus, int ecucount) {

	char *line; // Single line from the other end of the device
	char previousline[1024]; // Blank lines mean re-run previous command

	// Elm327 options go here.
	int e_headers = ELM_HEADERS; // Whether to show headers
	int e_spaces = ELM_SPACES; // Whether to show spaces
	int e_echo = ELM_ECHO; // Whether to echo commands
	int e_linefeed = ELM_LINEFEED; // Whether to echo commands
	int e_timeout = ELM_TIMEOUT; // The timeout on requests
	int e_adaptive = ELM_ADAPTIVETIMING; // The timeout on requests

	char *device_identifier = strdup("ChunkyKs");

	const char *newline_cr = "\r";
	const char *newline_crlf = "\r\n";

	sp->setEcho(e_echo);

	int mustexit = 0;
	while(!mustexit) {
		// Begin main loop by idling for OBDSIM_SLEEPTIME ms
		struct timeval starttime; // start time through loop
		struct timeval endtime; // end time through loop
		struct timeval selecttime; // =endtime-starttime [for select()]

		struct timeval timeouttime; // Used anytime we need a simulated timeout

		if(0 != gettimeofday(&starttime,NULL)) {
			perror("Couldn't gettimeofday for sim mainloop starttime");
			break;
		}

		int i;
		for(i=0;i<ecucount;i++) {
			if(NULL != ecus[i].simgen->idle) {
				if(0 != ecus[i].simgen->idle(ecus[i].dg,OBDSIM_SLEEPTIME/(ecucount * 1000))) {
					mustexit = 1;
					break;
				}
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
			line = previousline;
		} else {
			strncpy(previousline, line, sizeof(previousline));
		}

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
			unsigned int atopt_ui; // For hex values, mostly

			char *at_cmd = line + 2;

			for(; ' ' == *at_cmd; at_cmd++) { // Find the first non-space character in the AT command
			}

			if(1 == sscanf(at_cmd, "AT%i", &atopt_i)) {
				if(atopt_i >=0 && atopt_i <= 2) {
					printf("Adaptive Timing: %i\n", atopt_i);
					e_adaptive = atopt_i;
					command_recognised = 1;
					snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
				}
			}

			else if(1 == sscanf(at_cmd, "L%i", &atopt_i)) {
				printf("Linefeed %s\n", atopt_i?"enabled":"disabled");
				e_linefeed = atopt_i;
				command_recognised = 1;
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
			}

			else if(1 == sscanf(at_cmd, "H%i", &atopt_i)) {
				printf("Headers %s\n", atopt_i?"enabled":"disabled");
				e_headers = atopt_i;
				command_recognised = 1;
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
			}

			else if(1 == sscanf(at_cmd, "S%i", &atopt_i)) {
				printf("Spaces %s\n", atopt_i?"enabled":"disabled");
				e_spaces = atopt_i;
				command_recognised = 1;
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
			}

			else if(1 == sscanf(at_cmd, "E%i", &atopt_i)) {
				printf("Echo %s\n", atopt_i?"enabled":"disabled");
				e_echo = atopt_i;
				sp->setEcho(e_echo);
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
				command_recognised = 1;
			}

			else if(1 == sscanf(at_cmd, "ST%i", &atopt_ui)) {
				if(0 == atopt_ui) {
					e_timeout = ELM_TIMEOUT;
				} else {
					e_timeout = 4 * atopt_ui;
				}
				printf("Timeout %i\n", e_timeout);
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
				command_recognised = 1;
			}

			else if(1 == sscanf(at_cmd, "@%x", &atopt_ui)) {
				if(1 == atopt_ui) {
					snprintf(response, sizeof(response), "%s", elm_device);
					command_recognised = 1;
				} else if(2 == atopt_ui) {
					snprintf(response, sizeof(response), "%s", device_identifier);
					command_recognised = 1;
				} else if(3 == atopt_ui) {
					snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
					free(device_identifier);
					char *newid = at_cmd+2;
					while(' ' == *newid) newid++;
					device_identifier = strdup(newid);
					printf("Set device identifier to \"%s\"\n", device_identifier);
					command_recognised = 1;
				}
			}

			else if(0 == strncmp(at_cmd, "RV", 2)) {
				snprintf(response, sizeof(response), "%.1f", 11.8);
				command_recognised = 1;
			}

			else if(0 == strncmp(at_cmd, "DPN", 3)) {
				snprintf(response, sizeof(response), "%s", ELM_PROTOCOL_NUMBER);
				command_recognised = 1;
			} else if(0 == strncmp(at_cmd, "DP", 2)) {
				snprintf(response, sizeof(response), "%s", ELM_PROTOCOL_DESCRIPTION);
				command_recognised = 1;
			}

			else if('I' == at_cmd[0]) {
				snprintf(response, sizeof(response), "%s", elm_version);
				command_recognised = 1;
			}

			else if('Z' == at_cmd[0] || 0 == strncmp(at_cmd, "WS", 2) || 'D' == at_cmd[0]) {

				if('Z' == at_cmd[0]) {
					printf("Reset\n");
					snprintf(response, sizeof(response), "%s", elm_version);
					
					// 10 times the regular timeout, just for want of a number
					timeouttime.tv_sec=0;
					timeouttime.tv_usec=1000l*e_timeout * 10 / (e_adaptive +1);
					select(0,NULL,NULL,NULL,&timeouttime);
				} else if('D' == at_cmd[0]) {
					printf("Defaults\n");
					snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
				} else {
					printf("Warm Start\n");
					snprintf(response, sizeof(response), "%s", elm_version);

					// Wait half as long as a reset
					timeouttime.tv_sec=0;
					timeouttime.tv_usec=1000l*e_timeout * 5 / (e_adaptive +1);
					select(0,NULL,NULL,NULL,&timeouttime);
				}

				e_headers = ELM_HEADERS;
				e_linefeed = ELM_LINEFEED;
				e_timeout = ELM_TIMEOUT;
				e_spaces = ELM_SPACES;
				e_echo = ELM_ECHO;
				sp->setEcho(e_echo);

				command_recognised = 1;
			}


			if(0 == command_recognised) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
			}

			sp->writeData(e_linefeed?newline_crlf:newline_cr);
			sp->writeData(response);
			sp->writeData(e_linefeed?newline_crlf:newline_cr);
			sp->writeData(ELM_PROMPT);

			continue;
		}


		int num_vals_read; // Number of values parsed from the sscanf line
		int vals[3]; // Read up to three vals
		num_vals_read = sscanf(line, "%02x %02x %1x", &vals[0], &vals[1], &vals[2]);

		int responsecount = 0;

		sp->writeData(e_linefeed?newline_crlf:newline_cr);

		if(num_vals_read == 0) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
				sp->writeData(response);
				sp->writeData(e_linefeed?newline_crlf:newline_cr);
				responsecount++;
		} else if(num_vals_read == 1) {
				if(0x03 == vals[0] || 0x04 == vals[0]) {
					// TODO: Error code handling
					snprintf(response, sizeof(response), ELM_NODATA_PROMPT);
				} else {
					snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
				}
				sp->writeData(response);
				sp->writeData(e_linefeed?newline_crlf:newline_cr);
				responsecount++;
		} else { // Num_vals_read == 2 or 3

			struct obdservicecmd *cmd = obdGetCmdForPID(vals[1]);
			if(NULL == cmd) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
				sp->writeData(response);
				sp->writeData(e_linefeed?newline_crlf:newline_cr);
				responsecount++;
			} else if(0x01 != vals[0]) {
				// Eventually, modes other than 1 should move to the generators
				//  but for now, respond NO DATA
				// snprintf(response, sizeof(response), "%s", ELM_NODATA_PROMPT);
				// sp->writeData(e_linefeed?newline_crlf:newline_cr);
				// sp->writeData(response);
			} else {

				// Here's the meat & potatoes of the whole application

				// Success!
				for(i=0;i<ecucount;i++) {
					unsigned int abcd[4];
					int count = ecus[i].simgen->getvalue(ecus[i].dg,
									vals[0], vals[1],
									abcd+0, abcd+1, abcd+2, abcd+3);

					// printf("Returning %i values for %02X %02X\n", count, vals[0], vals[1]);

					if(-1 == count) {
						mustexit = 1;
						break;
					}

					if(0 < count) {
						char header[16];
						if(e_headers) {
							snprintf(header, sizeof(header), "%03X%s%02X%s",
								ecus[i].ecu_num, e_spaces?" ":"",
								count+2, e_spaces?" ":"");
						} else {
							snprintf(header, sizeof(header), "");
						}
						int i;
						snprintf(response, sizeof(response), "%s%02X%s%02X",
									header,
									vals[0]+0x40, e_spaces?" ":"", vals[1]);
						for(i=0;i<count;i++) {
							char shortbuf[10];
							snprintf(shortbuf, sizeof(shortbuf), "%s%02X",
									e_spaces?" ":"", abcd[i]);
							// printf("shortbuf: '%s'   i: %i\n", shortbuf, abcd[i]);
							strcat(response, shortbuf);
						}
						sp->writeData(response);
						sp->writeData(e_linefeed?newline_crlf:newline_cr);
						responsecount++;
					}
				}
			}
		}

		// Don't need a timeout if they specified this optimisation
		if(3 > num_vals_read) {
			timeouttime.tv_sec=0;
			timeouttime.tv_usec=1000l*e_timeout * 5 / (e_adaptive +1);
			select(0,NULL,NULL,NULL,&timeouttime);
		}
		if(0 >= responsecount) {
			sp->writeData(ELM_NODATA_PROMPT);
			sp->writeData(e_linefeed?newline_crlf:newline_cr);
		}
		sp->writeData(ELM_PROMPT);
	}

	free(device_identifier);

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
		"   [-g|--generator=<name of generator> [-s|--seed=<generator-seed>]]\n"
		"   [-q|--logfile=<logfilename to write to>]\n"
		"   [-V|--elm-version=<pretend to be this on ATZ>]\n"
		"   [-D|--elm-device=<pretend to be this on AT@1>]\n"
#ifdef OBDPLATFORM_POSIX
		"   [-o|--launch-logger]\n"
		"   [-c|--launch-screen] [use ctrl-a,k to exit screen]\n"
		"   [-t|--tty-device=<real /dev/ entry to open>]\n"
#endif //OBDPLATFORM_POSIX
#ifdef OBDPLATFORM_WINDOWS
		"   [-w|--com-port=<windows COM port>]\n"
#endif //OBDPLATFORM_WINDOWS
#ifdef HAVE_BLUETOOTH
		"   [-b|--bluetooth]\n"
#endif //HAVE_BLUETOOTH
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


