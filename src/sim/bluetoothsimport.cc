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
	readbuf_pos = 0;
	memset(readbuf, '\0', sizeof(readbuf));
	memset(lastread, '\0', sizeof(lastread));
	memset(portname, '\0', sizeof(portname));

	socklen_t opt = sizeof(rem_addr);

	// Open Socket
	s = socket(AF_BLUETOOTH, SOCK_STREAM, BTPROTO_RFCOMM);

	// bind socket to port 1 of the first available
	// local bluetooth adapter
	loc_addr.rc_family = AF_BLUETOOTH;
	memcpy(&loc_addr.rc_bdaddr, BDADDR_ANY, sizeof(loc_addr.rc_bdaddr));
	loc_addr.rc_channel = (uint8_t) 1;
	bind(s, (struct sockaddr *)&loc_addr, sizeof(loc_addr));

	// put socket into listening mode
	listen(s, 1);

	// accept one connection
	printf("Bluetooth waiting for connection\n");
	fd = accept(s, (struct sockaddr *)&rem_addr, &opt);
	if(-1 == fd) {
		perror("Couldn't accept bt connection");
		return;
	}

	printf("Bluetooth connected\n");

	fcntl(fd ,F_SETFL,O_NONBLOCK);

	mUsable = 1;
}

BluetoothSimPort::~BluetoothSimPort() {
	close(fd);
}

char *BluetoothSimPort::getPort() {
	// UGH UGH. Overflow
	// basnprintf probably does this, but I couldn't find documentation
	ba2str( &rem_addr.rc_bdaddr, portname );

	return portname;
}

char *BluetoothSimPort::readLine() {
	int nbytes; // Number of bytes read
	char *currpos = readbuf + readbuf_pos;
	nbytes = read(fd, currpos, sizeof(readbuf)-readbuf_pos);

	if(0 < nbytes) {
		writeLog(currpos);
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

void BluetoothSimPort::writeData(const char *line, int log) {
	if(log) writeLog(line);
	write(fd, line, strlen(line));
}


#endif // HAVE_BLUETOOTH

