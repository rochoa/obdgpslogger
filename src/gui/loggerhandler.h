/** \file
   \brief class to launch and handle obdgpslogger
*/

#ifndef __LOGGERHANDLER_H
#define __LOGGERHANDLER_H

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

class OBDUI;

/// Class to launch obdgpslogger and use the data from it
/** Drinks excessively from the Stevens kool-aid */
class loggerhandler {
public:
	/// Constructor
	/** This directly queries the UI window for parameters it needs
	   \param mainui the mainUI window already launched
	*/
	loggerhandler(OBDUI *mainui);

	/// Destructor
	~loggerhandler();

	/// Find out if this is usable
	bool isUsable() { return mUsable; }

	/// Check for new input. Do something useful with it
	void pulse();

	/// Tell obdgpslogger to start a trip
	void starttrip();

	/// Tell obdgpslogger to end a trip
	void endtrip();

protected:
	/// Handle to the main ui window
	OBDUI *mMainui;

	/// Handle onto obdgpslogger
	FILE *mLoggerHandle;

	/// Set if this object is usable
	bool mUsable;

	/// The pipe into which obdgpslogger writes stuff
	int mPipe[2];

	/// The PID of the obdgpslogger process
	pid_t mChildPID;
};


#endif // __LOGGERHANDLER_H

