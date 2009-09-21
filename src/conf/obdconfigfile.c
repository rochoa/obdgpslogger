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
 \brief Tool-wide configuration
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "obdconfig.h"
#include "obdservicecommands.h"
#include "obdconfigfile.h"

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif //MAX_PATH

/// Get "a" valid home dir in which to store a dotfile
static const char *getPlatformHomeDir() {
	static char homedir[MAX_PATH] = "\0";

	char *envhome;
	if(0 != strlen(homedir)) {
		return homedir;
	} else {
		// On OSX, might want to suffix "Application Data" ?
		envhome = getenv("HOME");
		if(NULL != envhome && *envhome) {
			snprintf(homedir,sizeof(homedir), "%s/", envhome);
			return homedir;
		}

		// Smells like windows
		envhome = getenv("APPDATA");
		if(NULL != envhome && *envhome) {
			snprintf(homedir,sizeof(homedir), "%s", envhome);
			return homedir;
		}

		snprintf(homedir,sizeof(homedir), ".");
	}
	
	return homedir;
}

static int obd_parseConfig(FILE *f, struct OBDGPSConfig *c, int verbose) {
	char line[1024];
	while(NULL != fgets(line,sizeof(line),f)) {
		// If you change the 1024 here, be damn sure to change it in the sscanf too
		char singleval_s[1024];
		int singleval_i;
		if(1 == sscanf(line, "obddevice=%1023s", singleval_s)) {
			if(NULL != c->obd_device) {
				free((void *)c->obd_device);
			}
			c->obd_device = strdup(singleval_s);
			if(verbose) printf("Conf Found OBD Device: %s\n", singleval_s);
		}
		if(1 == sscanf(line, "gpsdevice=%1023s", singleval_s)) {
			if(NULL != c->gps_device) {
				free((void *)c->gps_device);
			}
			c->gps_device = strdup(singleval_s);
			if(verbose) printf("Conf Found GPS Device: %s\n", singleval_s);
		}
		if(1 == sscanf(line, "log_columns=%1023s", singleval_s)) {
			if(NULL != c->log_columns) {
				free((void *)c->log_columns);
			}
			c->log_columns = strdup(singleval_s);
			if(verbose) printf("Conf Found log_columns: %s\n", singleval_s);
		}
		if(1 == sscanf(line, "samplerate=%i", &singleval_i)) {
			c->samplerate = singleval_i;
			if(verbose) printf("Conf Found samplerate: %i\n", singleval_i);
		}
		if(1 == sscanf(line, "optimisations=%i", &singleval_i)) {
			c->optimisations = singleval_i;
			if(verbose) printf("Conf Found optimisations: %i\n", singleval_i);
		}
	}
	return 0;
}

struct OBDGPSConfig *obd_loadConfig(int verbose) {
	struct OBDGPSConfig *c = (struct OBDGPSConfig *)malloc(sizeof(struct OBDGPSConfig));
	if(NULL == c) return NULL;
	c->obd_device = strdup(OBD_DEFAULT_SERIALPORT);
	c->gps_device = strdup(OBD_DEFAULT_GPSPORT);
	c->log_columns = strdup(OBD_DEFAULT_COLUMNS);
	c->samplerate = 1;
	c->optimisations = 0;

	char fullfilename[MAX_PATH];

	// For portableness back to older windows, '/' separator won't work
	snprintf(fullfilename, sizeof(fullfilename), "%s/%s",
			getPlatformHomeDir(), OBD_CONFIG_FILENAME
			);

	FILE *f = fopen(fullfilename, "r");

	if(NULL != f) {
		obd_parseConfig(f,c,verbose);
		fclose(f);
	}

	if(verbose) {
		printf("Full Config:\n"
					 "	obddevice:%s\n"
					 "	gpsdevice:%s\n"
					 "	logcolumns:%s\n"
					 "	optimisations:%i\n"
					 "	samplerate:%i\n",
					 	c->obd_device, c->gps_device, c->log_columns,
						c->optimisations, c->samplerate);
	}
	return c;
}

/// Free a config created by loadOBDGPSConfig
void obd_freeConfig(struct OBDGPSConfig *c) {
	if(NULL == c) return;
	if(NULL != c->obd_device) free((void *)c->obd_device);
	if(NULL != c->gps_device) free((void *)c->gps_device);
	if(NULL != c->log_columns) free((void *)c->log_columns);
	free(c);
}

int obd_configCmds(struct OBDGPSConfig *c, struct obdservicecmd ***cmds) {
	int cols = 0;
	*cmds = NULL;
	const char *toklist=",: "; // Seriously, we only want comma-separated though

	// Goind to do this twice. First time, get a count to allocate.
	char *cmdlist = strdup(c->log_columns);
	if(NULL == cmdlist) return -1;

	char *currcmd = strtok(cmdlist, toklist);
	while(currcmd) {
		struct obdservicecmd *c;
		if(NULL != (c = obdGetCmdForColumn(currcmd))) {
			cols++;
		} else {
			printf("Warning: Couldn't find column '%s'. Possible config file problem\n", currcmd);
		}
		currcmd = strtok(NULL, toklist);
	}
	free((void *)cmdlist);

	// Second time, do the allocation then the assignement
	*cmds = (struct obdservicecmd **)malloc((cols+1) * sizeof(struct obdservicecmd *)); // 1 == NULL sentinel

	int currcol = 0;
	cmdlist = strdup(c->log_columns);
	if(NULL == cmdlist) return -1;

	currcmd = strtok(cmdlist, toklist);
	while(currcmd) {
		struct obdservicecmd *c;
		if(NULL != (c = obdGetCmdForColumn(currcmd))) {
			(*cmds)[currcol++] = c;
		}
		currcmd = strtok(NULL, toklist);
	}
	free((void *)cmdlist);

	(*cmds)[currcol] = NULL;

	return cols;
}

void obd_freeConfigCmds(struct obdservicecmd **cmds) {
	free((void *)cmds);
}

