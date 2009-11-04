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
  \brief Tools to open the sim port
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "simport.h"
#include "obdsim.h"

/// Handle to the simport
struct simport_handle {
	int fd; ///< File descriptor
	char readbuf[4096]; ///< current char buf [when reading]
	char lastread[4096]; ///< Last line read
	int readbuf_pos; ///< Current position in the read buffer
	int enable_echo; ///< Set to echo things [ATE{0,1}]
};

void *simport_open() {
	struct simport_handle *simp = (struct simport_handle *)malloc(sizeof(struct simport_handle));

	if(NULL == simp) return NULL;

	simp->readbuf_pos = 0;
	simp->enable_echo = 1;
	memset(simp->readbuf, '\0', sizeof(simp->readbuf));
	memset(simp->lastread, '\0', sizeof(simp->lastread));

	// Cygwin appears to have posix_openpt in a header, but not
	//   available in libc. But it does have /dev/ptmx that does
	//   the right thing.
#ifdef HAVE_POSIX_OPENPT
	simp->fd = posix_openpt(O_RDWR | O_NOCTTY);
#else
	simp->fd = open("/dev/ptmx",O_RDWR | O_NOCTTY);
#endif //HAVE_POSIX_OPENPT

	if(-1 == simp->fd) {
#ifdef HAVE_POSIX_OPENPT
			perror("Error in posix_openpt");
#else
			perror("Error opening /dev/ptmx");
#endif //HAVE_POSIX_OPENPT
			free(simp);
			return NULL;
	}
	grantpt(simp->fd);
	unlockpt(simp->fd);

	struct termios oldtio;
	tcgetattr(simp->fd,&oldtio);
	//bzero(&newtio,sizeof(newtio));

	oldtio.c_cflag = CS8 | CLOCAL | CREAD; // CBAUD
	oldtio.c_iflag = IGNPAR | ICRNL;
	oldtio.c_oflag = 0;
	oldtio.c_lflag = ICANON & (~ECHO);
        
	oldtio.c_cc[VEOL]     = '\r';
	// oldtio.c_cc[VEOL2]    = 0;     /* '\0' */

	tcflush(simp->fd,TCIFLUSH);
	tcsetattr(simp->fd,TCSANOW,&oldtio);
	fcntl(simp->fd,F_SETFL,O_NONBLOCK); // O_NONBLOCK + fdopen/stdio == bad

	return (void *)simp;
}

void simport_echo(void *simport, int enableecho) {
	struct simport_handle *simp = (struct simport_handle *)simport;
	if(NULL == simp) return;

	simp->enable_echo = enableecho;
}

void simport_close(void *simport) {
	struct simport_handle *simp = (struct simport_handle *)simport;
	if(NULL == simp) return;

	close(simp->fd);
	free(simp);
}

char *simport_getptyslave(void *simport) {
	struct simport_handle *simp = (struct simport_handle *)simport;
	if(NULL == simp) return NULL;

#ifdef HAVE_PTSNAME_R
	static char buf[1024];
	if(0 != ptsname_r(simp->fd, buf, sizeof(buf))) {
		perror("Couldn't get pty slave");
		return NULL;
	}
	return buf;
#else
	return ptsname(simp->fd);
#endif //HAVE_PTSNAME_R
}

char *simport_readline(void *simport) {
	struct simport_handle *simp = (struct simport_handle *)simport;
	if(NULL == simp) return NULL;

	int nbytes; // Number of bytes read
	char *currpos = simp->readbuf + simp->readbuf_pos;
	nbytes = read(simp->fd, currpos, sizeof(simp->readbuf)-simp->readbuf_pos);

	if(0 < nbytes) {
		if(simp->enable_echo) {
			simport_writeline(simp, currpos);
		}

		// printf("Read %i bytes. strn is now '%s'\n", nbytes, simp->readbuf);
		simp->readbuf_pos += nbytes;
		char *lineend = strstr(simp->readbuf, "\r");
		if(NULL == lineend) { // Just in case
			char *lineend = strstr(simp->readbuf, "\n");
		}

		if(NULL != lineend) {
			int length = lineend - simp->readbuf;
			strncpy(simp->lastread, simp->readbuf, length);
			simp->lastread[length]='\0';

			while(*lineend == '\r' || *lineend == '\n') {
				lineend++;
			}
			memmove(simp->readbuf, lineend, sizeof(simp->readbuf) - (lineend - simp->readbuf));
			simp->readbuf_pos -= (lineend - simp->readbuf);

			return simp->lastread;
		}
	}
	return NULL;
}

void simport_writeline(void *simport, const char *line) {
	struct simport_handle *simp = (struct simport_handle *)simport;
	if(NULL == simp) return;

	write(simp->fd, line, strlen(line));
}



