/** \file
 \brief list of OBD service commands
 */
#ifndef __OBDSERVICECOMMANDS_H
#define __OBDSERVICECOMMANDS_H

#include <stdlib.h>

/// structure to hold OBD service commands
struct obdservicecmd {
	unsigned int cmdid; ///< Command ID [eg 0C == engine rpm]
	const char *db_column; ///< Database column name. NULL means we don't store this data
	const char *human_name; ///< Human friendly name. http://www.kbmsystems.net/obd_tech.htm
};

/// List of all OBD Service commands
/** obdlogger will attempt to log all fields with a non-NULL db_column */
static struct obdservicecmd obdcmds[] = {
	{ 0x01, NULL,            "Monitor status since DTCs cleared" },
	{ 0x02, NULL,            "DTC that caused required freeze frame data storage" },
	{ 0x03, NULL,            "Fuel system 1 and 2 status" },
	{ 0x04, NULL,            "Calculated LOAD Value" },
	{ 0x05, "temp",          "Engine Coolant Temperature" },
	{ 0x06, NULL,            "Short Term Fuel Trim - Bank 1" },
	{ 0x07, NULL,            "Long Term Fuel Trim - Bank 1" },
	{ 0x08, NULL,            "Short Term Fuel Trim - Bank 2" },
	{ 0x09, NULL,            "Long Term Fuel Trim - Bank 2" },
	{ 0x0A, NULL,            "Fuel Rail Pressure (gauge)" },
	{ 0x0B, NULL,            "Intake Manifold Absolute Pressure" },
	{ 0x0C, "rpm",           "Engine RPM" },
	{ 0x0D, "vss",           "Vehicle Speed Sensor" },
	{ 0x0E, NULL,            "Ignition Timing Advance for #1 Cylinder" },
	{ 0x0F, NULL,            "Intake Air Temperature" },
	{ 0x10, "maf",           "Air Flow Rate from Mass Air Flow Sensor" },
	{ 0x11, "throttlepos",   "Absolute Throttle Position" },
	{ 0x12, NULL,            "Commanded Secondary Air Status" },
	{ 0x13, NULL,            "Location of Oxygen Sensors" },
	{ 0x14, NULL,            "Bank 1 - Sensor 1/Bank 1 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x15, NULL,            "Bank 1 - Sensor 2/Bank 1 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x16, NULL,            "Bank 1 - Sensor 3/Bank 2 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x17, NULL,            "Bank 1 - Sensor 4/Bank 2 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x18, NULL,            "Bank 2 - Sensor 1/Bank 3 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x19, NULL,            "Bank 2 - Sensor 2/Bank 3 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x1A, NULL,            "Bank 2 - Sensor 3/Bank 4 - Sensor 1 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x1B, NULL,            "Bank 2 - Sensor 4/Bank 4 - Sensor 2 Oxygen Sensor Output Voltage / Short Term Fuel Trim" },
	{ 0x1C, NULL,            "OBD requirements to which vehicle is designed" },
	{ 0x1D, NULL,            "Location of oxygen sensors" },
	{ 0x1E, NULL,            "Auxiliary Input Status" },
	{ 0x1F, NULL,            "Time Since Engine Start" },
	{ 0x20, NULL,            "Distance Travelled While MIL is Activated" },
	{ 0x21, NULL,            "Fuel Rail Pressure relative to manifold vacuum" },
	{ 0x22, NULL,            "Fuel Rail Pressure" },
	{ 0x23, NULL,            "Bank 1 - Sensor 1/Bank 1 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x24, NULL,            "Bank 1 - Sensor 2/Bank 1 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x25, NULL,            "Bank 1 - Sensor 3 /Bank 2 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x26, NULL,            "Bank 1 - Sensor 4 /Bank 2 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x27, NULL,            "Bank 2 - Sensor 1 /Bank 3 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x28, NULL,            "Bank 2 - Sensor 2 /Bank 3 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x29, NULL,            "Bank 2 - Sensor 3 /Bank 4 - Sensor 1(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x2A, NULL,            "Bank 2 - Sensor 4 /Bank 4 - Sensor 2(wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Voltage" },
	{ 0x2B, NULL,            "Commanded EGR" },
	{ 0x2C, NULL,            "EGR Error = (EGR actual - EGR commanded) / EGR commanded * 100%" },
	{ 0x2D, NULL,            "Commanded Evaporative Purge" },
	{ 0x2E, NULL,            "Fuel Level Input" },
	{ 0x2F, NULL,            "Number of warm-ups since diagnostic trouble codes cleared" },
	{ 0x30, NULL,            "Distance since diagnostic trouble codes cleared" },
	{ 0x31, NULL,            "Evap System Vapour Pressure" },
	{ 0x32, NULL,            "Barometric Pressure" },
	{ 0x33, NULL,            "Bank 1 - Sensor 1/Bank 1 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x34, NULL,            "Bank 1 - Sensor 2/Bank 1 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x35, NULL,            "Bank 1 - Sensor 3/Bank 2 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x36, NULL,            "Bank 1 - Sensor 4/Bank 2 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x37, NULL,            "Bank 2 - Sensor 1/Bank 3 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x38, NULL,            "Bank 2 - Sensor 2/Bank 3 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x39, NULL,            "Bank 2 - Sensor 3/Bank 4 - Sensor 1 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x3A, NULL,            "Bank 2 - Sensor 4/Bank 4 - Sensor 2 (wide range O2S) Oxygen Sensors Equivalence Ratio (lambda) / Current" },
	{ 0x3B, NULL,            "Catalyst Temperature Bank 1 /  Sensor 1" },
	{ 0x3C, NULL,            "Catalyst Temperature Bank 2 /  Sensor 1" },
	{ 0x3D, NULL,            "Catalyst Temperature Bank 1 /  Sensor 2" },
	{ 0x3E, NULL,            "Catalyst Temperature Bank 2 /  Sensor 2" },
	{ 0x3F, NULL,            "Monitor status this driving cycle" },
	{ 0x40, NULL,            "Control module voltage" },
	{ 0x41, NULL,            "Absolute Load Value" },
	{ 0x42, NULL,            "Commanded Equivalence Ratio" },
	{ 0x43, NULL,            "Relative Throttle Position" },
	{ 0x44, NULL,            "Ambient air temperature" },
	{ 0x45, NULL,            "Absolute Throttle Position B" },
	{ 0x46, NULL,            "Absolute Throttle Position C" },
	{ 0x47, NULL,            "Accelerator Pedal Position D" },
	{ 0x48, NULL,            "Accelerator Pedal Position E" },
	{ 0x49, NULL,            "Accelerator Pedal Position F" },
	{ 0x4A, NULL,            "Commanded Throttle Actuator Control" },
	{ 0x4B, NULL,            "Minutes run by the engine while MIL activated" },
	{ 0x4C, NULL,            "Time since diagnostic trouble codes cleared" },
	{ 0x00, NULL,            NULL }
};


#endif // __OBDSERVICECOMMANDS_H

