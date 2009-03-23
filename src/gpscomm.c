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
 \brief gps communications
 */

#ifdef HAVE_GPSD

#include <gps.h>

struct gps_data_t *opengps(char *server, char *port) {
	return gps_open(server,port);
}

void closegps(struct gps_data_t *g) {
	gps_close(g);
}

int getgpsposition(struct gps_data_t *g, double *lat, double *lon, double *alt) {
	gps_query(g,"o");
	if(g->fix.mode < MODE_2D) {
		return -1;
	}
	if(g->fix.mode == MODE_2D) {
		*lat = g->fix.latitude;
		*lon = g->fix.longitude;
		return 0;
	}
	if(g->fix.mode == MODE_3D) {
		*lat = g->fix.latitude;
		*lon = g->fix.longitude;
		*alt = g->fix.altitude;
		return 1;
	}
	// Shouldn't be able to get to here...
	return -1;
}

#endif //HAVE_GPSD

