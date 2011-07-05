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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "fdsimport.h"
#include "obdsim.h"
#include "posixsimport.h"

PosixSimPort::PosixSimPort(const char *tty_device) {
	if(NULL != tty_device) {
		fd = open(tty_device, O_RDWR, O_NOCTTY);
		if(-1 == fd) {
			perror("Error opening device");
			return;
		}
		strncpy(portname, tty_device, sizeof(portname));

		struct termios oldtio;
		if(0 != tcgetattr(fd,&oldtio)) {
			perror("tcgetattr tty_device");
			return;
		}
		//bzero(&newtio,sizeof(newtio));

		oldtio.c_cflag |= CS8 | CLOCAL | CREAD; // CBAUD

		oldtio.c_iflag |= IGNPAR;
		oldtio.c_iflag &= ~(ICRNL | IMAXBEL);
#ifdef IUTF8
		oldtio.c_iflag &= ~(IUTF8);
#endif //IUTF8

		oldtio.c_oflag &= ~OPOST;

		oldtio.c_lflag |= ECHOE | ECHOK | ECHOCTL | ECHOKE;
		oldtio.c_lflag &= ~(ECHO | ICANON | ISIG);

		oldtio.c_cc[VEOL]     = '\r';
		// oldtio.c_cc[VEOL2]    = '\n';

		if(0 != cfsetispeed(&oldtio, B9600)) {
			perror("cfsetispeed");
			return;
		}

		if(0 != cfsetospeed(&oldtio, B9600)) {
			perror("cfsetospeed");
			return;
		}

		tcflush(fd,TCIFLUSH);
		if(0 != tcsetattr(fd,TCSANOW,&oldtio)) {
			perror("tcsetattr");
			return;
		}
	} else {
		// Cygwin appears to have posix_openpt in a header, but not
		//   available in libc. But it does have /dev/ptmx that does
		//   the right thing.
#ifdef HAVE_POSIX_OPENPT
		fd = posix_openpt(O_RDWR | O_NOCTTY);
#else
		fd = open("/dev/ptmx",O_RDWR | O_NOCTTY);
#endif //HAVE_POSIX_OPENPT

		if(-1 == fd) {
#ifdef HAVE_POSIX_OPENPT
			perror("Error in posix_openpt");
#else
			perror("Error opening /dev/ptmx");
#endif //HAVE_POSIX_OPENPT
			return;
		}
		grantpt(fd);
		unlockpt(fd);

		struct termios oldtio;
		if(0 != tcgetattr(fd,&oldtio)) {
			perror("tcgetattr openpt warning");
		}
		//bzero(&newtio,sizeof(newtio));

		oldtio.c_cflag = CS8 | CLOCAL | CREAD; // CBAUD
		oldtio.c_iflag = IGNPAR | ICRNL;
		oldtio.c_oflag = 0;
		oldtio.c_lflag = ICANON & (~ECHO);

		oldtio.c_cc[VEOL]     = '\r';
		// oldtio.c_cc[VEOL2]    = 0;     /* '\0' */

		tcflush(fd,TCIFLUSH);
		if(0 != tcsetattr(fd,TCSANOW,&oldtio)) {
			perror("tcsetattr warning");
		}

#ifdef HAVE_PTSNAME_R
		if(0 != ptsname_r(fd, portname, sizeof(portname))) {
			perror("Couldn't get pty slave name");
		}
#else
		char *ptsname_val = ptsname(fd);
		if(NULL == ptsname_val) {
			perror("Couldn't get pty slave name");
		}
		strncpy(portname, ptsname_val, sizeof(portname));
#endif //HAVE_PTSNAME_R

	}

	fcntl(fd,F_SETFL,O_NONBLOCK); // O_NONBLOCK + fdopen/stdio == bad
	setUsable(1);

}

PosixSimPort::~PosixSimPort() {
	close(fd);
}

int PosixSimPort::tryConnection() {
	return 1;
}

void PosixSimPort::closeCurrentConnection() {
}


