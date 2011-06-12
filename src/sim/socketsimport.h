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

#ifndef __SOCKETSIMPORT_H
#define __SOCKETSIMPORT_H

#ifdef HAVE_SOCKET

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <termios.h>
#include <fcntl.h>

#include "simport.h"
#include "fdsimport.h"

/// Base class for virtual ports
class SocketSimPort : public FDSimPort {
public:
	/// Constructor
	SocketSimPort(int port);

	/// Destructor
	virtual ~SocketSimPort();

protected:
	/// Wait for a connection
	virtual int tryConnection();

	/// Two locations
	struct sockaddr_in loc_addr, rem_addr;

	/// The actual socket
	int s;

	/// The port number
	int portno;
};

#endif //  HAVE_SOCKET

#endif //__SOCKETSIMPORT_H

