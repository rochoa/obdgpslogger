/** \file
  \brief Main obd gui entrypoint
*/

#ifndef __OBDGUI_H
#define __OBDGUI_H


/// UI State
enum ui_state {
	UI_STOPPED, ///< OBD GPS Logger not running
	UI_STARTING, ///< OBD GPS Logger starting
	UI_STARTED ///< OBD GPS Logger started
};

/// UI Trip State
enum trip_state {
	TRIP_NONE, ///< All trip buttons disabled
	TRIP_STARTED, ///< On a trip
	TRIP_STOPPED ///< Not on a trip
};

/// What we want to convert to
enum obd_convert_type {
	CONVERT_KML, ///< Output a google earth file
	CONVERT_CSV ///< Output a comma-separated values file
};

#endif // __OBDGUI_H

