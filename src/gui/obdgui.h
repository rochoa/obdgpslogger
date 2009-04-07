/** \file
  \brief Main obd gui entrypoint
*/

#ifndef __OBDGUI_H
#define __OBDGUI_H


enum ui_state {
	UI_STOPPED,
	UI_STARTING,
	UI_STARTED
};

enum trip_state {
	TRIP_NONE,
	TRIP_STARTED,
	TRIP_STOPPED
};

#endif // __OBDGUI_H

