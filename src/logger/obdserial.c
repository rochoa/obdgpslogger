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

/// Handle to the serial log
static FILE *seriallog = NULL;

/// Write to the log
static void appendseriallog(const char *line) {
	if(NULL != seriallog) {
		fprintf(seriallog, "%s", line);
		fflush(seriallog);
	}
}

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
	appendseriallog(retbuf);
}

// Blindly send a command and throw away all data to next prompt
/**
 \param cmd command to send
 \param fd file descriptor
 */
void blindcmd(int fd, const char *cmd) {
	appendseriallog(cmd);
	write(fd,cmd, strlen(cmd));
	readtonextprompt(fd);
}

int openserial(const char *portfilename) {
	struct termios options;
	int fd;

	fprintf(stderr,"Opening serial port %s, this can take a while\n", portfilename);
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
		blindcmd(fd,"0100\r");
		// Disable command echo [elm327]
		blindcmd(fd,"ATE0\r");
		// Don't insert spaces [readability is for ugly bags of mostly water]
		blindcmd(fd,"ATS0\r");

	}
	return fd;
}

void closeserial(int fd) {
	close(fd);
}

int startseriallog(const char *logname) {
	if(NULL == (seriallog = fopen(logname, "w"))) {
		perror("Couldn't open seriallog");
		return 1;
	}
	return 0;
}

void closeseriallog() {
	fflush(seriallog);
	fclose(seriallog);
	seriallog = NULL;
}

enum obd_serial_status getobdbytes(int fd, unsigned int cmd, int numbytes_expected,
	unsigned int *A, unsigned int *B, unsigned int *C, unsigned int *D, int *numbytes_returned) {

	char sendbuf[20]; // Command to send
	int sendbuflen; // Number of bytes in the send buffer

	char retbuf[4096]; // Buffer to store returned stuff
	char *bufptr; // current position in retbuf

	int nbytes; // Number of bytes read
	int tries; // Number of tries so far

	if(0 == numbytes_expected) {
		sendbuflen = snprintf(sendbuf,sizeof(sendbuf),"01%02X\r", cmd);
	} else {
		sendbuflen = snprintf(sendbuf,sizeof(sendbuf),"01%02X%01X\r", cmd, numbytes_expected);
	}

	appendseriallog(sendbuf);
	if(write(fd,sendbuf,sendbuflen) < sendbuflen) {
		return OBD_ERROR;
	}

	bufptr = retbuf;
	while(0 < (nbytes = read(fd,bufptr, retbuf+sizeof(retbuf)-bufptr-1))) {
		bufptr += nbytes;
		if(bufptr[-1] == '>') {
			break;
		}
	}
	appendseriallog(retbuf);

	*bufptr = '\0';

	unsigned int response; // Response. Should always be 0x41
	unsigned int mode; // Mode returned [should be the same as cmd]
	int count; // number of retvals successfully sscanf'd

	count = sscanf(retbuf, "%2x %2x %2x %2x %2x %2x", &response, &mode,
					A,B,C,D);

	if(NULL != strstr(retbuf, "NO DATA")) {
		fprintf(stderr, "OBD reported NO DATA for cmd %02X: %s\n", cmd, retbuf);
		return OBD_NO_DATA;
	}

	if(count <= 2) {
		fprintf(stderr, "Didn't get parsable data back for cmd %02X: %s\n", cmd, retbuf);
		return OBD_UNPARSABLE;
	}
	if(response != 0x41) {
		fprintf(stderr, "Didn't get successful response for cmd %02X: %s\n", cmd, retbuf);
		return OBD_INVALID_RESPONSE;
	}
	if(mode != cmd) {
		fprintf(stderr, "Didn't get returned data we wanted for cmd %02X: %s\n", cmd, retbuf);
		return OBD_INVALID_MODE;
	}

	*numbytes_returned = count-2;
	return OBD_SUCCESS;
}

enum obd_serial_status getobdvalue(int fd, unsigned int cmd, float *ret, int numbytes, OBDConvFunc conv) {
	int numbytes_returned;
	unsigned int fourbytes[4];

	enum obd_serial_status ret_status = getobdbytes(fd, cmd, numbytes,
		&fourbytes[0], &fourbytes[1], &fourbytes[2], &fourbytes[3],
		&numbytes_returned);

	if(OBD_SUCCESS != ret_status) return ret_status;

	if(NULL == conv) {
		int i;
		*ret = 0;
		for(i=0;i<numbytes_returned;i++) {
			*ret = *ret * 256;
			*ret = *ret + fourbytes[i];
		}
	} else {
		*ret = conv(fourbytes[0], fourbytes[1], fourbytes[2], fourbytes[3]);
	}
	return OBD_SUCCESS;
}



