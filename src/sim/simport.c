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

void *simport_open() {
	// int fd = open("/dev/ptmx",O_RDWR | O_NOCTTY);
	int fd = posix_openpt(O_RDWR | O_NOCTTY);
	if(-1 == fd) return NULL;
	grantpt(fd);
	unlockpt(fd);

	struct termios oldtio;
	tcgetattr(fd,&oldtio);
	//bzero(&newtio,sizeof(newtio));

	oldtio.c_cflag = CS8 | CLOCAL | CREAD; // CBAUD
	oldtio.c_iflag = IGNPAR; // | ICRNL;
	oldtio.c_oflag = 0;
	oldtio.c_lflag = ICANON;
        
	oldtio.c_cc[VEOL]     = '\r';     /* '\0' */
	oldtio.c_cc[VEOL2]    = 0;     /* '\0' */

	tcflush(fd,TCIFLUSH);
	tcsetattr(fd,TCSANOW,&oldtio);
	fcntl(fd,F_SETFL,O_NONBLOCK); // O_NONBLOCK + fdopen/stdio == bad

	FILE *f = fdopen(fd, "r+");

	if(NULL == f) {
		perror("Couldn't upgrade fd to FILE *");
		close(fd);
		return NULL;
	}
	return (void *)f;
}

void simport_close(void *simport) {
	fclose((FILE *)simport);
}

char *simport_getptyslave(void *simport) {
	int fd = fileno((FILE *)simport);

#ifdef HAVE_PTSNAME_R
	static char buf[1024];
	if(0 != ptsname_r(fd, buf, sizeof(buf))) {
		perror("Couldn't get pty slave");
		return NULL;
	}
	return buf;
#else
	return ptsname(fd);
#endif //HAVE_PTSNAME_R
}

char *simport_readline(void *simport) {
	static char buf[1024];
	if(NULL != fgets(buf, sizeof(buf), (FILE *)simport)) {
		return buf;
	}
	return NULL;
}

void simport_writeline(void *simport, const char *line) {
	fprintf((FILE *)simport, "%s", line);
}



