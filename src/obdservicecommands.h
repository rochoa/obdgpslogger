/* Copyright 2009 Gary Briggs

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
 \brief list of OBD service commands
 */
#ifndef __OBDSERVICECOMMANDS_H
#define __OBDSERVICECOMMANDS_H

#include <stdlib.h>

/// structure to hold OBD service commands
/** obdlogger will attempt to log all fields with a non-NULL db_column
 bytes_returned is a speed optimisation for elm327. If set to zero, do it the slow way */
struct obdservicecmd {
	unsigned int cmdid; ///< Command ID [eg 0C == engine rpm]
	int bytes_returned; ///< Number of bytes we expect back for this item.
	const char *db_column; ///< Database column name. NULL means we don't store this data
	const char *human_name; ///< Human friendly name. http://www.kbmsystems.net/obd_tech.htm
};

/// List of all OBD Service commands
/// Borrowed from various sources, mainly http://en.wikipedia.org/wiki/Table_of_OBD-II_Codes
static struct obdservicecmd obdcmds[] = {
	{ 0x01, 4, NULL,            "Monitor status since DTCs cleared" },
	{ 0x02, 4, NULL,            "DTC that caused required freeze frame data storage" },
	{ 0x03, 8, NULL,            "Fuel system 1 and 2 status" },
	{ 0x04, 2, NULL,            "Calculated LOAD Value" },
	{ 0x05, 1, "temp",          "Engine Coolant Temperature" },
	{ 0x06, 1, NULL,            "Short Term Fuel Trim - Bank 1" },
	{ 0x07, 1, NULL,            "Long Term Fuel Trim - Bank 1" },
	{ 0x08, 1, NULL,            "Short Term Fuel Trim - Bank 2" },
	{ 0x09, 1, NULL,            "Long Term Fuel Trim - Bank 2" },
	{ 0x0A, 1, NULL,            "Fuel Rail Pressure (gauge)" },
	{ 0x0B, 1, NULL,            "Intake Manifold Absolute Pressure" },
	{ 0x0C, 2, "rpm",           "Engine RPM" },
	{ 0x0D, 1, "vss",           "Vehicle Speed Sensor" },
	{ 0x0E, 1, NULL,            "Ignition Timing Advance for #1 Cylinder" },
	{ 0x0F, 1, NULL,            "Intake Air Temperature" },
	{ 0x10, 2, "maf",           "Air Flow Rate from Mass Air Flow Sensor" },
	{ 0x11, 1, "throttlepos",   "Absolute Throttle Position" },
	{ 0x12, 1, NULL,            "Commanded Secondary Air Status" },
	{ 0x13, 1, NULL,            "Location of Oxygen Sensors" },
	{ 0x14, 2, NULL,            "Bank 1 - Sensor 1/Bank 1 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x15, 2, NULL,            "Bank 1 - Sensor 2/Bank 1 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x16, 2, NULL,            "Bank 1 - Sensor 3/Bank 2 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x17, 2, NULL,            "Bank 1 - Sensor 4/Bank 2 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x18, 2, NULL,            "Bank 2 - Sensor 1/Bank 3 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x19, 2, NULL,            "Bank 2 - Sensor 2/Bank 3 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x1A, 2, NULL,            "Bank 2 - Sensor 3/Bank 4 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x1B, 2, NULL,            "Bank 2 - Sensor 4/Bank 4 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x1C, 1, NULL,            "OBD requirements to which vehicle is designed" },
	{ 0x1D, 1, NULL,            "Location of oxygen sensors" },
	{ 0x1E, 1, NULL,            "Auxiliary Input Status" },
	{ 0x1F, 2, NULL,            "Time Since Engine Start" },
	{ 0x20, 4, NULL,            "PIDs supported 21-40" },
	{ 0x21, 4, NULL,            "Distance Travelled While MIL is Activated" },
	{ 0x22, 2, NULL,            "Fuel Rail Pressure relative to manifold vacuum" },
	{ 0x23, 2, NULL,            "Fuel Rail Pressure (diesel)" },
	{ 0x24, 4, NULL,            "Bank 1 - Sensor 1/Bank 1 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x25, 4, NULL,            "Bank 1 - Sensor 2/Bank 1 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x26, 4, NULL,            "Bank 1 - Sensor 3 /Bank 2 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x27, 4, NULL,            "Bank 1 - Sensor 4 /Bank 2 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x28, 4, NULL,            "Bank 2 - Sensor 1 /Bank 3 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x29, 4, NULL,            "Bank 2 - Sensor 2 /Bank 3 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x2A, 4, NULL,            "Bank 2 - Sensor 3 /Bank 4 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x2B, 4, NULL,            "Bank 2 - Sensor 4 /Bank 4 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x2C, 1, NULL,            "Commanded EGR" },
	{ 0x2D, 1, NULL,            "EGR Error = (EGR actual - EGR commanded) / EGR commanded * 100%" },
	{ 0x2E, 1, NULL,            "Commanded Evaporative Purge" },
	{ 0x2F, 1, NULL,            "Fuel Level Input" },
	{ 0x30, 1, NULL,            "Number of warm-ups since diagnostic trouble codes cleared" },
	{ 0x31, 2, NULL,            "Distance since diagnostic trouble codes cleared" },
	{ 0x32, 2, NULL,            "Evap System Vapour Pressure" },
	{ 0x33, 1, NULL,            "Barometric Pressure" },
	{ 0x34, 4, NULL,            "Bank 1 - Sensor 1/Bank 1 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x35, 4, NULL,            "Bank 1 - Sensor 2/Bank 1 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x36, 4, NULL,            "Bank 1 - Sensor 3/Bank 2 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x37, 4, NULL,            "Bank 1 - Sensor 4/Bank 2 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x38, 4, NULL,            "Bank 2 - Sensor 1/Bank 3 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x39, 4, NULL,            "Bank 2 - Sensor 2/Bank 3 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x3A, 4, NULL,            "Bank 2 - Sensor 3/Bank 4 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x3B, 4, NULL,            "Bank 2 - Sensor 4/Bank 4 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x3C, 2, NULL,            "Catalyst Temperature Bank 1 /  Sensor 1" },
	{ 0x3D, 2, NULL,            "Catalyst Temperature Bank 2 /  Sensor 1" },
	{ 0x3E, 2, NULL,            "Catalyst Temperature Bank 1 /  Sensor 2" },
	{ 0x3F, 2, NULL,            "Catalyst Temperature Bank 2 /  Sensor 2" },
	{ 0x40, 4, NULL,            "PIDs supported 41-60" },
	{ 0x41, 4, NULL,            "Monitor status this driving cycle" },
	{ 0x42, 2, NULL,            "Control module voltage" },
	{ 0x43, 2, NULL,            "Absolute Load Value" },
	{ 0x44, 2, NULL,            "Commanded Equivalence Ratio" },
	{ 0x45, 1, NULL,            "Relative Throttle Position" },
	{ 0x46, 1, NULL,            "Ambient air temperature" },
	{ 0x47, 1, NULL,            "Absolute Throttle Position B" },
	{ 0x48, 1, NULL,            "Absolute Throttle Position C" },
	{ 0x49, 1, NULL,            "Accelerator Pedal Position D" },
	{ 0x4A, 1, NULL,            "Accelerator Pedal Position E" },
	{ 0x4B, 1, NULL,            "Accelerator Pedal Position F" },
	{ 0x4C, 1, NULL,            "Commanded Throttle Actuator Control" },
	{ 0x4D, 2, NULL,            "Minutes run by the engine while MIL activated" },
	{ 0x4E, 2, NULL,            "Time since diagnostic trouble codes cleared" },
	{ 0x51, 2, NULL,            "Fuel Type" },
	{ 0x52, 2, NULL,            "Ethanol fuel %" },
	{ 0x00, 0, NULL,            NULL }
};


#endif // __OBDSERVICECOMMANDS_H

