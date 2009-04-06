/** \file
   \brief class to launch and handle obdgpslogger
*/

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>


#include "maindisplay.h"
#include "loggerhandler.h"

loggerhandler::loggerhandler(OBDUI *mainui) {
	mMainui = mainui;
	mUsable = false;

	if(NULL == mMainui) return;

	if(pipe(mPipe) < 0) {
		perror("pipe error");
		return;
	}

	if(0 > (mChildPID = fork())) {
		perror("fork error");
		// Close the pipe since we can't use it
		close(mPipe[0]);
		close(mPipe[1]);
		return;
	}

	if(mChildPID == 0) { // In child
		close(mPipe[0]); // Close "read" end of pipe

		// child can now write to mPipe[1] to send stuff to parent

		dup2(mPipe[1], STDIN_FILENO); // hook stdin to the write end of the pipe
		
		const char *serialfilename = mMainui->getSerialfilename();
		const char *logfilename = mMainui->getLogfilename();

		int ret = execlp("obdgpslogger",
			"obdgpslogger",
			"--no-autotrip", // Don't start and stop trips automatically
			"--spam-stdout", // Spam all values to stdout
			"--db", // write to...
			logfilename, // this logfile
			"--serial", // connect to...
			serialfilename, // this serial port
			NULL // Sentinel
			);

		// Shouldn't execute this
		perror("execlp failed");
		exit(1);


	} else { // In parent
		close(mPipe[1]); // Close "write" end of pipe

		// parent can now read from mPipe[0] to get stout from child
	}

	mUsable = true;
}

loggerhandler::~loggerhandler() {
	if(!mUsable) return;

	// Only the parent will do this stuff
	close(mPipe[0]);

	if(0 > kill(mChildPID, SIGINT)) {
		perror("Couldn't KILL -INT child");
		return;
	}

	if(mChildPID != waitpid(mChildPID, NULL, 0)) {
		perror("waitpid unexpected value");
		return;
	}
}

void loggerhandler::pulse() {
}

void loggerhandler::starttrip() {
	if(!mUsable) return;

	if(0 > kill(mChildPID, SIGUSR1)) {
		perror("Couldn't send signal USR1 to child");
	}
}

void loggerhandler::endtrip() {
	if(!mUsable) return;

	if(0 > kill(mChildPID, SIGUSR2)) {
		perror("Couldn't send signal USR2 to child");
	}
}

