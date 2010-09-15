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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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
#ifdef OBDSIMGEN_ERROR
extern struct obdsim_generator obdsimgen_error;
#endif //OBDSIMGEN_ERROR

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
	&obdsimgen_gui_fltk,
#endif //OBDSIMGEN_GUI_FLTK
#ifdef OBDSIMGEN_ERROR
	&obdsimgen_error
#endif //OBDSIMGEN_ERROR
};

/// Initialise the variables in an ECU
void obdsim_initialiseecu(struct obdgen_ecu *e) {
	e->simgen = NULL;
	e->ecu_num = 0;
	e->seed = NULL;
	e->lasterrorcount = 0;
	e->ffcount = 0;
	e->dg = 0;
	memset(e->ff, 0, sizeof(e->ff));
}

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

/// Update the freeze frame info for all the ecus
void obdsim_freezeframes(struct obdgen_ecu *ecus, int ecucount);

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
	int i;
	for(i = 0; i<OBDSIM_MAXECUS; i++) {
		obdsim_initialiseecu(&ecus[i]);
	}
	memset(ecus, 0, sizeof(ecus));
	int ecu_count = 0;

	// Logfilen name
	char *logfile_name = NULL;
	
	// Pretend to be this on ATZ
	char *elm_version = strdup(OBDSIM_ELM_VERSION_STRING);

	// Pretend to be this on AT@1
	char *elm_device = strdup(OBDSIM_ELM_DEVICE_STRING);

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
					ecus[current_ecu].ecu_num = current_ecu;
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
		ecus[0].ecu_num = 0;
		ecus[0].simgen = find_generator(DEFAULT_SIMGEN);
		if(NULL == ecus[0].simgen) {
			fprintf(stderr, "Couldn't find default generator \"%s\"\n", DEFAULT_SIMGEN);
			mustexit = 1;
		}
		ecu_count++;
	}

	if(mustexit) return 0;

	
	int initialisation_errors = 0;
	for(i=0;i<ecu_count;i++) {
		if(0 != ecus[i].simgen->create(&ecus[i].dg, ecus[i].seed)) {
			fprintf(stderr,"Couldn't initialise generator \"%s\" using seed \"%s\"\n",
							ecus[i].simgen->name(), ecus[i].seed);
			initialisation_errors++;
		}
		if(initialisation_errors > 0) {
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
		"--enable-optimisations",            // Enable elm optimisations
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
		// To avoid seeing console spam in screen, dump stdout
		int fd;
		close(STDOUT_FILENO);
		if ((fd = open("/dev/null", O_RDWR, 0)) != -1) {
			dup2(fd, STDOUT_FILENO);
		}

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

	int e_autoprotocol = 1; // Whether or not we put the "A" and "Auto, " prefix on DP/DPN
	struct obdiiprotocol *e_protocol = find_obdprotocol(OBDSIM_DEFAULT_PROTOCOLNUM); // Starting protocol

	float e_currentvoltage = 11.8; // The current battery voltage

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

		obdsim_freezeframes(ecus, ecucount);

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
			char atopt_c; // If they pass a character option
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

			else if(1 == sscanf(at_cmd, "SPA%c", &atopt_c) || 1 == sscanf(at_cmd, "SP A%c", &atopt_c) ||
				1 == sscanf(at_cmd, "TPA%c", &atopt_c) || 1 == sscanf(at_cmd, "TP A%c", &atopt_c)) {

				struct obdiiprotocol *newprot = find_obdprotocol(atopt_c);
				if(NULL == newprot) {
					command_recognised = 0;
				} else {
					command_recognised = 1;
					e_autoprotocol = 1;
					e_protocol = newprot;
					snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
				}
			}

			else if(1 == sscanf(at_cmd, "SP%c", &atopt_c) ||
				1 == sscanf(at_cmd, "TP%c", &atopt_c)) {

				struct obdiiprotocol *newprot = find_obdprotocol(atopt_c);
				if(NULL == newprot) {
					command_recognised = 0;
				} else {
					command_recognised = 1;
					e_autoprotocol = 0;
					e_protocol = newprot;
					snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
				}
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

			else if(1 == sscanf(at_cmd, "CV%4i", &atopt_i)) {
				e_currentvoltage = (float)atopt_i/100;
				snprintf(response, sizeof(response), "%s", ELM_OK_PROMPT);
				command_recognised = 1;
			}

			else if(0 == strncmp(at_cmd, "RV", 2)) {
				float delta = (float)rand()/(float)RAND_MAX - 0.5f;
				snprintf(response, sizeof(response), "%.1f", e_currentvoltage+delta);
				command_recognised = 1;
			}

			else if(0 == strncmp(at_cmd, "DPN", 3)) {
				snprintf(response, sizeof(response), "%s%c", e_autoprotocol?"A":"", e_protocol->protocol_num);
				command_recognised = 1;
			} else if(0 == strncmp(at_cmd, "DP", 2)) {
				snprintf(response, sizeof(response), "%s%s", e_autoprotocol?"Auto, ":"", e_protocol->protocol_desc);
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
		num_vals_read = sscanf(line, "%02x %02x %x", &vals[0], &vals[1], &vals[2]);

		int responsecount = 0;

		sp->writeData(e_linefeed?newline_crlf:newline_cr);

		// There has *got* to be a better way to do the following complete mess

		if(num_vals_read <= 0) { // Couldn't parse

			snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
			sp->writeData(response);
			sp->writeData(e_linefeed?newline_crlf:newline_cr);
			responsecount++;
		} else if(num_vals_read == 1) { // Only got one valid thing [assume it's mode]

			if(0x03 == vals[0] || 0x07 == vals[0]) { // Get error codes
				unsigned int errorcodes[20];
				for(i=0;i<ecucount;i++) {
					if(NULL != ecus[i].simgen->geterrorcodes) {
						int errorcount;
						int mil = 0;
						errorcount = ecus[i].simgen->geterrorcodes(ecus[i].dg,
							errorcodes, (sizeof(errorcodes)/sizeof(errorcodes[0]))/2, &mil);

						if(0 == errorcount) continue;

						char header[16] = "\0";
						if(e_headers) {
							render_obdheader(header, sizeof(header), e_protocol, &ecus[i], 7, e_spaces);
						}

						int j;
						for(j=0;j<errorcount;j+=3) {
							char shortbuf[32];
							snprintf(shortbuf, sizeof(shortbuf), "%s%02X%s%02X%s%02X%s%02X%s%02X%s%02X",
									e_spaces?" ":"", errorcodes[j*2],
									e_spaces?" ":"", errorcodes[j*2+1],

									e_spaces?" ":"", (errorcount-j)>1?errorcodes[(j+1)*2]:0x00,
									e_spaces?" ":"", (errorcount-j)>1?errorcodes[(j+1)*2+1]:0x00,

									e_spaces?" ":"", (errorcount-j)>2?errorcodes[(j+2)*2]:0x00,
									e_spaces?" ":"", (errorcount-j)>2?errorcodes[(j+2)*2+1]:0x00
									);
							// printf("shortbuf: '%s'   i: %i\n", shortbuf, abcd[i]);
							snprintf(response, sizeof(response), "%s%02X%s",
									header, 0x43, shortbuf);
							sp->writeData(response);
							sp->writeData(e_linefeed?newline_crlf:newline_cr);
							responsecount++;
						}
					}
				}
			} else if(0x04 == vals[0]) { // Reset error codes
				for(i=0;i<ecucount;i++) {
					if(NULL != ecus[i].simgen->clearerrorcodes) {
						ecus[i].simgen->clearerrorcodes(ecus[i].dg);
					}
				}
				snprintf(response, sizeof(response), ELM_OK_PROMPT);
				sp->writeData(response);
				sp->writeData(e_linefeed?newline_crlf:newline_cr);
				responsecount++;
			} else { // Can't do anything with one value, in modes other three or four
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
				sp->writeData(response);
				sp->writeData(e_linefeed?newline_crlf:newline_cr);
				responsecount++;
			}
		} else { // Two or more vals  mode0x01 => mode,pid[,possible optimisation]
								// mode0x02 => mode,pid,frame

			struct obdservicecmd *cmd = obdGetCmdForPID(vals[1]);
			if(NULL == cmd) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
				sp->writeData(response);
				sp->writeData(e_linefeed?newline_crlf:newline_cr);
				responsecount++;
			} else if(0x02 == vals[0]) {
				// Freeze frame
				for(i=0;i<ecucount;i++) {
					int frame = 0;
					if(num_vals_read > 2) {
						// Third value is the frame
						frame = vals[2];
					}
					if(frame < OBDSIM_MAXFREEZEFRAMES && frame <= ecus[i].ffcount) {
						// Don't understand frames higher than this
						char ffmessage[256] = "\0";
						struct freezeframe *ff = &(ecus[i].ff[frame]);
						int count = ff->valuecount[vals[1]];
						int messagelen = count + 3; // Mode, PID, Frame

						// Or could probably just strncat some or something
						switch(count) {
							case 1:
								snprintf(ffmessage, sizeof(ffmessage),"%02X%s%02X%s%02X%s%02X",
									vals[0], e_spaces?" ":"",
									vals[1], e_spaces?" ":"",
									frame, e_spaces?" ":"",
									ecus[i].ff[frame].values[vals[1]][0]);
								break;
							case 2:
								snprintf(ffmessage, sizeof(ffmessage),"%02X%s%02X%s%02X%s%02X%s%02X",
									vals[0], e_spaces?" ":"",
									vals[1], e_spaces?" ":"",
									frame, e_spaces?" ":"",
									ff->values[vals[1]][0], e_spaces?" ":"",
									ff->values[vals[1]][1]);
								break;
							case 3:
								snprintf(ffmessage, sizeof(ffmessage),"%02X%s%02X%s%02X%s%02X%s%02X%s%02X",
									vals[0], e_spaces?" ":"",
									vals[1], e_spaces?" ":"",
									frame, e_spaces?" ":"",
									ff->values[vals[1]][0], e_spaces?" ":"",
									ff->values[vals[1]][1], e_spaces?" ":"",
									ff->values[vals[1]][2]);
								break;
							case 4:
								snprintf(ffmessage, sizeof(ffmessage),"%02X%s%02X%s%02X%s%02X%s%02X%s%02X%s%02X",
									vals[0], e_spaces?" ":"",
									vals[1], e_spaces?" ":"",
									frame, e_spaces?" ":"",
									ff->values[vals[1]][0], e_spaces?" ":"",
									ff->values[vals[1]][1], e_spaces?" ":"",
									ff->values[vals[1]][2], e_spaces?" ":"",
									ff->values[vals[1]][3]);
								break;
							case 0:
							default:
								// NO DATA
								break;
						}
						if(count > 0) {
							char header[16] = "\0";
							if(e_headers) {
								render_obdheader(header, sizeof(header), e_protocol, &ecus[i], 7, e_spaces);
							}
							snprintf(response, sizeof(response), "%s%s", header, ffmessage);
							sp->writeData(response);
							sp->writeData(e_linefeed?newline_crlf:newline_cr);
							responsecount++;
						}
					}

				}
			} else if(0x01 != vals[0]) {
				// Eventually, modes other than 1 should move to the generators
				//  but for now, respond NO DATA
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
						char header[16] = "\0";
						if(e_headers) {
							render_obdheader(header, sizeof(header), e_protocol, &ecus[i], count+2, e_spaces);
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
		if(0x01 == vals[0] && num_vals_read <= 2) {
			timeouttime.tv_sec=0;
			timeouttime.tv_usec=1000l*e_timeout / (e_adaptive +1);
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

void obdsim_freezeframes(struct obdgen_ecu *ecus, int ecucount) {
	int i;
	for(i=0;i<ecucount;i++) {
		struct obdgen_ecu *e = &ecus[i];

		if(NULL != e->simgen->geterrorcodes) {
			int errorcount;
			int mil;
			errorcount = e->simgen->geterrorcodes(e->dg,
				NULL, 0, &mil);

			if(0 == errorcount) {
				if(e->lasterrorcount > 0) {
					printf("Clearing errors\n");
				}
				e->lasterrorcount = 0;
				e->ffcount = 0;
				continue;
			}

			if(e->lasterrorcount == errorcount) continue;

			// Getting here means there's some new errors
			if(e->ffcount >= OBDSIM_MAXFREEZEFRAMES) {
				/* fprintf(stderr, "Warning: Ran out of Frames for ecu %i (%s)\nOBDSIM_MAXFREEZEFRAMES=%i\n",
								i, e->simgen->name(),
								OBDSIM_MAXFREEZEFRAMES); */
				continue;
			}

			printf("Storing new freezeframe(%i) on ecu %i (%s)\n", e->ffcount, i, e->simgen->name());
			unsigned int j;
			int total_vals = 0;
			for(j=0;j<sizeof(obdcmds_mode1)/sizeof(obdcmds_mode1[0]);j++) {
					e->ff[e->ffcount].valuecount[j] = e->simgen->getvalue(ecus[i].dg,
						0x01, j,
						e->ff[e->ffcount].values[j]+0, e->ff[e->ffcount].values[j]+1,
						e->ff[e->ffcount].values[j]+2, e->ff[e->ffcount].values[j]+3);

					if(e->ff[e->ffcount].valuecount[j] > 0) {
						total_vals++;
					}
			}
			printf("Stored %i vals\n", total_vals);

			e->ffcount++;
			e->lasterrorcount = errorcount;
		}
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
	size_t i;
	if(NULL == gen_name) return NULL;

	for(i=0; i<sizeof(available_generators)/sizeof(available_generators[0]); i++) {
		if(0 == strcmp(gen_name, available_generators[i]->name())) {
			return available_generators[i];
		}
	}
	return NULL;
}

struct obdiiprotocol *find_obdprotocol(const char protocol_num) {
	int i;
	for(i=0; i<sizeof(obdprotocols)/sizeof(obdprotocols[0]); i++) {
		if(protocol_num == obdprotocols[i].protocol_num) {
			return &(obdprotocols[i]);
		}
	}
	return NULL;
}

int render_obdheader(char *buf, size_t buflen, struct obdiiprotocol *proto,
	struct obdgen_ecu *ecu, unsigned int messagelen, int spaces) {

	unsigned int ecuaddress; //< The calculated address of this ecu
	unsigned int segments[4]; //< If the address needs to be split up

	switch(proto->headertype) {
		case OBDHEADER_J1850PWM:
			ecuaddress = ecu->ecu_num + 0x10;
			return snprintf(buf, buflen, "41%s6B%s%02X%s",
				spaces?" ":"",
				spaces?" ":"",
				ecuaddress, spaces?" ":"");
			break;
		case OBDHEADER_J1850VPW: // also ISO 9141-2
			ecuaddress = ecu->ecu_num + 0x10;
			return snprintf(buf, buflen, "48%s6B%s%02X%s",
				spaces?" ":"",
				spaces?" ":"",
				ecuaddress, spaces?" ":"");
			break;
		case OBDHEADER_14230:
			ecuaddress = ecu->ecu_num + 0x10;
			return snprintf(buf, buflen, "%02X%sF1%s%02X%s",
				(unsigned)0x80 | messagelen, spaces?" ":"",
				spaces?" ":"",
				ecuaddress, spaces?" ":"");
			break;
		case OBDHEADER_CAN29:
			ecuaddress = ecu->ecu_num + 0x18DAF110;
			segments[0] = (ecuaddress >> 24) & 0xFF;
			segments[1] = (ecuaddress >> 16) & 0xFF;
			segments[2] = (ecuaddress >> 8) & 0xFF;
			segments[3] = (ecuaddress >> 0) & 0xFF;
			return snprintf(buf, buflen, "%02X%s%02X%s%02X%s%02X%s%02X%s",
				segments[0], spaces?" ":"",
				segments[1], spaces?" ":"",
				segments[2], spaces?" ":"",
				segments[3], spaces?" ":"",
				messagelen, spaces?" ":"");
			break;
		case OBDHEADER_CAN11:
			ecuaddress = ecu->ecu_num + 0x7E8;
			return snprintf(buf, buflen, "%03X%s%02X%s",
				ecuaddress, spaces?" ":"",
				messagelen, spaces?" ":"");
			break;
		case OBDHEADER_NULL:
		default:
			return 0;
			break;
	}
	return snprintf(buf, buflen, "UNKNOWN%s", spaces?" ":"");
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

	size_t i;
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


