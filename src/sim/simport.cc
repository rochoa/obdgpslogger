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

#include <stdio.h>
#include <stdlib.h>
#ifdef OBDPLATFORM_POSIX
#include <time.h>
#include <sys/time.h>
#endif // OBDPLATFORM_POSIX
#include "simport.h"

OBDSimPort::OBDSimPort() {
	mUsable = false;
	mEcho = true;
	mLogFile = NULL;
}

OBDSimPort::~OBDSimPort() {
}

void OBDSimPort::setEcho(int yes) {
	mEcho = yes;
}

int OBDSimPort::getEcho() {
	return mEcho;
}

int OBDSimPort::startLog(const char *filename) {
	mLogFile = fopen(filename, "w");
	if(NULL == mLogFile) {
		perror("Couldn't open logfile\n");
		return 1;
	}
	return 0;
}

void OBDSimPort::endLog() {
	if(NULL != mLogFile) {
		fclose(mLogFile);
		mLogFile = NULL;
	}
}

void OBDSimPort::writeLog(const char *data, int out) {
#ifdef OBDPLATFORM_POSIX
	if(NULL != mLogFile) {
		char timestr[200];
		time_t t;
		struct tm *tmp;

		t = time(NULL);
		tmp = localtime(&t);
		if (tmp == NULL) {
			snprintf(timestr, sizeof(timestr), "Unknown time");
		}

		if (strftime(timestr, sizeof(timestr), "%H:%M:%S", tmp) == 0) {
			snprintf(timestr, sizeof(timestr), "Unknown time");
		}

		fprintf(mLogFile, "%s(%s): '%s'\n", timestr, out==SERIAL_OUT?"out":"in", data);
		fflush(mLogFile);
	}
#else
#warning "Better logging including timestamps not implemented here yet"
	if(NULL != mLogFile) {
		fputs(data, mLogFile);
		fflush(mLogFile);
	}
#endif
}

int OBDSimPort::isUsable() {
	return mUsable;
}

void OBDSimPort::setUsable(int yes) {
	mUsable = yes;
}


