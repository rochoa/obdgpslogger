/* Copyright 2009 Gary Briggs

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
 \brief serial stuff
 */

#ifndef __OBDSERIAL_H
#define __OBDSERIAL_H


/// Open the serial port and set appropriate options
/**
 \param portfilename path and filename of the serial port
 \return fd on success or -1 on error
 */
int openserial(const char *portfilename);

/// Close the serialport
void closeserial(int fd);

/// Get an OBD value
/** This sends "01 cmd-in-hex" to the OBD device.
 It then grabs all the data returned and does its best to put it into the return value
 \param fd the serial port opened with openserial
 \param cmd the obd service command
 \return the value returned by the obd device, or -1 on failure
 */
long getobdvalue(int fd, unsigned int cmd);

#endif // __OBDSERIAL_H

