#include "modbus_registers.h"
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>

// TCP socket handle defined in main.cpp
extern int modbusSocket;
extern uint16_t transactionId;

// Forward declarations for TCP Modbus functions
extern bool sendModbusTCPRequest(uint8_t unitId, uint8_t functionCode, uint16_t startAddress, uint16_t quantity);

// table extracted from chapter 3.3 of MB001_ASW GEN-Modbus-en_V2.1.1.
// addr = decimal AISWEI address, length = number of 16-bit registers
// name = first line of description, type, unit, gain, access
const RegisterInfo aiswei_registers[] = {
    {31001, 1,  "Device Type",               "String",    NULL, 1.0f, "RO"},
    {31002, 1,  "Modbus address",            "U16",       NULL, 1.0f, "RO"},
    {31003, 16, "Serial Number",             "String",    NULL, 1.0f, "RO"},
    {31019, 8,  "Machine type",              "String",    NULL, 1.0f, "RO"},
    {31027, 1,  "Current grid code",         "E16",       NULL, 1.0f, "RO"},
    {31028, 2,  "Rated Power",               "U32",       "W",  1.0f, "RO"},
    {31030, 7,  "Software Version",          "String",    NULL, 1.0f, "RO"},
    {31044, 7,  "Safety Version",            "String",    NULL, 1.0f, "RO"},
    {31057, 8,  "Manufacturer's name",       "String",    NULL, 1.0f, "RO"},
    {31065, 8,  "Brand name",                "String",    NULL, 1.0f, "RO"},

    {31301, 1,  "Grid rated voltage",        "U16",       "V",  0.1f, "RO"},
    {31302, 1,  "Grid rated frequency",      "U16",       "Hz", 0.01f,"RO"},
    {31303, 2,  "E-Today of inverter",       "U32",       "kWh",0.1f, "RO"},
    {31305, 2,  "E-Total of inverter",       "U32",       "kWh",0.1f, "RO"},
    {31307, 2,  "H-Total",                   "U32",       "H",  1.0f, "RO"},
    {31309, 1,  "Device State",              "E16",       NULL, 1.0f, "RO"},
    {31310, 1,  "Connect time",              "U16",       "s",  1.0f, "RO"},

    {31311, 1,  "Air temperature",           "S16",       "C",  0.1f, "RO"},
    {31312, 1,  "Inverter U phase temp",     "S16",       "C",  0.1f, "RO"},
    {31313, 1,  "Inverter V phase temp",     "S16",       "C",  0.1f, "RO"},
    {31314, 1,  "Inverter W phase temp",     "S16",       "C",  0.1f, "RO"},
    {31315, 1,  "Boost temperature",         "S16",       "C",  0.1f, "RO"},
    {31316, 1,  "Bidirectional DC/DC temp",  "S16",       "C",  0.1f, "RO"},
    {31317, 1,  "Bus voltage",               "U16",       "V",  0.1f, "RO"},

    {31319, 1,  "PV1 voltage",               "U16",       "V",  0.1f, "RO"},
    {31320, 1,  "PV1 current",               "U16",       "A",  0.01f,"RO"},
    {31321, 1,  "PV2 voltage",               "U16",       "V",  0.1f, "RO"},
    {31322, 1,  "PV2 current",               "U16",       "A",  0.01f,"RO"},
    {31323, 1,  "PV3 voltage",               "U16",       "V",  0.1f, "RO"},
    {31324, 1,  "PV3 current",               "U16",       "A",  0.01f,"RO"},
    {31325, 1,  "PV4 voltage",               "U16",       "V",  0.1f, "RO"},
    {31326, 1,  "PV4 current",               "U16",       "A",  0.01f,"RO"},
    {31327, 1,  "PV5 voltage",               "U16",       "V",  0.1f, "RO"},
    {31328, 1,  "PV5 current",               "U16",       "A",  0.01f,"RO"},

    {31339, 1,  "String 1 current",          "U16",       "A",  0.1f, "RO"},
    {31340, 1,  "String 2 current",          "U16",       "A",  0.1f, "RO"},
    {31341, 1,  "String 3 current",          "U16",       "A",  0.1f, "RO"},
    {31342, 1,  "String 4 current",          "U16",       "A",  0.1f, "RO"},
    {31343, 1,  "String 5 current",          "U16",       "A",  0.1f, "RO"},
    {31344, 1,  "String 6 current",          "U16",       "A",  0.1f, "RO"},
    {31345, 1,  "String 7 current",          "U16",       "A",  0.1f, "RO"},
    {31346, 1,  "String 8 current",          "U16",       "A",  0.1f, "RO"},
    {31347, 1,  "String 9 current",          "U16",       "A",  0.1f, "RO"},
    {31348, 1,  "String 10 current",         "U16",       "A",  0.1f, "RO"},

    {31359, 1,  "L1 Phase voltage",          "U16",       "V",  0.1f, "RO"},
    {31360, 1,  "L1 Phase current",          "U16",       "A",  0.1f, "RO"},
    {31361, 1,  "L2 Phase voltage",          "U16",       "V",  0.1f, "RO"},
    {31362, 1,  "L2 Phase current",          "U16",       "A",  0.1f, "RO"},
    {31363, 1,  "L3 Phase voltage",          "U16",       "V",  0.1f, "RO"},
    {31364, 1,  "L3 Phase current",          "U16",       "A",  0.1f, "RO"},
    {31365, 1,  "RS Line voltage",           "U16",       "V",  0.1f, "RO"},
    {31366, 1,  "RT Line voltage",           "U16",       "V",  0.1f, "RO"},
    {31367, 1,  "ST Line voltage",           "U16",       "V",  0.1f, "RO"},

    {31368, 1,  "Grid frequency",            "U16",       "Hz", 0.01f,"RO"},
    {31369, 2,  "Apparent power",            "U32",       "VA", 1.0f, "RO"},
    {31371, 2,  "Active power",              "U32",       "W",  1.0f, "RO"},
    {31373, 2,  "Reactive power",            "S32",       "Var",1.0f, "RO"},
    {31375, 1,  "Power factor",              "S16",       NULL, 0.01f,"RO"},
    {31378, 1,  "Error message",             "E16",       NULL, 1.0f, "RO"},
    {31379, 1,  "Warning message",           "E16",       NULL, 1.0f, "RO"},

    {31601, 2,  "PV total power",            "U32",       "W",  1.0f, "RO"},
    {31603, 2,  "PV E-Today",                "U32",       "kWh",0.1f, "RO"},
    {31605, 2,  "PV E-Total",                "U32",       "kWh",0.1f, "RO"},
    {31607, 1,  "Battery communication status","E16",     NULL, 1.0f, "RO"},
    {31608, 1,  "Battery status",            "E16",       NULL, 1.0f, "RO"},
    {31609, 1,  "Battery error status",      "B16",       NULL, 1.0f, "RO"},
    {31613, 1,  "Battery warning status",    "B16",       NULL, 1.0f, "RO"},
    {31617, 1,  "Battery voltage",           "U16",       "V",  0.01f,"RO"},
    {31618, 1,  "Battery current",           "S16",       "A",  0.1f, "RO"},
    {31619, 2,  "Battery power",             "S32",       "W",  1.0f, "RO"},
    {31621, 1,  "Battery temperature",       "S16",       "C",  0.1f, "RO"},
    {31622, 1,  "Battery SOC",               "U16",       NULL, 0.01f,"RO"},
    {31623, 1,  "Battery SOH",               "U16",       NULL, 0.01f,"RO"},
    {31624, 1,  "Battery charging current limit", "U16",  "A",  0.1f, "RO"},
    {31625, 1,  "Battery discharge current limit", "U16", "A",  0.1f, "RO"},
    {31626, 2,  "Battery E-Charge-Today",    "U32",       "kWh",0.1f, "RO"},
    {31628, 2,  "Battery E-Discharge-Today", "U32",       "kWh",0.1f, "RO"},
    {31630, 2,  "E-Consumption-Today at AC side", "U32",  "kWh",0.1f, "RO"},
    {31632, 2,  "E-Generation-Today at AC side", "U32",   "kWh",0.1f, "RO"},
    {31634, 1,  "EPS load voltage",          "U16",       "V",  0.1f, "RO"},
    {31635, 1,  "EPS load current",          "U16",       "A",  0.1f, "RO"},
    {31636, 1,  "EPS load frequency",        "U16",       "Hz", 0.01f,"RO"},
    {31637, 2,  "EPS load active power",     "U32",       "W",  1.0f, "RO"},
    {31639, 2,  "EPS load reactive power",   "U32",       "Var",1.0f, "RO"},
    {31641, 2,  "E-Consumption-Today at EPS load side", "U32", "kWh", 0.1f, "RO"},
    {31643, 2,  "E-Consumption-Total at EPS load side", "U32",  "kWh",0.1f, "RO"},

    /* Holding registers */
    {40201, 1,  "Remote switch command",     "E16",       NULL, 1.0f, "RW"},
    {41001, 1,  "RTC:Year",                  "U16",       NULL, 1.0f, "RW"},
    {41002, 1,  "RTC:Month",                 "U16",       NULL, 1.0f, "RW"},
    {41003, 1,  "RTC:Day",                   "U16",       NULL, 1.0f, "RW"},
    {41004, 1,  "RTC:Hour",                  "U16",       NULL, 1.0f, "RW"},
    {41005, 1,  "RTC:Minute",                "U16",       NULL, 1.0f, "RW"},
    {41006, 1,  "RTC:Seconds",               "U16",       NULL, 1.0f, "RW"},

    /* storage / inverter control area */
    {41102, 1,  "Storage Inverter Switch",   "E16",       NULL, 1.0f, "RW"},
    {41103, 1,  "Type selection of energy storage machine", "E16", NULL, 1.0f, "RW"},
    {41104, 1,  "Run mode",                  "E16",       NULL, 1.0f, "RW"},
    {41105, 1,  "Battery manufacturer",      "E16",       NULL, 1.0f, "RW"},
    {41108, 1,  "Smart meter status",        "E16",       NULL, 1.0f, "RW"},
    {41109, 1,  "Smart meter adjustment flag bit", "E16", NULL, 1.0f, "RW"},
    {41110, 2,  "Set target power value",    "S32",       "W",  1.0f, "RW"},
    {41112, 2,  "Current power value of smart meter", "S32", "W", 1.0f, "RW"},
    {41114, 1,  "Anti reverse current flag", "E16",       NULL, 1.0f, "RW"},
    {41115, 1,  "Battery wake-up (Force charge) sign", "E16", NULL, 1.0f, "RW"},
    {41151, 1,  "Commbox and cloud communication status", "E16", NULL, 1.0f, "RW"},
    {41152, 1,  "Charge discharge flag bit", "E16",       NULL, 1.0f, "RW"},
    {41153, 1,  "Charge and discharge power command", "S16", "W", 1.0f, "RW"},

    /* 44xxx control group */
    {44001, 1,  "Active power control function", "E16",   NULL, 1.0f, "RW"},
    {44002, 1,  "EEG control function",      "E16",       NULL, 1.0f, "RW"},
    {44003, 1,  "Slope load function",       "E16",       NULL, 1.0f, "RW"},
    {44004, 1,  "Overvoltage reduce power function", "E16", NULL, 1.0f, "RW"},
    {44005, 1,  "Overfrequency reduce power function", "E16", NULL, 1.0f, "RW"},
    {44006, 1,  "Reactive power control function", "E16", NULL, 1.0f, "RW"},
    {44007, 1,  "LVRT Function",             "E16",       NULL, 1.0f, "RW"},
    {44009, 1,  "10 Minutes Average Overvoltage protect function", "E16", NULL, 1.0f, "RW"},
    {44010, 1,  "Islanding protect function", "E16",      NULL, 1.0f, "RW"},
    {44012, 1,  "PE connection check function", "E16",     NULL, 1.0f, "RW"},
    {44017, 1,  "Overload function",         "E16",       NULL, 1.0f, "RW"},
    {44025, 1,  "Shadow MPPT function",      "E16",       NULL, 1.0f, "RW"},

    /* 45xxx grid code & limits */
    {45201, 1,  "Grid code",                 "E16",       NULL, 1.0f, "RW"},
    {45202, 1,  "Overvoltage protection value 3", "U16", "V", 0.1f, "RW"},
    {45203, 1,  "Overvoltage protection value 2", "U16", "V", 0.1f, "RW"},
    {45204, 1,  "Overvoltage protection value (freq?)", "U16","Hz",0.01f,"RW"},
    {45205, 1,  "Underfrequency protection value", "U16","Hz",0.01f,"RW"},
    {45206, 1,  "Grid Voltage High Limit3",  "U16",       "V",  0.1f, "RW"},
    {45207, 2,  "Grid Voltage High Limit Time3", "U32","ms", 1.0f, "RW"},
    {45209, 1,  "Grid Voltage High Limit2",  "U16",       "V",  0.1f, "RW"},
    {45210, 2,  "Grid Voltage High Limit Time2", "U32","ms", 1.0f, "RW"},
    {45212, 1,  "Grid Voltage High Limit1",  "U16",       "V",  0.1f, "RW"},
    {45213, 2,  "Grid Voltage High Limit Time1", "U32","ms", 1.0f, "RW"},
    {45215, 1,  "Grid Voltage Low Limit3",   "U16",       "V",  0.1f, "RW"},
    {45216, 2,  "Grid Voltage Low Limit Time3", "U32","ms", 1.0f, "RW"},
    {45218, 1,  "Grid Voltage Low Limit2",   "U16",       "V",  0.1f, "RW"},
    {45219, 2,  "Grid Voltage Low Limit Time2", "U32","ms", 1.0f, "RW"},
    {45221, 1,  "Grid Voltage Low Limit1",   "U16",       "V",  0.1f, "RW"},
    {45222, 2,  "Grid Voltage Low Limit Time1", "U32","ms", 1.0f, "RW"},
    {45224, 1,  "10 Minutes Average Overvoltage Threshold", "U16","V",0.1f,"RW"},
    {45225, 1,  "10 Minutes Average Overvoltage Protect Time", "U16","ms",1.0f,"RW"},
    {45226, 1,  "Overvoltage recover value", "U16",       "V",  0.1f, "RW"},
    {45227, 1,  "Undervoltage recover value","U16",       "V",  0.1f, "RW"},
    {45228, 1,  "Grid Frequency High Limit3","U16",       "Hz", 0.01f,"RW"},
    {45229, 2,  "Grid Frequency High Limit Time3", "U32","ms",1.0f,"RW"},
    {45231, 1,  "Grid Frequency High Limit2","U16",       "Hz", 0.01f,"RW"},
    {45232, 2,  "Grid Frequency High Limit Time2", "U32","ms",1.0f,"RW"},
    {45234, 1,  "Grid Frequency High Limit1","U16",       "Hz", 0.01f,"RW"},
    {45235, 2,  "Grid Frequency High Limit Time1", "U32","ms",1.0f,"RW"},
    {45237, 1,  "Grid Frequency Low Limit3", "U16",       "Hz", 0.01f,"RW"},
    {45238, 2,  "Grid Frequency Low Limit Time3", "U32","ms",1.0f,"RW"},
    {45240, 1,  "Grid Frequency Low Limit2", "U16",       "Hz", 0.01f,"RW"},
    {45241, 2,  "Grid Frequency Low Limit Time2", "U32","ms",1.0f,"RW"},
    {45243, 1,  "Grid Frequency Low Limit1", "U16",       "Hz", 0.01f,"RW"},
    {45244, 2,  "Grid Frequency Low Limit Time1", "U32","ms",1.0f,"RW"},
    {45246, 1,  "Vary rate of Frequency protect value", "U16","Hz/s",0.01f,"RW"},
    {45247, 2,  "Vary rate of Frequency protect time", "U32","ms",1.0f,"RW"},
    {45249, 1,  "Overfrequency recover value", "U16",     "Hz", 0.01f,"RW"},
    {45250, 1,  "Underfrequency recover value","U16",     "Hz", 0.01f,"RW"},
    {45251, 1,  "Time of first connection to grid","U16",  "s",  1.0f, "RW"},
    {45252, 1,  "Time of re-connection to grid","U16",    "s",  1.0f, "RW"},
    {45253, 1,  "ISO protect threshold",     "U16",       "kΩ", 1.0f, "RW"},
    {45254, 1,  "DCI protect threshold",     "U16",       "mA", 1.0f, "RW"},
    {45255, 1,  "DCI protect time",         "U16",       "ms", 1.0f, "RW"},

    /* 454xx active power / rate / control */
    {45401, 1,  "Load rate of first connection to grid", "U16", "%Pn/min", 1.0f, "RW"},
    {45402, 1,  "Load rate of re-connection to grid", "U16", "%Pn/min", 1.0f, "RW"},
    {45403, 1,  "Active Power Set",         "U16",       "%Pn", 0.01f, "RW"},
    {45404, 1,  "Increase rate of active power", "U16","%Pn/min",0.01f,"RW"},
    {45405, 1,  "Decrease rate of active power", "U16","%Pn/min",0.01f,"RW"},
    {45408, 1,  "Over frequency reduce power mode", "E16", NULL, 1.0f, "RW"},
    {45409, 1,  "Over frequency reduce power Start frequency", "U16","Hz",0.01f,"RW"},
    {45410, 1,  "Over frequency reduce power Stop frequency",  "U16","Hz",0.01f,"RW"},
    {45411, 1,  "Over frequency reduce power Back frequency",  "U16","Hz",0.01f,"RW"},
    {45412, 1,  "The reduce ratio of over frequency reduce power", "U16","%Pnor%Pm",0.01f,"RW"},
    {45413, 1,  "Over frequency reduce power :reduce power delay time", "U16","s",0.1f,"RW"},
    {45414, 1,  "Over frequency reduce power:recover power delay time", "U16","s",0.1f,"RW"},
    {45416, 1,  "Speed of Over frequency recover to Pn", "U16","%Pn/min",0.01f,"RW"},
    {45417, 1,  "Over frequency reduce power : 0 power frequency point", "U16","Hz",0.01f,"RW"},

    /* 45419.. overvoltage reduce power */
    {45419, 1,  "Over voltage reduce power mode", "E16", NULL, 1.0f, "RW"},
    {45420, 1,  "Over voltage reduce power Start voltage", "U16","%Un",0.01f,"RW"},
    {45422, 1,  "Over voltage reduce power Stop voltage", "U16","%Un",0.01f,"RW"},
    {45424, 1,  "Over voltage reduce power Back voltage", "U16","%Un",0.01f,"RW"},
    {45426, 1,  "The reduce ratio of over voltage reduce power", "U16","%Pnor%Pm",0.01f,"RW"},
    {45427, 1,  "Over voltage reduce power delay time", "U16","s",0.1f,"RW"},
    {45428, 1,  "Over voltage recover power delay time", "U16","s",0.1f,"RW"},
    {45429, 1,  "Speed of Over voltage recover to Pn", "U16","%Pn/min",0.01f,"RW"},

    /* 45432.. Under/Over frequency groups, DRMs etc (some repeated in doc) */
    {45432, 1,  "Under frequency increase power mode", "E16", NULL, 1.0f, "RW"},
    {45433, 1,  "Under frequency increase power: Start frequency", "U16", "Hz", 0.01f, "RW"},
    {45434, 1,  "Under frequency increase power: Stop frequency",  "U16", "Hz", 0.01f, "RW"},
    {45435, 1,  "Under frequency increase power: Back frequency",  "U16", "Hz", 0.01f, "RW"},
    {45436, 1,  "The increase ratio of under frequency increase power", "U16", "%Pnor%Pm", 0.01f, "RW"},
    {45437, 1,  "Under frequency increase power: delay time", "U16", "s", 0.1f, "RW"},
    {45438, 1,  "Under frequency recover power: delay time", "U16", "s", 0.1f, "RW"},
    {45440, 1,  "Speed of Under frequency recover to Pn", "U16", "%Pn/min", 0.01f, "RW"},
    {45441, 1,  "Under frequency increase power: 0 power frequency point", "U16", "Hz", 0.01f, "RW"},

    /* Under-voltage increase power group (45443..45450) */
    {45443, 1,  "Under voltage increase power mode", "E16", NULL, 1.0f, "RW"},
    {45444, 1,  "Under voltage increase power: Start voltage", "U16", "%Un", 0.01f, "RW"},
    {45445, 1,  "Under voltage increase power: Stop voltage",  "U16", "%Un", 0.01f, "RW"},
    {45446, 1,  "Under voltage increase power: Back voltage",  "U16", "%Un", 0.01f, "RW"},
    {45447, 1,  "The increase ratio of under voltage increase power", "U16", "%Pnor%Pm", 0.01f, "RW"},
    {45448, 1,  "Under voltage increase power: delay time", "U16", "s", 0.1f, "RW"},
    {45449, 1,  "Under voltage increase power: delay time 2", "U16", "s", 0.1f, "RW"},
    {45450, 1,  "Speed of under voltage recover to Pn", "U16", "%Pn/min", 0.01f, "RW"},

    /* DRMs / Pav entries */
    {45451, 1,  "Pav", "S16", "%Pn", 0.01f, "RW"},
    {45452, 1,  "DRMs Pval", "U16", "%Pn", 0.01f, "RW"},

    /* 455xx reactive/power-factor control */
    {45501, 1,  "Reactive power control mode", "E16", NULL, 1.0f, "RW"},
    {45502, 1,  "Time constant of reactive power curve", "U16","s",1.0f,"RW"},
    {45503, 1,  "Power factor",              "S16",       NULL, 0.0001f, "RW"},
    {45504, 1,  "cos φ(P) curve: Active power first point", "U16","%Pn",0.01f,"RW"},
    {45505, 1,  "cos φ(P) curve: cos φ of first point", "S16", NULL, 0.0001f, "RW"},
    {45506, 1,  "cos φ(P) curve: Active power second point", "U16","%Pn",0.01f,"RW"},
    {45507, 1,  "cos φ(P) curve: cos φ of second point", "S16", NULL, 0.0001f, "RW"},
    {45508, 1,  "cos φ(P) curve: Active power third point", "U16","%Pn",0.01f,"RW"},
    {45509, 1,  "cos φ(P) curve: cos φ of third point", "S16", NULL, 0.0001f, "RW"},
    {45510, 1,  "cos φ(P) curve: Active power fourth point", "U16","%Pn",0.01f,"RW"},
    {45511, 1,  "cos φ(P) curve: cos φ of fourth point", "S16", NULL, 0.0001f, "RW"},
    {45512, 1,  "Lock in voltage (for cos φ(P) curve)", "U16","%Un",0.01f,"RW"},
    {45513, 1,  "Lock out voltage (for cos φ(P) curve)", "U16","%Un",0.01f,"RW"},
    {45516, 1,  "Q Set Value",               "S16",       "%Sn", 0.01f, "RW"},
    {45518, 1,  "Q(U) curve: U of the first point", "U16","%Un",0.01f,"RW"},
    {45519, 1,  "Q(U) curve: Q of the first point", "S16","%Sn",0.01f,"RW"},
    {45520, 1,  "Q(U) curve: U of the second point", "U16","%Un",0.01f,"RW"},
    {45521, 1,  "Q(U) curve: Q of the second point", "S16","%Sn",0.01f,"RW"},
    {45522, 1,  "Q(U) curve: U of the third point", "U16","%Un",0.01f,"RW"},
    {45523, 1,  "Q(U) curve: Q of the third point", "S16","%Sn",0.01f,"RW"},
    {45524, 1,  "Q(U) curve: U of the fourth point", "U16","%Un",0.01f,"RW"},
    {45525, 1,  "Q(U) curve: Q of the fourth point", "S16","%Sn",0.01f,"RW"},
    {45526, 1,  "Lock in power (for Q(U) curve)", "U16","%Pn",0.01f,"RW"},
    {45527, 1,  "Lock out power (for Q(U) curve)", "U16","%Pn",0.01f,"RW"},

    {45606, 1,  "LVRT Trigger voltage",      "U16",       "%Un", 0.01f, "RW"},
    {45609, 1,  "LVRT active power limit mode", "E16",    NULL, 1.0f, "RW"},
};

// define count of entries
const size_t aiswei_registers_count = sizeof(aiswei_registers) / sizeof(aiswei_registers[0]);

int aiswei_find_register_index(uint32_t addr_dec) {
    for (size_t i = 0; i < aiswei_registers_count; ++i) {
        uint32_t start = aiswei_registers[i].addr;
        uint32_t end = start + (aiswei_registers[i].length > 0 ? (aiswei_registers[i].length - 1) : 0);
        if (addr_dec >= start && addr_dec <= end) return (int)i;
    }
    return -1;
}

// Convert AISWEI decimal address (e.g. 31001) to Modbus register index:
// strip leading '3' or '4' (equivalent to addr % 10000), then subtract 1.
uint16_t aiswei_dec2reg(uint32_t addr_dec) {
    uint32_t v = addr_dec % 10000;
    if (v == 0) v = 0;
    return static_cast<uint16_t>((v == 0) ? 0 : (v - 1));
}

static void requestAisweiRead(uint8_t slave, uint32_t addr_dec) {
    // determine register index and length from table if present
    int idx = aiswei_find_register_index(addr_dec);
    uint16_t length = 1;
    if (idx >= 0) length = aiswei_registers[idx].length;
    uint16_t reg = aiswei_dec2reg(addr_dec);

    if (addr_dec >= 40000 && addr_dec < 50000) {
        // addresses starting with 4xxxx are holding registers (function code 0x03)
        sendModbusTCPRequest(slave, 0x03, reg, length);
    } else {
        // default to input registers for 3xxxx (function code 0x03)
        sendModbusTCPRequest(slave, 0x03, reg, length);
    }
}

// Input register wrappers
void deviceType(uint8_t slave) { requestAisweiRead(slave, 31001); }
void modbusAddress(uint8_t slave) { requestAisweiRead(slave, 31002); }
void serialNumber(uint8_t slave) { requestAisweiRead(slave, 31003); }
void machineType(uint8_t slave) { requestAisweiRead(slave, 31019); }
void currentGridCode(uint8_t slave) { requestAisweiRead(slave, 31027); }
void ratedPower_W(uint8_t slave) { requestAisweiRead(slave, 31028); }
void softwareVersion(uint8_t slave) { requestAisweiRead(slave, 31030); }
void safetyVersion(uint8_t slave) { requestAisweiRead(slave, 31044); }
void manufacturerName(uint8_t slave) { requestAisweiRead(slave, 31057); }
void brandName(uint8_t slave) { requestAisweiRead(slave, 31065); }

void gridRatedVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31301); }
void gridRatedFrequency_Hz(uint8_t slave) { requestAisweiRead(slave, 31302); }
void eToday_kWh(uint8_t slave) { requestAisweiRead(slave, 31303); }
void eTotal_kWh(uint8_t slave) { requestAisweiRead(slave, 31305); }
void hTotal_H(uint8_t slave) { requestAisweiRead(slave, 31307); }
void deviceState(uint8_t slave) { requestAisweiRead(slave, 31309); }
void connectTime_s(uint8_t slave) { requestAisweiRead(slave, 31310); }

void airTemperature_C(uint8_t slave) { requestAisweiRead(slave, 31311); }
void inverterUphaseTemperature_C(uint8_t slave) { requestAisweiRead(slave, 31312); }
void inverterVphaseTemperature_C(uint8_t slave) { requestAisweiRead(slave, 31313); }
void inverterWphaseTemperature_C(uint8_t slave) { requestAisweiRead(slave, 31314); }
void boostTemperature_C(uint8_t slave) { requestAisweiRead(slave, 31315); }
void bidirectionalDCDCtemperature_C(uint8_t slave) { requestAisweiRead(slave, 31316);}
void busVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31317); }

void pv1Voltage_V(uint8_t slave) { requestAisweiRead(slave, 31319); }
void pv1Current_A(uint8_t slave) { requestAisweiRead(slave, 31320); }
void pv2Voltage_V(uint8_t slave) { requestAisweiRead(slave, 31321); }
void pv2Current_A(uint8_t slave) { requestAisweiRead(slave, 31322); }
void pv3Voltage_V(uint8_t slave) { requestAisweiRead(slave, 31323); }
void pv3Current_A(uint8_t slave) { requestAisweiRead(slave, 31324); }
void pv4Voltage_V(uint8_t slave) { requestAisweiRead(slave, 31325); }
void pv4Current_A(uint8_t slave) { requestAisweiRead(slave, 31326); }
void pv5Voltage_V(uint8_t slave) { requestAisweiRead(slave, 31327); }
void pv5Current_A(uint8_t slave) { requestAisweiRead(slave, 31328); }

void string1Current_A(uint8_t slave) { requestAisweiRead(slave, 31339); }
void string2Current_A(uint8_t slave) { requestAisweiRead(slave, 31340); }
void string3Current_A(uint8_t slave) { requestAisweiRead(slave, 31341); }
void string4Current_A(uint8_t slave) { requestAisweiRead(slave, 31342); }
void string5Current_A(uint8_t slave) { requestAisweiRead(slave, 31343); }
void string6Current_A(uint8_t slave) { requestAisweiRead(slave, 31344); }
void string7Current_A(uint8_t slave) { requestAisweiRead(slave, 31345); }
void string8Current_A(uint8_t slave) { requestAisweiRead(slave, 31346); }
void string9Current_A(uint8_t slave) { requestAisweiRead(slave, 31347); }
void string10Current_A(uint8_t slave) { requestAisweiRead(slave, 31348); }

void L1PhaseVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31359); }
void L1PhaseCurrent_A(uint8_t slave) { requestAisweiRead(slave, 31360); }
void L2PhaseVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31361); }
void L2PhaseCurrent_A(uint8_t slave) { requestAisweiRead(slave, 31362); }
void L3PhaseVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31363); }
void L3PhaseCurrent_A(uint8_t slave) { requestAisweiRead(slave, 31364); }
void RSLineVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31365); }
void RTLineVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31366); }
void STLineVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31367); }

void gridFrequency_Hz(uint8_t slave) { requestAisweiRead(slave, 31368); }
void apparentPower_VA(uint8_t slave) { requestAisweiRead(slave, 31369); }
void activePower_W(uint8_t slave) { requestAisweiRead(slave, 31371); }
void reactivePower_Var(uint8_t slave) { requestAisweiRead(slave, 31373); }
void powerFactor(uint8_t slave) { requestAisweiRead(slave, 31375); }

void errorCode(uint8_t slave) { requestAisweiRead(slave, 31378); }
void warningCode(uint8_t slave) { requestAisweiRead(slave, 31379); }

void pvTotalPower_W(uint8_t slave) { requestAisweiRead(slave, 31601); }
void pvEToday_kWh(uint8_t slave) { requestAisweiRead(slave, 31603); }
void pvETotal_kWh(uint8_t slave) { requestAisweiRead(slave, 31605); }
void batteryCommunicationStatus(uint8_t slave) { requestAisweiRead(slave, 31607); }
void batteryStatus(uint8_t slave) { requestAisweiRead(slave, 31608); }
void batteryErrorStatus(uint8_t slave) { requestAisweiRead(slave, 31609); }
void batteryWarningStatus(uint8_t slave) { requestAisweiRead(slave, 31613); }
void batteryVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31617); }
void batteryCurrent_A(uint8_t slave) { requestAisweiRead(slave, 31618); }
void batteryPower_W(uint8_t slave) { requestAisweiRead(slave, 31619); }
void batteryTemperature_C(uint8_t slave) { requestAisweiRead(slave, 31621); }
void batterySOC(uint8_t slave) { requestAisweiRead(slave, 31622); }
void batterySOH(uint8_t slave) { requestAisweiRead(slave, 31623); }
void batteryChargingCurrentLimit_A(uint8_t slave) { requestAisweiRead(slave, 31624); }
void batteryDischargeCurrentLimit_A(uint8_t slave) { requestAisweiRead(slave, 31625); }
void batteryEChargeToday_kWh(uint8_t slave) { requestAisweiRead(slave, 31626); }
void batteryEDischargeToday_kWh(uint8_t slave) { requestAisweiRead(slave, 31628); }
void eConsumptionToday_AC_kWh(uint8_t slave) { requestAisweiRead(slave, 31630); }
void eGenerationToday_AC_kWh(uint8_t slave) { requestAisweiRead(slave, 31632); }
void EPSLoadVoltage_V(uint8_t slave) { requestAisweiRead(slave, 31634); }
void EPSLoadCurrent_A(uint8_t slave) { requestAisweiRead(slave, 31635); }
void EPSLoadFrequency_Hz(uint8_t slave) { requestAisweiRead(slave, 31636); }
void EPSLoadActivePower_W(uint8_t slave) { requestAisweiRead(slave, 31637); }
void EPSLoadReactivePower_Var(uint8_t slave) { requestAisweiRead(slave, 31639); }
void eConsumptionToday_EPS_kWh(uint8_t slave) { requestAisweiRead(slave, 31641); }
void eConsumptionTotal_EPS_kWh(uint8_t slave) { requestAisweiRead(slave, 31643); }

// Holding register wrappers
void remoteSwitchCommand(uint8_t slave) { requestAisweiRead(slave, 40201); }
void rtc_Year(uint8_t slave) { requestAisweiRead(slave, 41001); }
void rtc_Month(uint8_t slave) { requestAisweiRead(slave, 41002); }
void rtc_Day(uint8_t slave) { requestAisweiRead(slave, 41003); }
void rtc_Hour(uint8_t slave) { requestAisweiRead(slave, 41004); }
void rtc_Minute(uint8_t slave) { requestAisweiRead(slave, 41005); }
void rtc_Seconds(uint8_t slave) { requestAisweiRead(slave, 41006); }

void storageInverterSwitch(uint8_t slave) { requestAisweiRead(slave, 41102); }
void typeSelectionOfEnergyStorageMachine(uint8_t slave) { requestAisweiRead(slave, 41103); }
void runMode(uint8_t slave) { requestAisweiRead(slave, 41104); }
void batteryManufacturer(uint8_t slave) { requestAisweiRead(slave, 41105); }
void smartMeterStatus(uint8_t slave) { requestAisweiRead(slave, 41108); }
void smartMeterAdjustmentFlag(uint8_t slave) { requestAisweiRead(slave, 41109); }
void setTargetPowerValue_W(uint8_t slave) { requestAisweiRead(slave, 41110); }
void currentPowerValueOfSmartMeter_W(uint8_t slave) { requestAisweiRead(slave, 41112); }
void antiReverseCurrentFlag(uint8_t slave) { requestAisweiRead(slave, 41114); }
void batteryWakeUp(uint8_t slave) { requestAisweiRead(slave, 41115); }
void commboxAndCloudCommunicationStatus(uint8_t slave) { requestAisweiRead(slave, 41151); }
void chargeDischargeFlagBit(uint8_t slave) { requestAisweiRead(slave, 41152); }
void chargeAndDischargePowerCommand_W(uint8_t slave) { requestAisweiRead(slave, 41153); }

void activePowerControlFunction(uint8_t slave) { requestAisweiRead(slave, 44001); }
void eegControlFunction(uint8_t slave) { requestAisweiRead(slave, 44002); }
void slopeLoadFunction(uint8_t slave) { requestAisweiRead(slave, 44003); }
void overvoltageReducePowerFunction(uint8_t slave) { requestAisweiRead(slave, 44004); }
void overfrequencyReducePowerFunction(uint8_t slave) { requestAisweiRead(slave, 44005); }
void reactivePowerControlFunction(uint8_t slave) { requestAisweiRead(slave, 44006); }
void LVRTFunction(uint8_t slave) { requestAisweiRead(slave, 44007); }
void tenMinutesAverageOvervoltageProtectFunction(uint8_t slave) { requestAisweiRead(slave, 44009); }
void islandingProtectFunction(uint8_t slave) { requestAisweiRead(slave, 44010); }
void peConnectionCheckFunction(uint8_t slave) { requestAisweiRead(slave, 44012); }
void overloadFunction(uint8_t slave) { requestAisweiRead(slave, 44017); }
void shadowMPPTFunction(uint8_t slave) { requestAisweiRead(slave, 44025); }

void gridCode(uint8_t slave) { requestAisweiRead(slave, 45201); }
void overvoltageProtectionValue3_V(uint8_t slave) { requestAisweiRead(slave, 45202); }
void overvoltageProtectionValue2_V(uint8_t slave) { requestAisweiRead(slave, 45203); }
void overvoltageProtectionValueFreq_Hz(uint8_t slave) { requestAisweiRead(slave, 45204); }
void underfrequencyProtectionValue_Hz(uint8_t slave) { requestAisweiRead(slave, 45205); }
void gridVoltageHighLimit3_V(uint8_t slave) { requestAisweiRead(slave, 45206); }
void gridVoltageHighLimitTime3_ms(uint8_t slave) { requestAisweiRead(slave, 45207); }
void gridVoltageHighLimit2_V(uint8_t slave) { requestAisweiRead(slave, 45209); }
void gridVoltageHighLimitTime2_ms(uint8_t slave) { requestAisweiRead(slave, 45210); }
void gridVoltageHighLimit1_V(uint8_t slave) { requestAisweiRead(slave, 45212); }
void gridVoltageHighLimitTime1_ms(uint8_t slave) { requestAisweiRead(slave, 45213); }
void gridVoltageLowLimit3_V(uint8_t slave) { requestAisweiRead(slave, 45215); }
void gridVoltageLowLimitTime3_ms(uint8_t slave) { requestAisweiRead(slave, 45216); }
void gridVoltageLowLimit2_V(uint8_t slave) { requestAisweiRead(slave, 45218); }
void gridVoltageLowLimitTime2_ms(uint8_t slave) { requestAisweiRead(slave, 45219); }
void gridVoltageLowLimit1_V(uint8_t slave) { requestAisweiRead(slave, 45221); }
void gridVoltageLowLimitTime1_ms(uint8_t slave) { requestAisweiRead(slave, 45222); }
void tenMinutesAverageOvervoltageThreshold_V(uint8_t slave) { requestAisweiRead(slave, 45224); }
void tenMinutesAverageOvervoltageProtectTime_ms(uint8_t slave) { requestAisweiRead(slave, 45225); }
void overvoltageRecoverValue_V(uint8_t slave) { requestAisweiRead(slave, 45226); }
void undervoltageRecoverValue_V(uint8_t slave) { requestAisweiRead(slave, 45227); }
void gridFrequencyHighLimit3_Hz(uint8_t slave) { requestAisweiRead(slave, 45228); }
void gridFrequencyHighLimitTime3_ms(uint8_t slave) { requestAisweiRead(slave, 45229); }
void gridFrequencyHighLimit2_Hz(uint8_t slave) { requestAisweiRead(slave, 45231); }
void gridFrequencyHighLimitTime2_ms(uint8_t slave) { requestAisweiRead(slave, 45232); }
void gridFrequencyHighLimit1_Hz(uint8_t slave) { requestAisweiRead(slave, 45234); }
void gridFrequencyHighLimitTime1_ms(uint8_t slave) { requestAisweiRead(slave, 45235); }
void gridFrequencyLowLimit3_Hz(uint8_t slave) { requestAisweiRead(slave, 45237); }
void gridFrequencyLowLimitTime3_ms(uint8_t slave) { requestAisweiRead(slave, 45238); }
void gridFrequencyLowLimit2_Hz(uint8_t slave) { requestAisweiRead(slave, 45240); }
void gridFrequencyLowLimitTime2_ms(uint8_t slave) { requestAisweiRead(slave, 45241); }
void gridFrequencyLowLimit1_Hz(uint8_t slave) { requestAisweiRead(slave, 45243); }
void gridFrequencyLowLimitTime1_ms(uint8_t slave) { requestAisweiRead(slave, 45244); }
void varyRateOfFrequencyProtectValue_HzPerS(uint8_t slave) { requestAisweiRead(slave, 45246); }
void varyRateOfFrequencyProtectTime_ms(uint8_t slave) { requestAisweiRead(slave, 45247); }
void overfrequencyRecoverValue_Hz(uint8_t slave) { requestAisweiRead(slave, 45249); }
void underfrequencyRecoverValue_Hz(uint8_t slave) { requestAisweiRead(slave, 45250); }
void timeOfFirstConnectionToGrid_s(uint8_t slave) { requestAisweiRead(slave, 45251); }
void timeOfReconnectionToGrid_s(uint8_t slave) { requestAisweiRead(slave, 45252); }
void isoProtectThreshold_kOhm(uint8_t slave) { requestAisweiRead(slave, 45253); }
void dciProtectThreshold_mA(uint8_t slave) { requestAisweiRead(slave, 45254); }
void dciProtectTime_ms(uint8_t slave) { requestAisweiRead(slave, 45255); }

void loadRateOfFirstConnectionToGrid(uint8_t slave) { requestAisweiRead(slave, 45401); }
void loadRateOfReconnectionToGrid(uint8_t slave) { requestAisweiRead(slave, 45402); }
void activePowerSet_percentPn(uint8_t slave) { requestAisweiRead(slave, 45403); }
void increaseRateOfActivePower_percentPnPerMin(uint8_t slave) { requestAisweiRead(slave, 45404); }
void decreaseRateOfActivePower_percentPnPerMin(uint8_t slave) { requestAisweiRead(slave, 45405); }
void overFrequencyReducePowerMode(uint8_t slave) { requestAisweiRead(slave, 45408); }
void overFrequencyReducePowerStartFrequency_Hz(uint8_t slave) { requestAisweiRead(slave, 45409); }
void overFrequencyReducePowerStopFrequency_Hz(uint8_t slave) { requestAisweiRead(slave, 45410); }
void overFrequencyReducePowerBackFrequency_Hz(uint8_t slave) { requestAisweiRead(slave, 45411); }
void reduceRatioOfOverFrequencyReducePower(uint8_t slave) { requestAisweiRead(slave, 45412); }
void overFrequencyReducePowerReduceDelayTime_s(uint8_t slave) { requestAisweiRead(slave, 45413); }
void overFrequencyReducePowerRecoverDelayTime_s(uint8_t slave) { requestAisweiRead(slave, 45414); }
void speedOfOverFrequencyRecoverToPn(uint8_t slave) { requestAisweiRead(slave, 45416); }
void overFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave) { requestAisweiRead(slave, 45417); }

void overVoltageReducePowerMode(uint8_t slave) { requestAisweiRead(slave, 45419); }
void overVoltageReducePowerStartVoltage_percentUn(uint8_t slave) { requestAisweiRead(slave, 45420); }
void overVoltageReducePowerStopVoltage_percentUn(uint8_t slave) { requestAisweiRead(slave, 45422); }
void overVoltageReducePowerBackVoltage_percentUn(uint8_t slave) { requestAisweiRead(slave, 45424); }
void reduceRatioOfOverVoltageReducePower(uint8_t slave) { requestAisweiRead(slave, 45426); }
void overVoltageReducePowerDelayTime_s(uint8_t slave) { requestAisweiRead(slave, 45427); }
void overVoltageRecoverPowerDelayTime_s(uint8_t slave) { requestAisweiRead(slave, 45428); }
void speedOfOverVoltageRecoverToPn(uint8_t slave) { requestAisweiRead(slave, 45429); }

void underFrequencyIncreasePowerMode(uint8_t slave) { requestAisweiRead(slave, 45432); }
void underFrequencyIncreasePowerStartFrequency_Hz(uint8_t slave) { requestAisweiRead(slave, 45433); }
void underFrequencyIncreasePowerStopFrequency_Hz(uint8_t slave) { requestAisweiRead(slave, 45434); }
void underFrequencyIncreasePowerBackFrequency_Hz(uint8_t slave) { requestAisweiRead(slave, 45435); }
void increaseRatioOfUnderFrequencyIncreasePower(uint8_t slave) { requestAisweiRead(slave, 45436); }
void underFrequencyIncreasePowerDelayTime_s(uint8_t slave) { requestAisweiRead(slave, 45437); }
void underFrequencyRecoverPowerDelayTime_s(uint8_t slave) { requestAisweiRead(slave, 45438); }
void speedOfUnderFrequencyRecoverToPn(uint8_t slave) { requestAisweiRead(slave, 45440); }
void underFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave) { requestAisweiRead(slave, 45441); }

void underVoltageIncreasePowerMode(uint8_t slave) { requestAisweiRead(slave, 45443); }
void underVoltageIncreasePowerStartVoltage_percentUn(uint8_t slave) { requestAisweiRead(slave, 45444); }
void underVoltageIncreasePowerStopVoltage_percentUn(uint8_t slave) { requestAisweiRead(slave, 45445); }
void underVoltageIncreasePowerBackVoltage_percentUn(uint8_t slave) { requestAisweiRead(slave, 45446); }
void increaseRatioOfUnderVoltageIncreasePower(uint8_t slave) { requestAisweiRead(slave, 45447); }
void underVoltageIncreasePowerDelayTime_s(uint8_t slave) { requestAisweiRead(slave, 45448); }
void underVoltageIncreasePowerDelayTime2_s(uint8_t slave) { requestAisweiRead(slave, 45449); }
void speedOfUnderVoltageRecoverToPn(uint8_t slave) { requestAisweiRead(slave, 45450); }

void pav_percentPn(uint8_t slave) { requestAisweiRead(slave, 45451); }
void drmsPval_percentPn(uint8_t slave) { requestAisweiRead(slave, 45452); }

void reactivePowerControlMode(uint8_t slave) { requestAisweiRead(slave, 45501); }
void timeConstantOfReactivePowerCurve_s(uint8_t slave) { requestAisweiRead(slave, 45502); }
void pfSetValue(uint8_t slave) { requestAisweiRead(slave, 45503); }
void cosPPowerCurve_point1_activePercentPn(uint8_t slave) { requestAisweiRead(slave, 45504); }
void cosPPowerCurve_point1_cosPhi(uint8_t slave) { requestAisweiRead(slave, 45505); }
void cosPPowerCurve_point2_activePercentPn(uint8_t slave) { requestAisweiRead(slave, 45506); }
void cosPPowerCurve_point2_cosPhi(uint8_t slave) { requestAisweiRead(slave, 45507); }
void cosPPowerCurve_point3_activePercentPn(uint8_t slave) { requestAisweiRead(slave, 45508); }
void cosPPowerCurve_point3_cosPhi(uint8_t slave) { requestAisweiRead(slave, 45509); }
void cosPPowerCurve_point4_activePercentPn(uint8_t slave) { requestAisweiRead(slave, 45510); }
void cosPPowerCurve_point4_cosPhi(uint8_t slave) { requestAisweiRead(slave, 45511); }
void lockInVoltageForCosPPowerCurve_percentUn(uint8_t slave) { requestAisweiRead(slave, 45512); }
void lockOutVoltageForCosPPowerCurve_percentUn(uint8_t slave) { requestAisweiRead(slave, 45513); }
void QSetValue_percentSn(uint8_t slave) { requestAisweiRead(slave, 45516); }
void QU_curve_point1_U_percentUn(uint8_t slave) { requestAisweiRead(slave, 45518); }
void QU_curve_point1_Q_percentSn(uint8_t slave) { requestAisweiRead(slave, 45519); }
void QU_curve_point2_U_percentUn(uint8_t slave) { requestAisweiRead(slave, 45520); }
void QU_curve_point2_Q_percentSn(uint8_t slave) { requestAisweiRead(slave, 45521); }
void QU_curve_point3_U_percentUn(uint8_t slave) { requestAisweiRead(slave, 45522); }
void QU_curve_point3_Q_percentSn(uint8_t slave) { requestAisweiRead(slave, 45523); }
void QU_curve_point4_U_percentUn(uint8_t slave) { requestAisweiRead(slave, 45524); }
void QU_curve_point4_Q_percentSn(uint8_t slave) { requestAisweiRead(slave, 45525); }
void lockInPowerForQU_curve_percentPn(uint8_t slave) { requestAisweiRead(slave, 45526); }
void lockOutPowerForQU_curve_percentPn(uint8_t slave) { requestAisweiRead(slave, 45527); }

void LVRT_TriggerVoltage_percentUn(uint8_t slave) { requestAisweiRead(slave, 45606); }
void LVRT_activePowerLimitMode(uint8_t slave) { requestAisweiRead(slave, 45609); }

/* --- TCP Modbus write helpers --- */
bool sendModbusTCPWriteRequest(uint8_t unitId, uint16_t registerAddress, uint16_t value) {
    if (modbusSocket < 0) {
        printf("Socket not connected\n");
        return false;
    }

    uint8_t frame[12];
    uint16_t tid = transactionId++;
    
    // MBAP Header
    frame[0] = (tid >> 8) & 0xFF;
    frame[1] = tid & 0xFF;
    frame[2] = 0x00;
    frame[3] = 0x00;
    frame[4] = 0x00;
    frame[5] = 0x06;
    frame[6] = unitId;
    
    // PDU (Function Code 0x06 - Write Single Register)
    frame[7] = 0x06;
    frame[8] = (registerAddress >> 8) & 0xFF;
    frame[9] = registerAddress & 0xFF;
    frame[10] = (value >> 8) & 0xFF;
    frame[11] = value & 0xFF;

    if (send(modbusSocket, frame, sizeof(frame), 0) < 0) {
        printf("Failed to send write request\n");
        return false;
    }

    printf("Sent Modbus TCP write: reg=%u, value=%u\n", registerAddress, value);
    return true;
}

bool writeRegisterU16(uint8_t slave, uint32_t addr_dec, uint16_t value) {
    uint16_t reg = aiswei_dec2reg(addr_dec);
    return sendModbusTCPWriteRequest(slave, reg, value);
}

bool writeRegisterU32(uint8_t slave, uint32_t addr_dec, uint32_t value) {
    // For U32, we need to write two consecutive registers
    uint16_t reg = aiswei_dec2reg(addr_dec);
    uint16_t highWord = (value >> 16) & 0xFFFF;
    uint16_t lowWord = value & 0xFFFF;
    
    // Write high word first
    if (!sendModbusTCPWriteRequest(slave, reg, highWord)) return false;
    usleep(50000);
    // Write low word second
    return sendModbusTCPWriteRequest(slave, reg + 1, lowWord);
}

/* --- write wrappers for each RW holding register --- */
bool write_remoteSwitchCommand(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 40201, value); }
bool write_rtc_Year(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41001, value); }
bool write_rtc_Month(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41002, value); }
bool write_rtc_Day(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41003, value); }
bool write_rtc_Hour(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41004, value); }
bool write_rtc_Minute(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41005, value); }
bool write_rtc_Seconds(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41006, value); }

bool write_storageInverterSwitch(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41102, value); }
bool write_typeSelectionOfEnergyStorageMachine(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41103, value); }
bool write_runMode(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41104, value); }
bool write_batteryManufacturer(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41105, value); }
bool write_smartMeterStatus(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41108, value); }
bool write_smartMeterAdjustmentFlag(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41109, value); }
bool write_setTargetPowerValue_W(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 41110, value); }
bool write_currentPowerValueOfSmartMeter_W(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 41112, value); }
bool write_antiReverseCurrentFlag(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41114, value); }
bool write_batteryWakeUp(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41115, value); }
bool write_commboxAndCloudCommunicationStatus(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41151, value); }
bool write_chargeDischargeFlagBit(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41152, value); }
bool write_chargeAndDischargePowerCommand_W(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 41153, value); }

bool write_activePowerControlFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44001, value); }
bool write_eegControlFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44002, value); }
bool write_slopeLoadFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44003, value); }
bool write_overvoltageReducePowerFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44004, value); }
bool write_overfrequencyReducePowerFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44005, value); }
bool write_reactivePowerControlFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44006, value); }
bool write_LVRTFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44007, value); }
bool write_tenMinutesAverageOvervoltageProtectFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44009, value); }
bool write_islandingProtectFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44010, value); }
bool write_peConnectionCheckFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44012, value); }
bool write_overloadFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44017, value); }
bool write_shadowMPPTFunction(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 44025, value); }

bool write_gridCode(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45201, value); }
bool write_overvoltageProtectionValue3_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45202, value); }
bool write_overvoltageProtectionValue2_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45203, value); }
bool write_overvoltageProtectionValueFreq_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45204, value); }
bool write_underfrequencyProtectionValue_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45205, value); }
bool write_gridVoltageHighLimit3_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45206, value); }
bool write_gridVoltageHighLimitTime3_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45207, value); }
bool write_gridVoltageHighLimit2_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45209, value); }
bool write_gridVoltageHighLimitTime2_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45210, value); }
bool write_gridVoltageHighLimit1_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45212, value); }
bool write_gridVoltageHighLimitTime1_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45213, value); }
bool write_gridVoltageLowLimit3_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45215, value); }
bool write_gridVoltageLowLimitTime3_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45216, value); }
bool write_gridVoltageLowLimit2_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45218, value); }
bool write_gridVoltageLowLimitTime2_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45219, value); }
bool write_gridVoltageLowLimit1_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45221, value); }
bool write_gridVoltageLowLimitTime1_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45222, value); }
bool write_tenMinutesAverageOvervoltageThreshold_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45224, value); }
bool write_tenMinutesAverageOvervoltageProtectTime_ms(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45225, value); }
bool write_overvoltageRecoverValue_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45226, value); }
bool write_undervoltageRecoverValue_V(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45227, value); }
bool write_gridFrequencyHighLimit3_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45228, value); }
bool write_gridFrequencyHighLimitTime3_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45229, value); }
bool write_gridFrequencyHighLimit2_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45231, value); }
bool write_gridFrequencyHighLimitTime2_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45232, value); }
bool write_gridFrequencyHighLimit1_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45234, value); }
bool write_gridFrequencyHighLimitTime1_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45235, value); }
bool write_gridFrequencyLowLimit3_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45237, value); }
bool write_gridFrequencyLowLimitTime3_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45238, value); }
bool write_gridFrequencyLowLimit2_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45240, value); }
bool write_gridFrequencyLowLimitTime2_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45241, value); }
bool write_gridFrequencyLowLimit1_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45243, value); }
bool write_gridFrequencyLowLimitTime1_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45244, value); }
bool write_varyRateOfFrequencyProtectValue_HzPerS(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45246, value); }
bool write_varyRateOfFrequencyProtectTime_ms(uint8_t slave, uint32_t value) { return writeRegisterU32(slave, 45247, value); }
bool write_overfrequencyRecoverValue_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45249, value); }
bool write_underfrequencyRecoverValue_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45250, value); }
bool write_timeOfFirstConnectionToGrid_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45251, value); }
bool write_timeOfReconnectionToGrid_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45252, value); }
bool write_isoProtectThreshold_kOhm(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45253, value); }
bool write_dciProtectThreshold_mA(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45254, value); }
bool write_dciProtectTime_ms(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45255, value); }

bool write_loadRateOfFirstConnectionToGrid(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45401, value); }
bool write_loadRateOfReconnectionToGrid(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45402, value); }
bool write_activePowerSet_percentPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45403, value); }
bool write_increaseRateOfActivePower_percentPnPerMin(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45404, value); }
bool write_decreaseRateOfActivePower_percentPnPerMin(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45405, value); }
bool write_overFrequencyReducePowerMode(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45408, value); }
bool write_overFrequencyReducePowerStartFrequency_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45409, value); }
bool write_overFrequencyReducePowerStopFrequency_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45410, value); }
bool write_overFrequencyReducePowerBackFrequency_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45411, value); }
bool write_reduceRatioOfOverFrequencyReducePower(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45412, value); }
bool write_overFrequencyReducePowerReduceDelayTime_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45413, value); }
bool write_overFrequencyReducePowerRecoverDelayTime_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45414, value); }
bool write_speedOfOverFrequencyRecoverToPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45416, value); }
bool write_overFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45417, value); }

bool write_overVoltageReducePowerMode(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45419, value); }
bool write_overVoltageReducePowerStartVoltage_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45420, value); }
bool write_overVoltageReducePowerStopVoltage_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45422, value); }
bool write_overVoltageReducePowerBackVoltage_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45424, value); }
bool write_reduceRatioOfOverVoltageReducePower(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45426, value); }
bool write_overVoltageReducePowerDelayTime_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45427, value); }
bool write_overVoltageRecoverPowerDelayTime_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45428, value); }
bool write_speedOfOverVoltageRecoverToPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45429, value); }

bool write_underFrequencyIncreasePowerMode(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45432, value); }
bool write_underFrequencyIncreasePowerStartFrequency_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45433, value); }
bool write_underFrequencyIncreasePowerStopFrequency_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45434, value); }
bool write_underFrequencyIncreasePowerBackFrequency_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45435, value); }
bool write_increaseRatioOfUnderFrequencyIncreasePower(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45436, value); }
bool write_underFrequencyIncreasePowerDelayTime_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45437, value); }
bool write_underFrequencyRecoverPowerDelayTime_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45438, value); }
bool write_speedOfUnderFrequencyRecoverToPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45440, value); }
bool write_underFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45441, value); }

bool write_underVoltageIncreasePowerMode(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45443, value); }
bool write_underVoltageIncreasePowerStartVoltage_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45444, value); }
bool write_underVoltageIncreasePowerStopVoltage_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45445, value); }
bool write_underVoltageIncreasePowerBackVoltage_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45446, value); }
bool write_increaseRatioOfUnderVoltageIncreasePower(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45447, value); }
bool write_underVoltageIncreasePowerDelayTime_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45448, value); }
bool write_underVoltageIncreasePowerDelayTime2_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45449, value); }
bool write_speedOfUnderVoltageRecoverToPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45450, value); }

bool write_pav_percentPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45451, value); }
bool write_drmsPval_percentPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45452, value); }

bool write_reactivePowerControlMode(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45501, value); }
bool write_timeConstantOfReactivePowerCurve_s(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45502, value); }
bool write_pfSetValue(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45503, value); }
bool write_cosPPowerCurve_point1_activePercentPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45504, value); }
bool write_cosPPowerCurve_point1_cosPhi(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45505, value); }
bool write_cosPPowerCurve_point2_activePercentPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45506, value); }
bool write_cosPPowerCurve_point2_cosPhi(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45507, value); }
bool write_cosPPowerCurve_point3_activePercentPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45508, value); }
bool write_cosPPowerCurve_point3_cosPhi(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45509, value); }
bool write_cosPPowerCurve_point4_activePercentPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45510, value); }
bool write_cosPPowerCurve_point4_cosPhi(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45511, value); }
bool write_lockInVoltageForCosPPowerCurve_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45512, value); }
bool write_lockOutVoltageForCosPPowerCurve_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45513, value); }
bool write_QSetValue_percentSn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45516, value); }
bool write_QU_curve_point1_U_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45518, value); }
bool write_QU_curve_point1_Q_percentSn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45519, value); }
bool write_QU_curve_point2_U_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45520, value); }
bool write_QU_curve_point2_Q_percentSn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45521, value); }
bool write_QU_curve_point3_U_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45522, value); }
bool write_QU_curve_point3_Q_percentSn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45523, value); }
bool write_QU_curve_point4_U_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45524, value); }
bool write_QU_curve_point4_Q_percentSn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45525, value); }
bool write_lockInPowerForQU_curve_percentPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45526, value); }
bool write_lockOutPowerForQU_curve_percentPn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45527, value); }

bool write_LVRT_TriggerVoltage_percentUn(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45606, value); }
bool write_LVRT_activePowerLimitMode(uint8_t slave, uint16_t value) { return writeRegisterU16(slave, 45609, value); }