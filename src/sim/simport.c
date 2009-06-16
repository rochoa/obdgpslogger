/** \file
  \brief Tools to open the sim port
*/

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#include "simport.h"
#include "obdsim.h"

void *simport_open() {
	int fd = open("/dev/ptmx",O_RDWR | O_NOCTTY);
	if(-1 == fd) return NULL;
	grantpt(fd);
	unlockpt(fd);
	FILE *f = fdopen(fd, "r+");

	if(NULL == f) {
		perror("Couldn't upgrade fd to FILE *");
		close(fd);
		return NULL;
	}
	return (void *)f;
}

int simport_close(void *simport) {
	fclose((FILE *)simport);
}

char *simport_getptyslave(void *simport) {
	int fd = fileno((FILE *)simport);

	static char buf[1024];
	if(0 != ptsname_r(fd, buf, sizeof(buf))) {
		perror("Couldn't get pty slave");
		return NULL;
	}
	return buf;
}

char *simport_readline(void *simport) {
	static char buf[1024];
	if(NULL != fgets(buf, sizeof(buf), (FILE *)simport)) {
		return buf;
	}
	return NULL;
}

void simport_writeline(void *simport, const char *line) {
	fprintf((FILE *)simport, "%s", line);
}



