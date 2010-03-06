#include <FL/Fl.H>
#include "fftwindow.h"

int main(int argc, char **argv) {
	FFTOBD mainwindow;

	if(argc > 1) {
		if(NULL != strstr(argv[1], ".csv") || NULL != strstr(argv[1], ".txt")) {
			mainwindow.opencsv(argv[1]);
		} else if(NULL != strstr(argv[1], ".db")) {
			mainwindow.opendb(argv[1]);
		} else {
			fprintf(stderr, "First argument to %s must be csv,txt or db\n", argv[0]);
		}
	}

	// mainwindow.show(argc,argv);
	mainwindow.show();

	Fl::run();

	return 0;
}

