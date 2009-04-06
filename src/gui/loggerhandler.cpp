/** \file
   \brief class to launch and handle obdgpslogger
*/

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
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

		dup2(mPipe[1], STDOUT_FILENO); // hook stdout to the pipe
		
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

			"--samplerate", // Sample...
			"10",               // 10 times a second

			NULL // Sentinel
			);

		perror("execlp failed");
		exit(1);


	} else { // In parent
		close(mPipe[1]); // Close "write" end of pipe
		// parent can now read from mPipe[0] to get stout from child

		memset(mLinebuf, '\0', sizeof(mLinebuf));
		mCurrentBufpos = mLinebuf;
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

void loggerhandler::checkRunning(bool block) {
	if(0 < waitpid(mChildPID, NULL, block?0:WNOHANG)) {
		close(mPipe[0]);
		mUsable = false;
	}
}

void loggerhandler::pulse() {
	if(!mUsable) return;

	fd_set mask;
	FD_ZERO( &mask );
	FD_SET( mPipe[0], &mask );
	struct timeval timeout = {0, 0};

	if(select( mPipe[0]+1, &mask, NULL, NULL, &timeout ) > 0) {
		size_t readlen = read(mPipe[0], mCurrentBufpos, sizeof(mLinebuf) - (mCurrentBufpos - mLinebuf));

		if(0 < readlen) {
			mCurrentBufpos += readlen;
			*mCurrentBufpos = '\0';
		}
	}

	char line[sizeof(mLinebuf)];
	size_t linelen = strcspn(mLinebuf, "\r\n\0"); // Look for a newline

	if(0 < linelen && mLinebuf[linelen] != '\0') {
		// This one's the one we parse later
		strncpy(line, mLinebuf, linelen);
		line[linelen] = '\0';


		// Copy the rest of the string back to the start of the buffer
		char tmp[sizeof(mLinebuf)];
		strncpy(tmp, mLinebuf+linelen+1, sizeof(mLinebuf)-linelen);
		
		memset(mLinebuf, '\0', sizeof(mLinebuf));
		mCurrentBufpos = mLinebuf + strlen(mLinebuf);

		strncpy(mLinebuf, tmp, sizeof(mLinebuf));

		// printf("Got a line: %s\n", line);
		// printf("New Buffer: %s\n", mLinebuf);

	}


	if(NULL != strstr(line, "Exiting")) {
		checkRunning(true);
		return;
	}

	checkRunning(false);
	if(!mUsable) return;

	int val;

	if(0 < sscanf(line, "vss=%i", &val)) {
		mMainui->vss->value(val);
	}

	if(0 < sscanf(line, "rpm=%i", &val)) {
		mMainui->rpm->value((float)val/4.0f); // Measured in quarter revs!
	}

	if(0 < sscanf(line, "maf=%i", &val)) {
		mMainui->maf->value(val);
	}

	if(0 < sscanf(line, "throttlepos=%i", &val)) {
		mMainui->throttlepos->value(val);
	}

	if(0 < sscanf(line, "temp=%i", &val)) {
		mMainui->temp->value(val);
	}
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

