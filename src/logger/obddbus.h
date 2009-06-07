/** \file
 \brief For sending OBD data via dbus
 */

#ifndef __OBDDBUS_H
#define __OBDDBUS_H

#ifdef HAVE_DBUS

#include <dbus/dbus.h>

/// Initialise dbus
int obdinitialisedbus();

/// Signal that we have found a value for this cmd
void obddbussignalpid(struct obdservicecmd *cmd, float value);


#endif //HAVE_DBUS

#endif // __OBDDBUS_H

