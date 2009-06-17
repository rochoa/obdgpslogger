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
#include "obdconvertfunctions.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

/// structure to hold OBD service commands
/** obdlogger will attempt to log all fields with a non-NULL db_column
 bytes_returned is a speed optimisation for elm327. If set to zero, do it the slow way.
  */
struct obdservicecmd {
	unsigned int cmdid; ///< Command ID [eg 0C == engine rpm]
	int bytes_returned; ///< Number of bytes we expect back for this item. Elm327 optimisation
	const char *db_column; ///< Database column name. NULL means we don't store this data
	const char *human_name; ///< Human friendly name. http://www.kbmsystems.net/obd_tech.htm
	float min_value; ///< Minimum allowable value [after conversion]
	float max_value; ///< Maximum allowable value [after conversion]
	const char *units; ///< Human friendly representation of units
	OBDConvFunc conv; ///< Function to convert OBD values for this command to a useful number
	OBDConvRevFunc convrev; ///< Function to convert a useful number back to OBD values
};

/// Return the obdservicecmd struct for the requested db_column name
/** This function is O(n) including a strcmp. You probably don't
want to call it very often. */
struct obdservicecmd *obdGetCmdForColumn(const char *db_column);

/// Return the obdservicecmd struct for the requested db_column name
/** This function is O(n). You probably don't
want to call it very often. */
struct obdservicecmd *obdGetCmdForPID(const unsigned int pid);


/// List of all OBD Service commands
/// Borrowed from various sources, mainly http://en.wikipedia.org/wiki/Table_of_OBD-II_Codes
static struct obdservicecmd obdcmds[] = {
	{ 0x00, 4, NULL,            "PIDs supported 00-20" , 0, 0, "Bit Encoded", NULL },
	{ 0x01, 4, NULL,            "Monitor status since DTCs cleared" , 0, 0, "Bit Encoded", NULL },
	{ 0x02, 4, NULL,            "DTC that caused required freeze frame data storage" , 0, 0, "Bit Encoded", NULL },
	{ 0x03, 8, NULL,            "Fuel system 1 and 2 status" , 0, 0, "Bit Encoded", NULL },
	{ 0x04, 2, NULL,            "Calculated LOAD Value" , 0, 100, "%", obdConvert_04, obdRevConvert_04 },
	{ 0x05, 1, "temp",          "Engine Coolant Temperature" , -40, 215, "Celsius", obdConvert_05, obdRevConvert_05 },
	{ 0x06, 1, NULL,            "Short Term Fuel Trim - Bank 1", -100, 99.22, "%", obdConvert_06_09, obdRevConvert_06_09 },
	{ 0x07, 1, NULL,            "Long Term Fuel Trim - Bank 1", -100, 99.22, "%", obdConvert_06_09, obdRevConvert_06_09 },
	{ 0x08, 1, NULL,            "Short Term Fuel Trim - Bank 2", -100, 99.22, "%", obdConvert_06_09, obdRevConvert_06_09 },
	{ 0x09, 1, NULL,            "Long Term Fuel Trim - Bank 2", -100, 99.22, "%", obdConvert_06_09, obdRevConvert_06_09 },
	{ 0x0A, 1, NULL,            "Fuel Rail Pressure (gauge)", -100, 99.22, "%", obdConvert_0A, obdRevConvert_0A },
	{ 0x0B, 1, NULL,            "Intake Manifold Absolute Pressure", 0, 765, "kPa", obdConvert_0B, obdRevConvert_0B },
	{ 0x0C, 2, "rpm",           "Engine RPM", 0, 16383.75, "kPa", obdConvert_0C, obdRevConvert_0C },
	{ 0x0D, 1, "vss",           "Vehicle Speed Sensor", 0, 255, "km/h", obdConvert_0D, obdRevConvert_0D },
	{ 0x0E, 1, NULL,            "Ignition Timing Advance for #1 Cylinder", -64, 63.5, "degrees relative to #1 cylinder", obdConvert_0E, obdRevConvert_0E },
	{ 0x0F, 1, NULL,            "Intake Air Temperature", -40, 215, "Celsius", obdConvert_0F, obdRevConvert_0F },
	{ 0x10, 2, "maf",           "Air Flow Rate from Mass Air Flow Sensor", 0, 655.35, "g/s", obdConvert_10, obdRevConvert_10 },
	{ 0x11, 1, "throttlepos",   "Absolute Throttle Position", 1, 100, "%", obdConvert_11, obdRevConvert_11 },
	{ 0x12, 1, NULL,            "Commanded Secondary Air Status" , 0, 0, "Bit Encoded", NULL },
	{ 0x13, 1, NULL,            "Location of Oxygen Sensors" , 0, 0, "Bit Encoded", NULL },
	{ 0x14, 2, NULL,            "Bank 1 - Sensor 1/Bank 1 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x15, 2, NULL,            "Bank 1 - Sensor 2/Bank 1 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x16, 2, NULL,            "Bank 1 - Sensor 3/Bank 2 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x17, 2, NULL,            "Bank 1 - Sensor 4/Bank 2 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x18, 2, NULL,            "Bank 2 - Sensor 1/Bank 3 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x19, 2, NULL,            "Bank 2 - Sensor 2/Bank 3 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x1A, 2, NULL,            "Bank 2 - Sensor 3/Bank 4 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x1B, 2, NULL,            "Bank 2 - Sensor 4/Bank 4 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim", 0, 1.275, "V", obdConvert_14_1B, obdRevConvert_14_1B },
	{ 0x1C, 1, NULL,            "OBD requirements to which vehicle is designed" , 0, 0, "Bit Encoded", NULL },
	{ 0x1D, 1, NULL,            "Location of oxygen sensors" , 0, 0, "Bit Encoded", NULL },
	{ 0x1E, 1, NULL,            "Auxiliary Input Status" , 0, 0, "Bit Encoded", NULL },
	{ 0x1F, 2, NULL,            "Time Since Engine Start", 0, 65535, "seconds", obdConvert_1F, obdRevConvert_1F },
	{ 0x20, 4, NULL,            "PIDs supported 21-40" , 0, 0, "Bit Encoded", NULL },
	{ 0x21, 4, NULL,            "Distance Travelled While MIL is Activated", 0, 65535, "km", obdConvert_21, obdRevConvert_21 },
	{ 0x22, 2, NULL,            "Fuel Rail Pressure relative to manifold vacuum", 0, 5177.265, "kPa", obdConvert_22, obdRevConvert_22 },
	{ 0x23, 2, NULL,            "Fuel Rail Pressure (diesel)", 0, 655350, "kPa", obdConvert_23, obdRevConvert_23 },
	{ 0x24, 4, NULL,            "Bank 1 - Sensor 1/Bank 1 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x25, 4, NULL,            "Bank 1 - Sensor 2/Bank 1 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x26, 4, NULL,            "Bank 1 - Sensor 3 /Bank 2 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x27, 4, NULL,            "Bank 1 - Sensor 4 /Bank 2 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x28, 4, NULL,            "Bank 2 - Sensor 1 /Bank 3 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x29, 4, NULL,            "Bank 2 - Sensor 2 /Bank 3 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x2A, 4, NULL,            "Bank 2 - Sensor 3 /Bank 4 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x2B, 4, NULL,            "Bank 2 - Sensor 4 /Bank 4 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage", 0, 2, "(ratio)", obdConvert_24_2B, obdRevConvert_24_2B },
	{ 0x2C, 1, NULL,            "Commanded EGR", 0, 100, "%", obdConvert_2C, obdRevConvert_2C },
	{ 0x2D, 1, NULL,            "EGR Error", -100, 99.22, "%", obdConvert_2D, obdRevConvert_2D },
	{ 0x2E, 1, NULL,            "Commanded Evaporative Purge", 0, 100, "%", obdConvert_2E, obdRevConvert_2E },
	{ 0x2F, 1, NULL,            "Fuel Level Input", 0, 100, "%", obdConvert_2F, obdRevConvert_2F },
	{ 0x30, 1, NULL,            "Number of warm-ups since diagnostic trouble codes cleared", 0, 255, "", obdConvert_30, obdRevConvert_30 },
	{ 0x31, 2, NULL,            "Distance since diagnostic trouble codes cleared", 0, 65535, "km", obdConvert_31, obdRevConvert_31 },
	{ 0x32, 2, NULL,            "Evap System Vapour Pressure", -8192, 8192, "Pa", obdConvert_32, obdRevConvert_32 },
	{ 0x33, 1, NULL,            "Barometric Pressure", 0, 255, "kPa", obdConvert_33, obdRevConvert_33 },
	{ 0x34, 4, NULL,            "Bank 1 - Sensor 1/Bank 1 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x35, 4, NULL,            "Bank 1 - Sensor 2/Bank 1 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x36, 4, NULL,            "Bank 1 - Sensor 3/Bank 2 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x37, 4, NULL,            "Bank 1 - Sensor 4/Bank 2 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x38, 4, NULL,            "Bank 2 - Sensor 1/Bank 3 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x39, 4, NULL,            "Bank 2 - Sensor 2/Bank 3 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x3A, 4, NULL,            "Bank 2 - Sensor 3/Bank 4 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x3B, 4, NULL,            "Bank 2 - Sensor 4/Bank 4 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current", 0, 2, "(ratio)", obdConvert_34_3B, obdRevConvert_34_3B },
	{ 0x3C, 2, NULL,            "Catalyst Temperature Bank 1 /  Sensor 1", -40, 6513.5, "Celsius", obdConvert_3C_3F, obdRevConvert_3C_3F },
	{ 0x3D, 2, NULL,            "Catalyst Temperature Bank 2 /  Sensor 1", -40, 6513.5, "Celsius", obdConvert_3C_3F, obdRevConvert_3C_3F },
	{ 0x3E, 2, NULL,            "Catalyst Temperature Bank 1 /  Sensor 2", -40, 6513.5, "Celsius", obdConvert_3C_3F, obdRevConvert_3C_3F },
	{ 0x3F, 2, NULL,            "Catalyst Temperature Bank 2 /  Sensor 2", -40, 6513.5, "Celsius", obdConvert_3C_3F, obdRevConvert_3C_3F },
	{ 0x40, 4, NULL,            "PIDs supported 41-60" , 0, 0, "Bit Encoded", NULL },
	{ 0x41, 4, NULL,            "Monitor status this driving cycle" , 0, 0, "Bit Encoded", NULL },
	{ 0x42, 2, NULL,            "Control module voltage", 0, 65535, "V", obdConvert_42, obdRevConvert_42 },
	{ 0x43, 2, NULL,            "Absolute Load Value", 0, 25700, "%", obdConvert_43, obdRevConvert_43 },
	{ 0x44, 2, NULL,            "Commanded Equivalence Ratio", 0, 2, "(ratio)", obdConvert_44, obdRevConvert_44 },
	{ 0x45, 1, NULL,            "Relative Throttle Position", 0, 100, "%", obdConvert_45, obdRevConvert_45 },
	{ 0x46, 1, NULL,            "Ambient air temperature", -40, 215, "Celsius", obdConvert_46, obdRevConvert_46 },
	{ 0x47, 1, NULL,            "Absolute Throttle Position B", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x48, 1, NULL,            "Absolute Throttle Position C", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x49, 1, NULL,            "Accelerator Pedal Position D", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x4A, 1, NULL,            "Accelerator Pedal Position E", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x4B, 1, NULL,            "Accelerator Pedal Position F", 0, 100, "%", obdConvert_47_4B, obdRevConvert_47_4B },
	{ 0x4C, 1, NULL,            "Commanded Throttle Actuator Control", 0, 100, "%", obdConvert_4C, obdRevConvert_4C },
	{ 0x4D, 2, NULL,            "Time run by the engine while MIL activated", 0, 65525, "minutes", obdConvert_4D, obdRevConvert_4D },
	{ 0x4E, 2, NULL,            "Time since diagnostic trouble codes cleared", 0, 65535, "minutes", obdConvert_4E, obdRevConvert_4E },
	{ 0x51, 2, NULL,            "Fuel Type", 0, 0, "", NULL },
	{ 0x52, 2, NULL,            "Ethanol fuel %", 0, 100, "%", obdConvert_52, obdRevConvert_52 },
	{ 0x00, 0, NULL,            NULL }
};

#ifdef __cplusplus
}
#endif // __cplusplus


#endif // __OBDSERVICECOMMANDS_H

