/* Copyright 2011 Gary Briggs

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
  \brief Tools to open the sim port
*/


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "simport.h"
#include "fdsimport.h"
#include "obdsim.h"

FDSimPort::FDSimPort() {
	readbuf_pos = 0;
	setConnected(0);
	memset(readbuf, '\0', sizeof(readbuf));
	memset(lastread, '\0', sizeof(lastread));
	snprintf(portname, sizeof(portname), "Unknown");
	fd = -1;
}

FDSimPort::~FDSimPort() {
}

char *FDSimPort::getPort() {
	return portname;
}

char *FDSimPort::readLine() {
	int nbytes; // Number of bytes read
	char *currpos = readbuf + readbuf_pos;

	if(!isConnected()) {
		int conn = tryConnection();
		setConnected(conn);
		if(0 >= conn) {
			return NULL;
		}
	}

	nbytes = read(fd, currpos, sizeof(readbuf)-readbuf_pos);

	if(-1 == nbytes && errno != EAGAIN) {
		perror("Error reading from fd");
		closeCurrentConnection();
		setConnected(0);
		return NULL;
	}

	if(0 < nbytes) {
		writeLog(currpos, SERIAL_IN);
		if(getEcho()) {
			writeData(currpos, 0);
		}

		// printf("Read %i bytes. strn is now '%s'\n", nbytes, readbuf);
		readbuf_pos += nbytes;
		char *lineend = strstr(readbuf, "\r");
		if(NULL == lineend) { // Just in case
			char *lineend = strstr(readbuf, "\n");
		}

		if(NULL != lineend) {
			int length = lineend - readbuf;
			strncpy(lastread, readbuf, length);
			lastread[length]='\0';

			while(*lineend == '\r' || *lineend == '\n') {
				lineend++;
			}
			memmove(readbuf, lineend, sizeof(readbuf) - (lineend - readbuf));
			readbuf_pos -= (lineend - readbuf);

			return lastread;
		}
	}
	return NULL;
}

void FDSimPort::writeData(const char *line, int log) {
	if(!isConnected()) {
		int conn = tryConnection();
		setConnected(conn);
		if(0 >= conn) {
			return;
		}
	}

	if(log) writeLog(line, SERIAL_OUT);

	int nbytes = write(fd, line, strlen(line));
	if(-1 == nbytes && errno != EAGAIN) {
		perror("Error writing to fd");
		closeCurrentConnection();
		setConnected(0);
	}
}

int FDSimPort::isConnected() {
	return connected;
}

void FDSimPort::setConnected(int yes) {
	connected = yes;
}


