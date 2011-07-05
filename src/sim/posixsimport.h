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

#ifndef __POSIXSIMPORT_H
#define __POSIXSIMPORT_H

#include "simport.h"
#include "fdsimport.h"

/// Base class for virtual ports
class PosixSimPort : public FDSimPort {
public:
	/// Constructor
	/** \param tty_device Pass an actual /dev node and we'll open that instead of a pty
	 */
	PosixSimPort(const char *tty_device);

	/// Destructor
	virtual ~PosixSimPort();

private:
	virtual int tryConnection();
        virtual void closeCurrentConnection();
};

#endif //__POSIXSIMPORT_H

