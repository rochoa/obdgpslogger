#include "maindisplay.h"
#include "loggerhandler.h"
#include "obdgui.h"

/// Main entrypoint
int main(int argc, char **argv) {
	OBDUI *mainwindow = new OBDUI();
	mainwindow->show(argc,argv);

	loggerhandler *lh;
	while (Fl::check()) {
		mainwindow->checkLogger();
		if(NULL != (lh = mainwindow->getOBDHandler())) {
			lh->pulse();
		}
	}
	delete mainwindow;
	return 0;
}

