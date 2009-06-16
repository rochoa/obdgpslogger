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

#include "sqlite3.h"

/// It's a main loop.
void main_loop(void *sp);

/// Launch obdgpslogger connected to the pty
int spawnlogger(char *ptyname);

int main(int argc, char **argv) {
	char *databasename = NULL;

	int launch_logger = 0;

	int optc;
	int mustexit = 0;
	while ((optc = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (optc) {
			case 'h':
				printhelp(argv[0]);
				mustexit = 1;
				break;
			case 'v':
				printversion();
				mustexit = 1;
				break;
			case 'd':
				if(NULL != databasename) {
					free(databasename);
				}
				databasename = strdup(optarg);
				break;
			case 'o':
				launch_logger = 1;
				break;
			default:
				mustexit = 1;
				break;
		}
	}


	if(mustexit) return 0;


	void *sp = simport_open();
	if(NULL == sp) {
		fprintf(stderr,"Couldn't open pseudo terminal master\n");
		return 1;
	}

	char *slave_name = simport_getptyslave(sp);
	if(NULL == slave_name) {
		printf("Couldn't get slave name for pty\n");
		simport_close(sp);
		return -1;
	}

	printf("Slave Name for pty: %s\n", slave_name);

	if(launch_logger) {
		spawnlogger(slave_name);
	}

	main_loop(sp);

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

void main_loop(void *sp) {
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
				snprintf(response, sizeof(response), "OK\n>");
			}

			if(1 == sscanf(at_cmd, "S%i", &atopt_i)) {
				printf("Spaces %s\n", atopt_i?"enabled":"disabled");
				e_spaces = atopt_i;
				command_recognised = 1;
				snprintf(response, sizeof(response), "OK\n>");
			}

			if(1 == sscanf(at_cmd, "E%i", &atopt_i)) {
				printf("Echo %s\n", atopt_i?"enabled":"disabled");
				e_echo = atopt_i;
				command_recognised = 1;
				snprintf(response, sizeof(response), "OK\n>");
			}

			if('Z' == at_cmd[0]) {
				printf("Reset\n");
				command_recognised = 1;
				snprintf(response, sizeof(response), "%s\n>", ELM_VERSION_STRING);
			}

			if(0 == command_recognised) {
				snprintf(response, sizeof(response), "?\n>");
			}
		} else {
			int num_vals_read; // Number of values parsed from the sscanf line
			int vals[3]; // Read up to three vals
			num_vals_read = sscanf(line, "%02x %02x %1x", &vals[0], &vals[1], &vals[2]);

			switch(num_vals_read) {
				case 1:
					if(0x04 == vals[1]) {
						// TODO: Unset error code
						snprintf(response, sizeof(response), ">");
					} else {
						snprintf(response, sizeof(response), "?\n>");
					}
					break;
				case 2:
				case 3: {
						struct obdservicecmd *cmd = obdGetCmdForPID(vals[1]);
						if(NULL == cmd) {
							snprintf(response, sizeof(response), "?\n>");
							break;
						}

						// Here's the meat & potatoes of the whole application

						// Success!
						snprintf(response, sizeof(response), "41%s%02X%s",
							e_spaces?" ":"", vals[1], e_spaces?" ":"");

						// Values!
						char retvals[1000];
						snprintf(retvals, sizeof(retvals), "%02X%s%02X\n>",
							12, e_spaces?" ":"", 34);

						// TODO: Suffix a real value here

						strcat(response, retvals); // STRCAT BAD MMKAY.
						break;
				}
				case 0:
				default:
					snprintf(response, sizeof(response), "?\n>");
					break;
			}
		}

		simport_writeline(sp, response);
	}
}

void printhelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-d|--db=[" OBD_DEFAULT_DATABASE "]]\n"
		"   [-o|--launch-logger]\n"
		"   [-v|--version] [-h|--help]\n", argv0);
}

void printversion() {
	printf("Version: %i.%i\n", OBDLOGGER_MAJOR_VERSION, OBDLOGGER_MINOR_VERSION);
}


