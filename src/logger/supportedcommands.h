#ifndef __SUPPORTEDCOMMANDS_H
#define __SUPPORTEDCOMMANDS_H

/// Print the capabilities this device claims
void printobdcapabilities(int obd_serial_port);

/// Get the capabilities this device claims
/** Be sure to pass the return value to freecapabilities when you're done
  \return an opaque type you then pass to iscapabilitysupported
  */
void *getobdcapabilities(int obd_serial_port);

/// Free the values returned from getcapabilities
void freeobdcapabilities(void *caps);

/// Find out if a pid is supported
/** \param caps the value returned by getcapabilities
    \param pid the PID we want to know if it's supported
	\return 1 for "yes", 0 for "no".
 */
int isobdcapabilitysupported(void *caps, const unsigned int pid);

#endif // __SUPPORTEDCOMMANDS_H


