/** \file
 \brief gps communications
 */
#ifdef HAVE_GPSD

#ifndef __GPSCOMM_H
#define __GPSCOMM_H

#include <gps.h>

/// Open the gps
/** \param server server running gpsd
 \param port port gpsd is listening on
 \return a gps_data_t, or NULL on failure
 */
struct gps_data_t *opengps(char *server, char *port);

/// Close the gps
void closegps(struct gps_data_t *g);

/// Get the current position
/** \param g the gps_data_t returned from opengps
 \param lat pointer to where you want the latitude stored
 \param lon pointer to where you want the longitude stored
 \param alt pointer to where you want the altitude stored
 \return -1 for no workable co-ordinates, 0 for lat,lon, and 1 for lat,lon,alt
 */
int getgpsposition(struct gps_data_t *g, double *lat, double *lon, double *alt);


#endif //__GPSCOMM_H

#endif //HAVE_GPSD

