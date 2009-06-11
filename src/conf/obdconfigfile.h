/** \file
 \brief Tool-wide configuration
 */
#ifndef __OBDCONFIGFILE_H
#define __OBDCONFIGFILE_H

#ifdef __cplusplus
extern "C" {
#endif //  __cplusplus

/// This is the config we create
struct OBDGPSConfig {
	const char *obd_device; //< Full path to the obd device
	const char *gps_device; //< Full path to the gps device
	int samplerate; //< SampleRate [number-per-second]
	int optimisations; //< Enable Optimsations
};

/// Load a config, return a struct. Must be free'd using freeOBDGPSConfig
struct OBDGPSConfig *obd_loadConfig();

/// Free a config created by loadOBDGPSConfig
void obd_freeConfig(struct OBDGPSConfig *c);

#ifdef __cplusplus
}
#endif //  __cplusplus

#endif //__OBDCONFIGFILE_H

