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

