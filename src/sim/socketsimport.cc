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


#ifdef HAVE_SOCKET

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>

#include "simport.h"
#include "obdsim.h"
#include "socketsimport.h"

SocketSimPort::SocketSimPort(int port) {
	portno = port;
	snprintf(portname, sizeof(portname), "Port %i", portno);

	// Open Socket
	s = socket(AF_INET, SOCK_STREAM, 0);
	if(-1 == s) {
		perror("Couldn't open socket");
		return;
	}

	bzero((char *) &loc_addr, sizeof(loc_addr));
	loc_addr.sin_family=AF_INET;
	loc_addr.sin_addr.s_addr = INADDR_ANY;
	loc_addr.sin_port = htons(portno);

	if(-1 == bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr))) {
		perror("Couldn't bind socket");
		return;
	}

	// put socket into listening mode
	listen(s, 1);

	setUsable(1);
}

int SocketSimPort::tryConnection() {

	if(0 != isConnected()) {
		fprintf(stderr, "Error, cannot wait for socket while still connected\n");
		return -1;
	}

	socklen_t opt = sizeof(rem_addr);

	fd_set select_set; 
	FD_ZERO(&select_set);
	FD_SET(s, &select_set);

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	if(0 < select(FD_SETSIZE, &select_set, NULL, NULL, &tv)) {

		// accept one connection
		fd = accept(s, (struct sockaddr *)&rem_addr, &opt);
		if(-1 == fd) {
			perror("Couldn't accept socket connection");
			return -1;
		}
		connected = 1;
		printf("Socket connected: %s\n", getPort());
		fcntl(fd ,F_SETFL,O_NONBLOCK);

		return fd;
	}

	return 0;
}

SocketSimPort::~SocketSimPort() {
	closeCurrentConnection();
	close(s);
}

void SocketSimPort::closeCurrentConnection() {
	if(isConnected()) {
		close(fd);
	}
	setConnected(0);
}


#endif // HAVE_BLUETOOTH

