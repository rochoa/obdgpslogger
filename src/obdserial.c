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


/** \file
 \brief serial stuff
 */

// Code and ideas borrowed in places from
// http://easysw.com/~mike/serial/serial.html

#include "obdserial.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

/// Throw away all data until the next prompt
void readtonextprompt(int fd) {
	char retbuf[4096]; // Buffer to store returned stuff
	char *bufptr = retbuf; // current position in retbuf

	int nbytes; // Number of bytes read
	while(0 < (nbytes = read(fd,bufptr, retbuf+sizeof(retbuf)-bufptr-1))) {
		bufptr += nbytes;
		if(bufptr[-1] == '>') {
			break;
		}
	}
}

// Blindly send a command and throw away all data to next prompt
/**
 \param cmd command to send
 \param fd file descriptor
 */
void blindcmd(int fd, const char *cmd) {
	write(fd,cmd, strlen(cmd));
	readtonextprompt(fd);
}

int openserial(const char *portfilename) {
	struct termios options;
	int fd;

	printf("Opening serial port %s, this can take a while\n", portfilename);
	// MIGHT WANT TO REMOVE O_NDELAY
	fd = open(portfilename, O_RDWR | O_NOCTTY | O_NDELAY);

	if(fd == -1) {
		perror(portfilename);
	} else {
		fcntl(fd, F_SETFL, 0);

		// Get the current options for the port
		tcgetattr(fd, &options);

		options.c_cflag |= (CLOCAL | CREAD);
		options.c_lflag &= !(ICANON | ECHO | ECHOE | ISIG);
		options.c_oflag &= !(OPOST);
		options.c_cc[VMIN] = 0;
		options.c_cc[VTIME] = 100;

		tcsetattr(fd, TCSANOW, &options);


		// Now some churn to get everything up and running.
		readtonextprompt(fd);
		// Do a general cmd that all obd-devices support
		blindcmd(fd,"01 00\r");
		// Disable command echo [elm327]
		blindcmd(fd,"AT E0\r");

	}
	return fd;
}

void closeserial(int fd) {
	close(fd);
}

long getobdvalue(int fd, unsigned int cmd) {
	char sendbuf[20]; // Command to send
	int sendbuflen; // Number of bytes in the send buffer

	char retbuf[4096]; // Buffer to store returned stuff
	char *bufptr; // current position in retbuf

	int nbytes; // Number of bytes read
	int tries; // Number of tries so far

	sendbuflen = snprintf(sendbuf,sizeof(sendbuf),"01 %02X\r", cmd);

	for(tries = 0; tries<3; tries++) {
		if(write(fd,sendbuf,sendbuflen) < sendbuflen) {
			return -1;
		}

		bufptr = retbuf;
		while(0 < (nbytes = read(fd,bufptr, retbuf+sizeof(retbuf)-bufptr-1))) {
			bufptr += nbytes;
			if(bufptr[-1] == '>') {
				break;
			}
		}

		*bufptr = '\0';

		unsigned int response; // Response. Should always be 0x41
		unsigned int mode; // Mode returned [should be the same as cmd]
		unsigned int retvals[4]; // attempt to read up to four returned values from the buffer
		int count; // number of retvals successfully sscanf'd

		count = sscanf(retbuf, "%2x %2x %2x %2x %2x %2x", &response, &mode,
						&retvals[0],&retvals[1],&retvals[2],&retvals[3]);

		if(count <= 2) {
			printf("Didn't get parsable data back for cmd %02X: %s\n", cmd, retbuf);
			continue;
		}
		if(response != 0x41) {
			printf("Didn't get successful response for cmd %02X: %s\n", cmd, retbuf);
			continue;
		}
		if(mode != cmd) {
			printf("Didn't get returned data we wanted for cmd %02X: %s\n", cmd, retbuf);
			continue;
		}

		int i;
		long ret = 0;
		for(i=2;i<count;i++) {
			ret = ret * 256;
			ret = ret + retvals[i-2];
		}
		return ret;
	}
	return -1;
}


