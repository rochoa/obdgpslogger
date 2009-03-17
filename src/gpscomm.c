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

