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

/// What to use as the obd newline char in commands
#define OBDCMD_NEWLINE "\r"

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

int openserial(const char *portfilename, long baudrate) {
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

		if(0 != modifybaud(fd, baudrate)) {
			fprintf(stderr, "Error modifying baudrate. Attempting to continue, but may suffer issues\n");
		}

		// Now some churn to get everything up and running.
		blindcmd(fd,"" OBDCMD_NEWLINE);
		// Reset the device. Some software changes settings and then leaves it
		blindcmd(fd,"ATZ" OBDCMD_NEWLINE);
		// Do a general cmd that all obd-devices support
		blindcmd(fd,"0100" OBDCMD_NEWLINE);
		// Disable command echo [elm327]
		blindcmd(fd,"ATE0" OBDCMD_NEWLINE);
		// Don't insert spaces [readability is for ugly bags of mostly water]
		blindcmd(fd,"ATS0" OBDCMD_NEWLINE);

	}
	return fd;
}

void closeserial(int fd) {
	blindcmd(fd,"ATZ" OBDCMD_NEWLINE);
	close(fd);
}

int modifybaud(int fd, long baudrate) {
	if(baudrate == -1) return -1;

	struct termios options;
	if(0 != tcgetattr(fd, &options)) {
		perror("tcgetattr");
		return -1;
	}

	speed_t speedreq = B38400;

	switch(baudrate) {
		case 38400:
			speedreq = B38400;
			break;
		case 19200:
			speedreq = B19200;
			break;
		case 9600:
			speedreq = B9600;
			break;
		case 4800:
			speedreq = B4800;
			break;
		case 2400:
			speedreq = B2400;
			break;
		case 1200:
			speedreq = B1200;
			break;
		case 600:
			speedreq = B600;
			break;
		case 300:
			speedreq = B300;
			break;
		case 150:
			speedreq = B150;
			break;
		case 134:
			speedreq = B134;
			break;
		case 110:
			speedreq = B110;
			break;
		case 75:
			speedreq = B75;
			break;
		case 50:
			speedreq = B50;
			break;
		case 0: // Don't look at me like *I* think it's a good idea
			speedreq = B0;
			break;
		default:
			fprintf(stderr,"Uknown baudrate: %li\n", baudrate);
			return -1;
			break;

	}

	if(0 != cfsetspeed(&options, speedreq)) {
		perror("cfsetspeed");
		return -1;
	}

	if(0 != tcsetattr(fd, TCSANOW, &options)) {
		perror("tcsetattr");
		return -1;
	}

	return 0;
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
	// int tries; // Number of tries so far

	if(0 == numbytes_expected) {
		sendbuflen = snprintf(sendbuf,sizeof(sendbuf),"01%02X" OBDCMD_NEWLINE, cmd);
	} else {
		sendbuflen = snprintf(sendbuf,sizeof(sendbuf),"01%02X%01X" OBDCMD_NEWLINE, cmd, numbytes_expected);
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

	if(NULL != strstr(retbuf, "UNABLE TO CONNECT")) {
		fprintf(stderr, "OBD reported UNABLE TO CONNECT for cmd %02X: %s\n", cmd, retbuf);
		return OBD_UNABLE_TO_CONNECT;
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

int getnumobderrors(int fd) {
	int numbytes_returned;
	unsigned int fourbytes[4];
	
	enum obd_serial_status ret_status = getobdbytes(fd, 0x01, 0,
		&fourbytes[0], &fourbytes[1], &fourbytes[2], &fourbytes[3],
		&numbytes_returned);
	
	if(OBD_SUCCESS != ret_status || 0 == numbytes_returned) return 0;
	
	if(fourbytes[0] > 0) {
		return fourbytes[0] & 0x7F;
	}
	
	return 0;
}

