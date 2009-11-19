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

#ifndef __WINDOWSSIMPORT_H
#define __WINDOWSSIMPORT_H

#include "simport.h"
#include "windows.h"

/// Base class for virtual ports
class WindowSimPort : public OBDSimPort {
public:
	/// Constructor
	WindowSimPort();

	/// Destructor
	virtual ~WindowSimPort();

	/// Get a string representing the port as it's exposed
	/** Take a copy if you care - the memory won't stay valid */
	virtual char *getPort();

	/// Read a line from the virtual port
	/** Take a copy if you care - the memory won't stay valid */
	virtual char *readLine();

	/// Write some data to the virtual port
	virtual void writeData(const char *data);

private:
	/// Handle onto the windows virtual port
	HANDLE portHandle;

	/// The portname for getPort();
	char *portname;
};

#endif //__WINDOWSSIMPORT_H

