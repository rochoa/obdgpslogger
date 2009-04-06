#include "maindisplay.h"
#include "loggerhandler.h"
#include "obdgui.h"

/// FLTK idle handler to pulse input
void obd_idle_cb(void *ui) {
	OBDUI *o = static_cast<OBDUI *>(ui);
	loggerhandler *lh;

	o->checkLogger();
}

/// Main entrypoint
int main(int argc, char **argv) {
	OBDUI *mainwindow = new OBDUI();
	mainwindow->show(argc,argv);

	Fl::add_idle(obd_idle_cb, static_cast<void *>(mainwindow));

	Fl::run();

	delete mainwindow;
	return 0;
}

