/** \file
 \brief Test the obdconfig stuffs
 */
#include <stdio.h>
#include <stdlib.h>

#include "obdconfigfile.h"

int main() {
	struct OBDGPSConfig *c;
	if(NULL == (c = obd_loadConfig(1))) {
		printf("Error in loadConfig\n");
		exit(1);
	} else {
		printf("Successfully loaded config\n");
	}

	struct obdservicecmd **cmds;
	int cmds_found = obd_configCmds(c->log_columns, &cmds);
	printf("Found %i cmds:\n", cmds_found);
	int i;
	for(i=0;NULL != cmds[i];i++) {
		printf("   [%02X] %s\n", cmds[i]->cmdid, cmds[i]->human_name);
	}

	obd_freeConfigCmds(cmds);
	printf("Freed commands\n");

	obd_freeConfig(c);
	printf("Freed config\n");
	return 0;
}

