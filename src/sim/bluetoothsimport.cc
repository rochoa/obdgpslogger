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


/*
NOTE
To use this on my system and advertise it correctly, I ran
   these commands [substitude the hwaddr of your bt chip]:

sudo rfcomm bind 0 00:02:72:14:41:C4 1
sudo sdptool add SP

*/

#ifdef HAVE_BLUETOOTH

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
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "simport.h"
#include "obdsim.h"
#include "bluetoothsimport.h"

BluetoothSimPort::BluetoothSimPort() {
	// Open Socket
	s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);
	if(-1 == s) {
		perror("Couldn't open socket");
		return;
	}

	// bind socket to port 1 of the first available
	// local bluetooth adapter
	loc_addr.rc_family = AF_BLUETOOTH;
	str2ba("00:00:00:00:00:00", &loc_addr.rc_bdaddr);
	loc_addr.rc_channel = (uint8_t) 1;
	bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

	// put socket into listening mode
	listen(s, 1);

	setUsable(1);
}

BluetoothSimPort::~BluetoothSimPort() {
	closeCurrentConnection();
	close(s);
}

void BluetoothSimPort::closeCurrentConnection() {
	if(isConnected()) {
		close(fd);
	}
	setConnected(0);
}

int BluetoothSimPort::tryConnection() {

	if(0 != isConnected()) {
		fprintf(stderr, "Error, cannot wait for bluetooth while still connected\n");
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
			perror("Couldn't accept bt connection");
			return -1;
		}
		setConnected(1);
		printf("Bluetooth connected: %s\n", getPort());
		fcntl(fd ,F_SETFL,O_NONBLOCK);

		return fd;
	}

	if(isConnected()) {
		ba2str( &rem_addr.rc_bdaddr, portname );
	} else {
		snprintf(portname, sizeof(portname), "Not yet connected");
	}

	return 0;
}


#endif // HAVE_BLUETOOTH

