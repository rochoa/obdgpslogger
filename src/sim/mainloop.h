/* Copyright 2009-10 Gary Briggs

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
 \brief OBD Simulator Main Loop
*/
#ifndef __MAINLOOP_H
#define __MAINLOOP_H


#include "obdsim.h"

/// It's a main loop.
/** \param sp the simport handle
    \param ss the overall sim state
    \param ecus the obdsim_generators the user has selected
    \param ecucount the number of generators in the stack
*/
void main_loop(OBDSimPort *sp, struct simsettings *ss,
	struct obdgen_ecu *ecus, int ecucount);

/// Parse this AT command [assumes that line is already known to be an AT command]
int parse_ATcmd(struct simsettings *ss, OBDSimPort *sp, char *line, char *response, size_t n);

/// Render a header into the passed string
/** \param buf buffer to put rendered header into
    \param buflen size of buf
    \param proto the obdii protocol we're rendering
    \param ecu the ecu this message is from
    \param messagelen the number of bytes being returned as the message itself
    \param spaces whether or not to put spaces between the characters [and at the end]
    \return length of string put in buf
*/
int render_obdheader(char *buf, size_t buflen, struct obdiiprotocol *proto,
	struct obdgen_ecu *ecu, unsigned int messagelen, int spaces);

/// Update the freeze frame info for all the ecus
void obdsim_freezeframes(struct obdgen_ecu *ecus, int ecucount);

#endif // __MAINLOOP_H

