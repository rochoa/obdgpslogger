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

#include <stdio.h>
#include <stdlib.h>

#include "obdservicecommands.h"
#include "supportedcommands.h"
#include "obdserial.h"

/// Opaque structure. Don't ever use it, use iscapabilitysupported()
/* This is likely to change. Don't ever use it.
 Internal note: things existing in this ordered linked list implies
 they're supported  */
struct obdcapabilities {
	unsigned int pid; //< The PID this is for
	struct obdcapabilities *next; //< Linked lists suck.
};

void printobdcapabilities(int obd_serial_port) {
	if(-1 == obd_serial_port) {
		fprintf(stderr, "No capabilities when we can't open serial port\n");
		exit(1);
	}

	printf("Your OBD Device claims to support PIDs:\n");

	struct obdcapabilities *caps = (struct obdcapabilities *)getobdcapabilities(obd_serial_port);
	struct obdcapabilities *currcap;
	for(currcap = caps; currcap = currcap->next; currcap != NULL) {
		if(currcap->pid > sizeof(obdcmds)/sizeof(obdcmds[0])) {
			printf("%02X: unknown\n", currcap->pid);
		} else {
			printf("%02X: %s\n", currcap->pid, obdcmds[currcap->pid].human_name);
		}
	}

	freeobdcapabilities(caps);
}

void *getobdcapabilities(int obd_serial_port) {
	struct obdcapabilities *caps = (struct obdcapabilities *)malloc(sizeof(struct obdcapabilities));
	caps->next = NULL;
	caps->pid = 0x00;
	struct obdcapabilities *curr_cap = caps;

	unsigned int A,B,C,D;
	int bytes_returned;
	enum obd_serial_status cap_status;
	unsigned int current_cmd = 0x00;

	while(1) {

		cap_status = getobdbytes(obd_serial_port, current_cmd, 0,
			&A, &B, &C, &D, &bytes_returned);

		if(OBD_SUCCESS != cap_status || 4 != bytes_returned) {
			fprintf(stderr, "Couldn't get obd bytes for cmd %02X\n", current_cmd);
			return caps;
		}

		unsigned long val;
		val = (unsigned long)A*(256*256*256) + (unsigned long)B*(256*256) + (unsigned long)C*(256) + (unsigned long)D;

		int currbit;
		int c;
		for(c=current_cmd+1, currbit=31 ; currbit>=0 ; currbit--, c++) {
			if(val & ((unsigned long)1<<currbit)) {
				struct obdcapabilities *nextcap = (struct obdcapabilities *)malloc(sizeof(struct obdcapabilities));
				nextcap->next = NULL;
				nextcap->pid = c;
				curr_cap->next = nextcap;
				curr_cap = nextcap;
			}
		}

		if(D&0x01) {
			current_cmd += 0x20;
		} else {
			break;
		}
	}
	return caps;
}

void freeobdcapabilities(void *caps) {
	struct obdcapabilities *freeme = (struct obdcapabilities *)caps;
	do {
		struct obdcapabilities *next = freeme->next;
		free(freeme);
		freeme = next;
	} while(NULL != freeme);
}

int isobdcapabilitysupported(void *caps, const unsigned int pid) {
	struct obdcapabilities *currcap = (struct obdcapabilities *)caps;

	for(; currcap = currcap->next; currcap != NULL) {
		if(pid == currcap->pid) return 1;
		if(pid < currcap->pid) break; // They're stored in order
	}
	return 0;
}

