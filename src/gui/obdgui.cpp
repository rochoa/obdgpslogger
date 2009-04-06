#include "maindisplay.h"
#include "loggerhandler.h"
#include "obdgui.h"

/// Main entrypoint
int main(int argc, char **argv) {
	OBDUI *mainwindow = new OBDUI();
	mainwindow->show(argc,argv);
	return Fl::run();
}

