#include <FL/Fl.H>
#include "fftwindow.h"

int main(int argc, char **argv) {
	FFTOBD mainwindow;
	mainwindow.show(argc,argv);

	Fl::run();

	return 0;
}

