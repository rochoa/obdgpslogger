/** \file
 \brief Provide a pty to access an ftdi device, in case of missing kernel driver

gcc -o ftdipty ftdipty.c -lftdi
*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ftdi.h>

int main(int argc, const char **argv) {
	int ret;

	// Create an ftdi context
	struct ftdi_context *ftdic;
	if (NULL == (ftdic = ftdi_new())) {
		fprintf(stderr, "ftdi_new failed\n");
		return 1;
	}

	int vendorid = 0x0403;
	int product = 0x6001;
	
	// Open the ftdi device
	if (0 > (ret = ftdi_usb_open(ftdic, vendorid, product))) {
		fprintf(stderr, "unable to open ftdi device: %d (%s)\n", ret, ftdi_get_error_string(ftdic));
		ftdi_free(ftdic);
		return 1;
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

