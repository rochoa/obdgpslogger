/* Copyright 2009-10 Gary Briggs

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
 \brief OBD Simulator Main Loop
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#ifdef OBDPLATFORM_POSIX
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#endif //OBDPLATFORM_POSIX

#ifdef OBDPLATFORM_WINDOWS
#include <windows.h>
#include "windowssimport.h" // Contains implementation of gettimeofday for windows
#endif //OBDPLATFORM_WINDOWS

#include "obdconfig.h"
#include "obdservicecommands.h"

#include "obdsim.h"
#include "simport.h"
#include "datasource.h"
#include "mainloop.h"

void main_loop(OBDSimPort *sp, struct simsettings *ss,
	 	struct obdgen_ecu *ecus, int ecucount) {

	char *line; // Single line from the other end of the device
	char previousline[1024]; // Blank lines mean re-run previous command

	// Benchmarking
	struct timeval benchmarkstart; // Occasionally dump benchmark numbers
	struct timeval benchmarkend; // Occasionally dump benchmark numbers
	int benchmarkcount = 0;
	float benchmarkdelta; // Time between benchmarkstart and benchmarkend

	const char *newline_cr = "\r";
	const char *newline_crlf = "\r\n";

	sp->setEcho(ss->e_echo);

	int mustexit = 0;

	if(0 != gettimeofday(&benchmarkstart,NULL)) {
		fprintf(stderr, "Couldn't gettimeofday for benchmarking\n");
		mustexit = 1;
	}

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


		if(ss->benchmark > 0) {
			if(0 != gettimeofday(&benchmarkend, NULL)) {
				fprintf(stderr, "Couldn't gettimeofday for benchmarking\n");
				break;
			}
			benchmarkdelta = (benchmarkend.tv_sec - benchmarkstart.tv_sec) +
					((float)(benchmarkend.tv_usec - benchmarkstart.tv_usec))/1000000.0f;
			if(ss->benchmark < benchmarkdelta) {
				printf("%i samples in %f seconds. %0.2f samples/sec\n",
					benchmarkcount, benchmarkdelta,
					(float)benchmarkcount/benchmarkdelta);
				gettimeofday(&benchmarkstart,NULL);
				benchmarkcount = 0;
			}
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

			command_recognised = parse_ATcmd(ss,sp,line,response,sizeof(response));

			if(0 == command_recognised) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
			}

			sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
			sp->writeData(response);
			sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
			sp->writeData(ELM_PROMPT);

			continue;
		}


		int num_vals_read; // Number of values parsed from the sscanf line
		int vals[3]; // Read up to three vals
		num_vals_read = sscanf(line, "%02x %02x %x", &vals[0], &vals[1], &vals[2]);

		int responsecount = 0;

		sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);

		// There has *got* to be a better way to do the following complete mess

		if(num_vals_read <= 0) { // Couldn't parse

			snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
			sp->writeData(response);
			sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
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
						if(ss->e_headers) {
							render_obdheader(header, sizeof(header), ss->e_protocol, &ecus[i], 7, ss->e_spaces);
						}

						int j;
						for(j=0;j<errorcount;j+=3) {
							char shortbuf[32];
							snprintf(shortbuf, sizeof(shortbuf), "%s%02X%s%02X%s%02X%s%02X%s%02X%s%02X",
									ss->e_spaces?" ":"", errorcodes[j*2],
									ss->e_spaces?" ":"", errorcodes[j*2+1],

									ss->e_spaces?" ":"", (errorcount-j)>1?errorcodes[(j+1)*2]:0x00,
									ss->e_spaces?" ":"", (errorcount-j)>1?errorcodes[(j+1)*2+1]:0x00,

									ss->e_spaces?" ":"", (errorcount-j)>2?errorcodes[(j+2)*2]:0x00,
									ss->e_spaces?" ":"", (errorcount-j)>2?errorcodes[(j+2)*2+1]:0x00
									);
							// printf("shortbuf: '%s'   i: %i\n", shortbuf, abcd[i]);
							snprintf(response, sizeof(response), "%s%02X%s",
									header, 0x43, shortbuf);
							sp->writeData(response);
							sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
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
				sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
				responsecount++;
			} else { // Can't do anything with one value, in modes other three or four
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
				sp->writeData(response);
				sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
				responsecount++;
			}
		} else { // Two or more vals  mode0x01 => mode,pid[,possible optimisation]
								// mode0x02 => mode,pid,frame

			struct obdservicecmd *cmd = obdGetCmdForPID(vals[1]);
			if(NULL == cmd) {
				snprintf(response, sizeof(response), "%s", ELM_QUERY_PROMPT);
				sp->writeData(response);
				sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
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
									vals[0], ss->e_spaces?" ":"",
									vals[1], ss->e_spaces?" ":"",
									frame, ss->e_spaces?" ":"",
									ecus[i].ff[frame].values[vals[1]][0]);
								break;
							case 2:
								snprintf(ffmessage, sizeof(ffmessage),"%02X%s%02X%s%02X%s%02X%s%02X",
									vals[0], ss->e_spaces?" ":"",
									vals[1], ss->e_spaces?" ":"",
									frame, ss->e_spaces?" ":"",
									ff->values[vals[1]][0], ss->e_spaces?" ":"",
									ff->values[vals[1]][1]);
								break;
							case 3:
								snprintf(ffmessage, sizeof(ffmessage),"%02X%s%02X%s%02X%s%02X%s%02X%s%02X",
									vals[0], ss->e_spaces?" ":"",
									vals[1], ss->e_spaces?" ":"",
									frame, ss->e_spaces?" ":"",
									ff->values[vals[1]][0], ss->e_spaces?" ":"",
									ff->values[vals[1]][1], ss->e_spaces?" ":"",
									ff->values[vals[1]][2]);
								break;
							case 4:
								snprintf(ffmessage, sizeof(ffmessage),"%02X%s%02X%s%02X%s%02X%s%02X%s%02X%s%02X",
									vals[0], ss->e_spaces?" ":"",
									vals[1], ss->e_spaces?" ":"",
									frame, ss->e_spaces?" ":"",
									ff->values[vals[1]][0], ss->e_spaces?" ":"",
									ff->values[vals[1]][1], ss->e_spaces?" ":"",
									ff->values[vals[1]][2], ss->e_spaces?" ":"",
									ff->values[vals[1]][3]);
								break;
							case 0:
							default:
								// NO DATA
								break;
						}
						if(count > 0) {
							char header[16] = "\0";
							if(ss->e_headers) {
								render_obdheader(header, sizeof(header), ss->e_protocol, &ecus[i], 7, ss->e_spaces);
							}
							snprintf(response, sizeof(response), "%s%s", header, ffmessage);
							sp->writeData(response);
							sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
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
					// fprintf(stderr, "ecu %i count %i\n", i, count);

					// printf("Returning %i values for %02X %02X\n", count, vals[0], vals[1]);

					if(-1 == count) {
						mustexit = 1;
						break;
					}

					if(0 < count) {
						char header[16] = "\0";
						if(ss->e_headers) {
							render_obdheader(header, sizeof(header), ss->e_protocol, &ecus[i], count+2, ss->e_spaces);
						}
						int j;
						snprintf(response, sizeof(response), "%s%02X%s%02X",
									header,
									vals[0]+0x40, ss->e_spaces?" ":"", vals[1]);
						for(j=0;j<count;j++) {
							char shortbuf[10];
							snprintf(shortbuf, sizeof(shortbuf), "%s%02X",
									ss->e_spaces?" ":"", abcd[j]);
							// printf("shortbuf: '%s'   j: %i\n", shortbuf, abcd[j]);
							strcat(response, shortbuf);
						}
						sp->writeData(response);
						sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
						responsecount++;
					}
				}
			}
		}

		// Don't need a timeout if they specified this optimisation
		if(0x01 == vals[0] && num_vals_read <= 2) {
			timeouttime.tv_sec=0;
			timeouttime.tv_usec=1000l*ss->e_timeout / (ss->e_adaptive +1);
			select(0,NULL,NULL,NULL,&timeouttime);
		}
		if(0 >= responsecount) {
			sp->writeData(ELM_NODATA_PROMPT);
			sp->writeData(ss->e_linefeed?newline_crlf:newline_cr);
		} else {
			benchmarkcount++;
		}
		sp->writeData(ELM_PROMPT);
	}

	free(ss->device_identifier);

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

int parse_ATcmd(struct simsettings *ss, OBDSimPort *sp, char *line, char *response, size_t n) {
	// This is an AT command

	struct timeval timeouttime; // Used anytime we need a simulated timeout

	int atopt_i; // If they pass an integer option
	char atopt_c; // If they pass a character option
	unsigned int atopt_ui; // For hex values, mostly

	char *at_cmd = line + 2;

	int command_recognised = 0;

	for(; ' ' == *at_cmd; at_cmd++) { // Find the first non-space character in the AT command
	}

	if(1 == sscanf(at_cmd, "AT%i", &atopt_i)) {
		if(atopt_i >=0 && atopt_i <= 2) {
			printf("Adaptive Timing: %i\n", atopt_i);
			ss->e_adaptive = atopt_i;
			command_recognised = 1;
			snprintf(response, n, "%s", ELM_OK_PROMPT);
		}
	}

	else if(1 == sscanf(at_cmd, "L%i", &atopt_i)) {
		printf("Linefeed %s\n", atopt_i?"enabled":"disabled");
		ss->e_linefeed = atopt_i;
		command_recognised = 1;
		snprintf(response, n, "%s", ELM_OK_PROMPT);
	}

	else if(1 == sscanf(at_cmd, "H%i", &atopt_i)) {
		printf("Headers %s\n", atopt_i?"enabled":"disabled");
		ss->e_headers = atopt_i;
		command_recognised = 1;
		snprintf(response, n, "%s", ELM_OK_PROMPT);
	}

	else if(('S' == at_cmd[0] || 'T' == at_cmd[0]) && 'P' == at_cmd[1]) {
		if(0 == set_obdprotocol(at_cmd+2, ss)) {
			command_recognised = 1;
			snprintf(response, n, "%s", ELM_OK_PROMPT);
		}
	}

	else if(1 == sscanf(at_cmd, "S%i", &atopt_i)) {
		printf("Spaces %s\n", atopt_i?"enabled":"disabled");
		ss->e_spaces = atopt_i;
		command_recognised = 1;
		snprintf(response, n, "%s", ELM_OK_PROMPT);
	}

	else if(1 == sscanf(at_cmd, "E%i", &atopt_i)) {
		printf("Echo %s\n", atopt_i?"enabled":"disabled");
		ss->e_echo = atopt_i;
		sp->setEcho(ss->e_echo);
		snprintf(response, n, "%s", ELM_OK_PROMPT);
		command_recognised = 1;
	}

	else if(1 == sscanf(at_cmd, "ST%i", &atopt_ui)) {
		if(0 == atopt_ui) {
			ss->e_timeout = ELM_TIMEOUT;
		} else {
			ss->e_timeout = 4 * atopt_ui;
		}
		printf("Timeout %i\n", ss->e_timeout);
		snprintf(response, n, "%s", ELM_OK_PROMPT);
		command_recognised = 1;
	}

	else if(1 == sscanf(at_cmd, "@%x", &atopt_ui)) {
		if(1 == atopt_ui) {
			snprintf(response, n, "%s", ss->elm_device);
			command_recognised = 1;
		} else if(2 == atopt_ui) {
			snprintf(response, n, "%s", ss->device_identifier);
			command_recognised = 1;
		} else if(3 == atopt_ui) {
			snprintf(response, n, "%s", ELM_OK_PROMPT);
			free(ss->device_identifier);
			char *newid = at_cmd+2;
			while(' ' == *newid) newid++;
			ss->device_identifier = strdup(newid);
			printf("Set device identifier to \"%s\"\n", ss->device_identifier);
			command_recognised = 1;
		}
	}

	else if(1 == sscanf(at_cmd, "CV%4i", &atopt_i)) {
		ss->e_currentvoltage = (float)atopt_i/100;
		snprintf(response, n, "%s", ELM_OK_PROMPT);
		command_recognised = 1;
	}

	else if(0 == strncmp(at_cmd, "RV", 2)) {
		float delta = (float)rand()/(float)RAND_MAX - 0.5f;
		snprintf(response, n, "%.1f", ss->e_currentvoltage+delta);
		command_recognised = 1;
	}

	else if(0 == strncmp(at_cmd, "DPN", 3)) {
		snprintf(response, n, "%s%c", ss->e_autoprotocol?"A":"", ss->e_protocol->protocol_num);
		command_recognised = 1;
	} else if(0 == strncmp(at_cmd, "DP", 2)) {
		snprintf(response, n, "%s%s", ss->e_autoprotocol?"Auto, ":"", ss->e_protocol->protocol_desc);
		command_recognised = 1;
	}

	else if('I' == at_cmd[0]) {
		snprintf(response, n, "%s", ss->elm_version);
		command_recognised = 1;
	}

	else if('Z' == at_cmd[0] || 0 == strncmp(at_cmd, "WS", 2) || 'D' == at_cmd[0]) {

		if('Z' == at_cmd[0]) {
			printf("Reset\n");
			snprintf(response, n, "%s", ss->elm_version);
			
			// 10 times the regular timeout, just for want of a number
			timeouttime.tv_sec=0;
			timeouttime.tv_usec=1000l*ss->e_timeout * 10 / (ss->e_adaptive +1);
			select(0,NULL,NULL,NULL,&timeouttime);
		} else if('D' == at_cmd[0]) {
			printf("Defaults\n");
			snprintf(response, n, "%s", ELM_OK_PROMPT);
		} else {
			printf("Warm Start\n");
			snprintf(response, n, "%s", ss->elm_version);

			// Wait half as long as a reset
			timeouttime.tv_sec=0;
			timeouttime.tv_usec=1000l*ss->e_timeout * 5 / (ss->e_adaptive +1);
			select(0,NULL,NULL,NULL,&timeouttime);
		}

		obdsim_elmreset(ss);
		sp->setEcho(ss->e_echo);

		command_recognised = 1;
	}

	return command_recognised;
}
