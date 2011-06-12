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

#ifndef __BLUETOOTHSIMPORT_H
#define __BLUETOOTHSIMPORT_H

#ifdef HAVE_BLUETOOTH

#include <sys/socket.h>
#include <termios.h>
#include <sys/types.h>
#include <fcntl.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/rfcomm.h>

#include "simport.h"
#include "fdsimport.h"

/// Base class for virtual ports
class BluetoothSimPort : public FDSimPort {
public:
	/// Constructor
	BluetoothSimPort();

	/// Destructor
	virtual ~BluetoothSimPort();

private:
	/// Wait for a bluetooth connection
	virtual int tryConnection();

	/// Two locations
	struct sockaddr_rc loc_addr, rem_addr;

	/// The actual socket
	int s;

};

#endif //  HAVE_BLUETOOTH

#endif //__BLUETOOTHSIMPORT_H

