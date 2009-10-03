/** \file
 \brief Provide a pty to access an ftdi device, in case of missing kernel driver

gcc -o ftdipty ftdipty.c -lftdi
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ftdi.h>

#include "obdconfig.h"
#include "obdconfigfile.h"
#include "ftdipty.h"

int main(int argc, const char *argv[]) {
	int baudrate = -1;
	int mustexit = 0;
	int modifyconf = 0;

	int optc;
	while ((optc = getopt_long (argc, argv, shortopts, longopts, NULL)) != -1) {
		switch(optc) {
			case 'h':
				printhelp(argv[0]);
				mustexit = 1;
				break;
			case 'v':
				printversion();
				mustexit = 1;
				break;
			case 'c':
				modifyconf = 1;
				break;
			case 'b':
				baudrate = atoi(optarg);
				break;
		}
	}

	if(mustexit) {
		exit(0);
	}

	int ret;

	// Create an ftdi context
	struct ftdi_context *ftdic;
	if (NULL == (ftdic = ftdi_new())) {
		fprintf(stderr, "ftdi_new failed\n");
		return 1;
	}

	int vendorid = 0x0403;
	int possibleproducts[] = {
		0x6001, //<FT232
		0x6010, //<FT2232
		0x6006  //<FT
	};
	
	int i;
	int product;
	int found_dev = 0;
	for(i=0;i<sizeof(possibleproducts)/sizeof(possibleproducts[0]); i++) {
		// Open the ftdi device
		product = possibleproducts[0];

		if (0 == (ret = ftdi_usb_open(ftdic, vendorid, product))) {
			printf("Found ftdi device with productid 0x%X\n", product);
			found_dev = 1;
			break;
		} else {
			// fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdic));
			// ftdi_free(ftdic);
			// return 1;
		}
	}

	if(!found_dev) {
		fprintf(stderr, "Couldn't find any FTDI devices attached to system\n");
		ftdi_free(ftdic);
		return 1;
	}

	if(baudrate > -1) {
		if(0 > (ret = ftdi_set_baudrate(ftdic, baudrate))) {
			fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdic));
			ftdi_free(ftdic);
			return 1;
		}
	}

	// Open the pseudoterminal
	int fd = posix_openpt(O_RDWR | O_NOCTTY);
	if(-1 == fd) {
		perror("Couldn't posix_openpt");
		return 1;
	}
	grantpt(fd);
	unlockpt(fd);
	fcntl(fd,F_SETFL,O_NONBLOCK);

	// Print the pty slave name
	static char ptyname[1024];
	if(0 != ptsname_r(fd, ptyname, sizeof(ptyname))) {
		perror("Couldn't get pty slave");
		return 1;
	}

	printf("%s successfully opened pty. Name: %s\n", argv[0], ptyname);

	if(modifyconf) {
		struct OBDGPSConfig *conf = obd_loadConfig(0);
		if(NULL == conf) {
			fprintf(stderr, "Error opening config file. Not going to write it\n");
		} else {
			// Bad! Freeing stuff in someone else's struct!
			// TODO: fix obdconf to let us modify fields
			free((void *)conf->obd_device);
			conf->obd_device = strdup(ptyname);
			if(0 != obd_writeConfig(conf)) {
				fprintf(stderr, "Error writing config file\n");
			}
			obd_freeConfig(conf);
		}
	}

	// Seriously, how cheesy is this.
	while(1) {
		char buf[4096];
		int nbytes;

		// printf("About to read from the pty\n");
		if(0 < (nbytes = read(fd, buf, sizeof(buf)))) {
			// printf("About to write to the ftdi\n");
			ftdi_write_data(ftdic, buf, nbytes);
			write(STDIN_FILENO, buf, nbytes);
		}

		// printf("About to usleep\n");
		// Just to stop it using 100% cpu
		usleep(10000);

		// printf("About to read from the ftdi\n");
		if(0 < (nbytes = ftdi_read_data(ftdic, buf, sizeof(buf)))) {
			// printf("About to write to the pty\n");
			write(fd, buf, nbytes);
			write(STDIN_FILENO, buf, nbytes);
		}
	}


	// Close the pty
	close(fd);

	// Close the ftdi device
	if (0 > (ret = ftdi_usb_close(ftdic))) {
		fprintf(stderr, "unable to close ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdic));
		return EXIT_FAILURE;
	}

	// Free the ftdi context
	ftdi_free(ftdic);

	return 0;
}

void printhelp(const char *argv0) {
	printf("Usage: %s [params]\n"
		"   [-c|--modifyconf]\n"
		"   [-b|--baud <number>]\n"
		"   [-v|--version] [-h|--help]\n", argv0);
}

void printversion() {
        printf("Version: %i.%i\n", OBDGPSLOGGER_MAJOR_VERSION, OBDGPSLOGGER_MINOR_VERSION);
}

