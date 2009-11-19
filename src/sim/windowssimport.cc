/* Copyright 2009 Gary Briggs, Michael Carpenter

This file is part of obdgpslogger.

obdgpslogger is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

obdgpslogger is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with obdgpslogger.  If not, see <http://www.gnu.org/licenses/>.
*/

/** \file
  \brief Tools to open the sim port
*/

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "windowssimport.h"
#include "simport.h"

WindowsSimPort::WindowsSimPort(const char *port) {
	portname = NULL;

	if(NULL == port || 0 == strlen(port)) {
		portname = strdup(port);
	}

	portHandle=CreateFileA(portname, GENERIC_READ|GENERIC_WRITE,0, NULL, OPEN_EXISTING, 0, NULL);
	if (portHandle == INVALID_HANDLE_VALUE) {
		return;
	}
	COMMCONFIG Win_CommConfig;
	COMMTIMEOUTS Win_CommTimeouts;
	unsigned long confSize = sizeof(COMMCONFIG);
	Win_CommConfig.dwSize = confSize;
	GetCommConfig(portHandle, &Win_CommConfig, &confSize);
	Win_CommConfig.dcb.Parity = 0;
	Win_CommConfig.dcb.fRtsControl = RTS_CONTROL_DISABLE;
	Win_CommConfig.dcb.fOutxCtsFlow = FALSE;
	Win_CommConfig.dcb.fOutxDsrFlow = FALSE;
	Win_CommConfig.dcb.fDtrControl = DTR_CONTROL_DISABLE;
	Win_CommConfig.dcb.fDsrSensitivity = FALSE;
	Win_CommConfig.dcb.fNull=FALSE;
	Win_CommConfig.dcb.fTXContinueOnXoff = FALSE;
	Win_CommConfig.dcb.fInX=FALSE;
	Win_CommConfig.dcb.fOutX=FALSE;
	Win_CommConfig.dcb.fBinary=TRUE;
	Win_CommConfig.dcb.DCBlength = sizeof(DCB);
	Win_CommConfig.dcb.BaudRate = 9600;
	Win_CommConfig.dcb.ByteSize = 8;
	Win_CommTimeouts.ReadIntervalTimeout = 50;
	Win_CommTimeouts.ReadTotalTimeoutMultiplier = 0;
	Win_CommTimeouts.ReadTotalTimeoutConstant = 110;
	Win_CommTimeouts.WriteTotalTimeoutMultiplier = 0;
	Win_CommTimeouts.WriteTotalTimeoutConstant = 110;
	SetCommConfig(portHandle, &Win_CommConfig, sizeof(COMMCONFIG));
	SetCommTimeouts(portHandle,&Win_CommTimeouts);


	setUsable(1);
}

WindowsSimPort::~WindowsSimPort() {
	delete portname;
	CloseHandle(portHandle);
}

char *WindowsSimPort::getPort() {
	return portname;
}

char *WindowsSimPort::readLine() {
	int nbytes; // Number of bytes read
	char *currpos = readbuf + readbuf_pos;

	ReadFile(portHandle, (void*)currpos, sizeof(readbuf) - readbuf_pos, (LPDWORD)&nBytes, NULL);

	if(0 < nbytes) {
		if(getEcho()) {
			writeData(currpos);
		}

		// printf("Read %i bytes. strn is now '%s'\n", nbytes, readbuf);
		readbuf_pos += nbytes;
		char *lineend = strstr(readbuf, "\r");
		if(NULL == lineend) { // Just in case
			char *lineend = strstr(readbuf, "\n");
		}

		if(NULL != lineend) {
			int length = lineend - readbuf;
			strncpy(lastread, readbuf, length);
			lastread[length]='\0';

			while(*lineend == '\r' || *lineend == '\n') {
				lineend++;
			}
			memmove(readbuf, lineend, sizeof(readbuf) - (lineend - readbuf));
			readbuf_pos -= (lineend - readbuf);

			return lastread;
		}
	}
	return NULL;
}

void WindowsSimPort::writeData(const char *line) {
	int len;
	WriteFile(portHandle,line,(DWORD)strlen(line),(LPDWORD)&len,NULL);
}



