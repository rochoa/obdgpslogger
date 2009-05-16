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
 \brief OBD service commands
 */

#include "obdservicecommands.h"
#include <string.h>

struct obdservicecmd *obdGetCmdForColumn(const char *db_column) {
	int i;
	int numrows = sizeof(obdcmds)/sizeof(obdcmds[0]);
	for(i=0;i<numrows;i++) {
		struct obdservicecmd *o = &obdcmds[i];
		if(NULL == o->db_column) {
			continue;
		}

		if(0 == strcmp(db_column,o->db_column)) {
			return o;
		}
	}
	return NULL;
}

struct obdservicecmd *obdGetCmdForPID(const unsigned int pid) {
	int i;
	int numrows = sizeof(obdcmds)/sizeof(obdcmds[0]);
	for(i=0;i<numrows;i++) {
		struct obdservicecmd *o = &obdcmds[i];

		if(pid == o->cmdid) {
			return o;
		}
	}
	return NULL;
}



