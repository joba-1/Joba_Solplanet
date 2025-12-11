#include "modbus_registers.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <unistd.h>
#include <stdio.h>

// Logging helper
#define LOG(fmt, ...) printf("[%s] " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

// Modbus TCP configuration
#include "modbus_config.h"


// ModbusTCP socket handle
int modbusSocket = -1;

static uint16_t transactionId = 0;   // id of current transaction to match register responses
static uint16_t transactionReg = 0;  // first register address of current transaction

// decode a single Modbus response and publish a human friendly payload to MQTT (defined in main.cpp)
void decodeAndPublish(uint8_t serverAddress, uint16_t reg, uint8_t* data, size_t length);

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
    uint16_t v = addr_dec % 10000;
    return (v == 0) ? 9999 : (v - 1);
}

// Modbus TCP connection management
static bool connectModbusTCP() {
    if (modbusSocket != -1) {
        return true;  // already connected
    }

    const char *server = MODBUS_SERVER;
    int port = MODBUS_PORT;

    struct hostent* host = gethostbyname(server);
    if (!host) {
        LOG("Failed to resolve hostname: %s", server);
        return false;
    }

    modbusSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (modbusSocket < 0) {
        LOG("Failed to create socket");
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = ((struct in_addr*)host->h_addr)->s_addr;

    if (connect(modbusSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG("Failed to connect to Modbus TCP server %s:%d", server, port);
        close(modbusSocket);
        modbusSocket = -1;
        return false;
    }

    LOG("Connected to Modbus TCP server %s:%d", server, port);
    return true;
}

void cleanupModbusTCP() {
    if (modbusSocket != -1) {
        close(modbusSocket);
        modbusSocket = -1;
        LOG("Modbus TCP connection closed");
    }
}

// Modbus TCP read request builder and sender (no address translation)
static bool sendModbusTCPRequest(uint8_t unitId, uint8_t functionCode, uint16_t startAddress, uint16_t quantity) {
    if (!connectModbusTCP()) {
        return false;
    }

    // Build Modbus TCP frame
    uint8_t frame[12];
    uint16_t tid = ++transactionId;
    transactionReg = startAddress;
    
    // MBAP Header (7 bytes)
    frame[0] = (tid >> 8) & 0xFF;           // Transaction ID (high)
    frame[1] = tid & 0xFF;                  // Transaction ID (low)
    frame[2] = 0x00;                        // Protocol ID (high) = 0
    frame[3] = 0x00;                        // Protocol ID (low) = 0
    frame[4] = 0x00;                        // Length (high) = 6 bytes
    frame[5] = 0x06;                        // Length (low)
    frame[6] = unitId;                      // Unit ID
    
    // PDU (5 bytes)
    frame[7] = functionCode;                // Function code (0x03 = Read Holding Registers)
    frame[8] = (startAddress >> 8) & 0xFF;  // Starting Address (high)
    frame[9] = startAddress & 0xFF;         // Starting Address (low)
    frame[10] = (quantity >> 8) & 0xFF;     // Quantity (high)
    frame[11] = quantity & 0xFF;            // Quantity (low)

    if (send(modbusSocket, frame, sizeof(frame), 0) < 0) {
        LOG("Failed to send Modbus TCP request");
        close(modbusSocket);
        modbusSocket = -1;
        return false;
    }

    LOG("Sent Modbus TCP request: unitId=%u, fc=0x%02x, addr=%u, qty=%u", unitId, functionCode, startAddress, quantity);
    return true;
}

// Modbus TCP write word request builder and sender (no address translation)
static bool sendModbusTCPWriteRequest(uint8_t unitId, uint16_t registerAddress, uint16_t value) {
    if (!connectModbusTCP()) {
        return false;
    }

    uint8_t frame[12];
    uint16_t tid = ++transactionId;
    transactionReg = registerAddress;

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
        LOG("Failed to send write request\n");
        return false;
    }

    LOG("Sent Modbus TCP write: reg=%u, value=%u\n", registerAddress, value);
    return true;
}


static bool requestAisweiRead(uint8_t slave, uint32_t addr_dec) {
    // determine register index and length from table if present
    int idx = aiswei_find_register_index(addr_dec);
    uint16_t length = 1;
    if (idx >= 0) length = aiswei_registers[idx].length;
    uint16_t reg = aiswei_dec2reg(addr_dec);

    if (idx < 0) {
        LOG("Unknown modbus address: %u", addr_dec);
        return false;
    }

    if (addr_dec >= 40000 && addr_dec < 50000) {
        // addresses starting with 4xxxx are holding registers (function code 0x03)
        return sendModbusTCPRequest(slave, 0x03, reg, length);
    } 
    
    // default to input registers for 3xxxx (function code 0x04)
    return sendModbusTCPRequest(slave, 0x04, reg, length);
}


static bool requestAisweiWriteWord(uint8_t slave, uint32_t addr_dec, uint16_t value) {
    uint16_t reg = aiswei_dec2reg(addr_dec);
    return sendModbusTCPWriteRequest(slave, reg, value);
}

static bool requestAisweiWriteDWord(uint8_t slave, uint32_t addr_dec, uint32_t value) {
    // For U32, we need to write two consecutive registers
    uint16_t reg = aiswei_dec2reg(addr_dec);
    uint16_t highWord = (value >> 16) & 0xFFFF;
    uint16_t lowWord = value & 0xFFFF;
    
    // Write high word first
    if (!sendModbusTCPWriteRequest(slave, reg, highWord)) return false;
    usleep(50000); // 50ms delay between writes
    // Write low word second
    return sendModbusTCPWriteRequest(slave, reg + 1, lowWord);
}


// Modbus TCP response parser
void parseModbusTCPResponse() {
    if (modbusSocket < 0) return;

    uint8_t buffer[256];
    int bytesRead = recv(modbusSocket, buffer, sizeof(buffer), 0);

    if (bytesRead < 0) {
        LOG("Failed to read from socket");
        close(modbusSocket);
        modbusSocket = -1;
        return;
    }

    if (bytesRead == 0) {
        LOG("Connection closed by server");
        close(modbusSocket);
        modbusSocket = -1;
        return;
    }

    if (bytesRead < 9) {
        LOG("Response too short: %d bytes", bytesRead);
        return;
    }

    // Parse MBAP Header
    uint16_t tid = ((uint16_t)buffer[0] << 8) | buffer[1];
    uint16_t pid = ((uint16_t)buffer[2] << 8) | buffer[3];
    uint16_t len = ((uint16_t)buffer[4] << 8) | buffer[5];
    uint8_t unitId = buffer[6];
    uint8_t fc = buffer[7];

    if (pid != 0x0000) {
        LOG("Invalid Protocol ID: 0x%04x", pid);
        return;
    }

    if (tid != transactionId) {
        LOG("Transaction ID mismatch: expected %u, got %u", transactionId, tid);
        return;
    }

    if (len != bytesRead - 6) {
        LOG("Length mismatch: expected %u, got %d", len, bytesRead - 6);
        return;
    }

    // Check for exception response (bit 7 set)
    if (fc & 0x80) {
        uint8_t exceptionCode = buffer[8];
        LOG("Modbus exception: fc=0x%02x, exception=0x%02x", fc, exceptionCode);
        return;
    }

    // Parse PDU for function code 0x03 (Read Holding or Input Registers or WriteSingleRegister)
    if (fc == 0x03 || fc == 0x04 || fc == 0x06) {
        uint16_t dataBytes = buffer[8];
        if (bytesRead < 9 + dataBytes) {
            LOG("Response data incomplete");
            return;
        }

        uint8_t* registerData = &buffer[9];
        
        printf("[parseModbusTCPResponse] id 0x%02x fc 0x%02x len %u: 0x", unitId, fc, dataBytes);
        for (int i = 0; i < dataBytes; ++i) {
            printf("%02x", registerData[i]);
        }
        printf("\n");

        // Decode and publish
        decodeAndPublish(unitId, transactionReg, registerData, dataBytes);
    }
}


// Input register wrappers
bool deviceType(uint8_t slave) { return requestAisweiRead(slave, 31001); }
bool modbusAddress(uint8_t slave) { return requestAisweiRead(slave, 31002); }
bool serialNumber(uint8_t slave) { return requestAisweiRead(slave, 31003); }
bool machineType(uint8_t slave) { return requestAisweiRead(slave, 31019); }
bool currentGridCode(uint8_t slave) { return requestAisweiRead(slave, 31027); }
bool ratedPower_W(uint8_t slave) { return requestAisweiRead(slave, 31028); }
bool softwareVersion(uint8_t slave) { return requestAisweiRead(slave, 31030); }
bool safetyVersion(uint8_t slave) { return requestAisweiRead(slave, 31044); }
bool manufacturerName(uint8_t slave) { return requestAisweiRead(slave, 31057); }
bool brandName(uint8_t slave) { return requestAisweiRead(slave, 31065); }

bool gridRatedVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31301); }
bool gridRatedFrequency_Hz(uint8_t slave) { return requestAisweiRead(slave, 31302); }
bool eToday_kWh(uint8_t slave) { return requestAisweiRead(slave, 31303); }
bool eTotal_kWh(uint8_t slave) { return requestAisweiRead(slave, 31305); }
bool hTotal_H(uint8_t slave) { return requestAisweiRead(slave, 31307); }
bool deviceState(uint8_t slave) { return requestAisweiRead(slave, 31309); }
bool connectTime_s(uint8_t slave) { return requestAisweiRead(slave, 31310); }

bool airTemperature_C(uint8_t slave) { return requestAisweiRead(slave, 31311); }
bool inverterUphaseTemperature_C(uint8_t slave) { return requestAisweiRead(slave, 31312); }
bool inverterVphaseTemperature_C(uint8_t slave) { return requestAisweiRead(slave, 31313); }
bool inverterWphaseTemperature_C(uint8_t slave) { return requestAisweiRead(slave, 31314); }
bool boostTemperature_C(uint8_t slave) { return requestAisweiRead(slave, 31315); }
bool bidirectionalDCDCtemperature_C(uint8_t slave) { return requestAisweiRead(slave, 31316);}
bool busVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31317); }

bool pv1Voltage_V(uint8_t slave) { return requestAisweiRead(slave, 31319); }
bool pv1Current_A(uint8_t slave) { return requestAisweiRead(slave, 31320); }
bool pv2Voltage_V(uint8_t slave) { return requestAisweiRead(slave, 31321); }
bool pv2Current_A(uint8_t slave) { return requestAisweiRead(slave, 31322); }
bool pv3Voltage_V(uint8_t slave) { return requestAisweiRead(slave, 31323); }
bool pv3Current_A(uint8_t slave) { return requestAisweiRead(slave, 31324); }
bool pv4Voltage_V(uint8_t slave) { return requestAisweiRead(slave, 31325); }
bool pv4Current_A(uint8_t slave) { return requestAisweiRead(slave, 31326); }
bool pv5Voltage_V(uint8_t slave) { return requestAisweiRead(slave, 31327); }
bool pv5Current_A(uint8_t slave) { return requestAisweiRead(slave, 31328); }

bool string1Current_A(uint8_t slave) { return requestAisweiRead(slave, 31339); }
bool string2Current_A(uint8_t slave) { return requestAisweiRead(slave, 31340); }
bool string3Current_A(uint8_t slave) { return requestAisweiRead(slave, 31341); }
bool string4Current_A(uint8_t slave) { return requestAisweiRead(slave, 31342); }
bool string5Current_A(uint8_t slave) { return requestAisweiRead(slave, 31343); }
bool string6Current_A(uint8_t slave) { return requestAisweiRead(slave, 31344); }
bool string7Current_A(uint8_t slave) { return requestAisweiRead(slave, 31345); }
bool string8Current_A(uint8_t slave) { return requestAisweiRead(slave, 31346); }
bool string9Current_A(uint8_t slave) { return requestAisweiRead(slave, 31347); }
bool string10Current_A(uint8_t slave) { return requestAisweiRead(slave, 31348); }

bool L1PhaseVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31359); }
bool L1PhaseCurrent_A(uint8_t slave) { return requestAisweiRead(slave, 31360); }
bool L2PhaseVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31361); }
bool L2PhaseCurrent_A(uint8_t slave) { return requestAisweiRead(slave, 31362); }
bool L3PhaseVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31363); }
bool L3PhaseCurrent_A(uint8_t slave) { return requestAisweiRead(slave, 31364); }
bool RSLineVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31365); }
bool RTLineVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31366); }
bool STLineVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31367); }

bool gridFrequency_Hz(uint8_t slave) { return requestAisweiRead(slave, 31368); }
bool apparentPower_VA(uint8_t slave) { return requestAisweiRead(slave, 31369); }
bool activePower_W(uint8_t slave) { return requestAisweiRead(slave, 31371); }
bool reactivePower_Var(uint8_t slave) { return requestAisweiRead(slave, 31373); }
bool powerFactor(uint8_t slave) { return requestAisweiRead(slave, 31375); }

bool errorCode(uint8_t slave) { return requestAisweiRead(slave, 31378); }
bool warningCode(uint8_t slave) { return requestAisweiRead(slave, 31379); }

bool pvTotalPower_W(uint8_t slave) { return requestAisweiRead(slave, 31601); }
bool pvEToday_kWh(uint8_t slave) { return requestAisweiRead(slave, 31603); }
bool pvETotal_kWh(uint8_t slave) { return requestAisweiRead(slave, 31605); }
bool batteryCommunicationStatus(uint8_t slave) { return requestAisweiRead(slave, 31607); }
bool batteryStatus(uint8_t slave) { return requestAisweiRead(slave, 31608); }
bool batteryErrorStatus(uint8_t slave) { return requestAisweiRead(slave, 31609); }
bool batteryWarningStatus(uint8_t slave) { return requestAisweiRead(slave, 31613); }
bool batteryVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31617); }
bool batteryCurrent_A(uint8_t slave) { return requestAisweiRead(slave, 31618); }
bool batteryPower_W(uint8_t slave) { return requestAisweiRead(slave, 31619); }
bool batteryTemperature_C(uint8_t slave) { return requestAisweiRead(slave, 31621); }
bool batterySOC(uint8_t slave) { return requestAisweiRead(slave, 31622); }
bool batterySOH(uint8_t slave) { return requestAisweiRead(slave, 31623); }
bool batteryChargingCurrentLimit_A(uint8_t slave) { return requestAisweiRead(slave, 31624); }
bool batteryDischargeCurrentLimit_A(uint8_t slave) { return requestAisweiRead(slave, 31625); }
bool batteryEChargeToday_kWh(uint8_t slave) { return requestAisweiRead(slave, 31626); }
bool batteryEDischargeToday_kWh(uint8_t slave) { return requestAisweiRead(slave, 31628); }
bool eConsumptionToday_AC_kWh(uint8_t slave) { return requestAisweiRead(slave, 31630); }
bool eGenerationToday_AC_kWh(uint8_t slave) { return requestAisweiRead(slave, 31632); }
bool EPSLoadVoltage_V(uint8_t slave) { return requestAisweiRead(slave, 31634); }
bool EPSLoadCurrent_A(uint8_t slave) { return requestAisweiRead(slave, 31635); }
bool EPSLoadFrequency_Hz(uint8_t slave) { return requestAisweiRead(slave, 31636); }
bool EPSLoadActivePower_W(uint8_t slave) { return requestAisweiRead(slave, 31637); }
bool EPSLoadReactivePower_Var(uint8_t slave) { return requestAisweiRead(slave, 31639); }
bool eConsumptionToday_EPS_kWh(uint8_t slave) { return requestAisweiRead(slave, 31641); }
bool eConsumptionTotal_EPS_kWh(uint8_t slave) { return requestAisweiRead(slave, 31643); }

// Holding register wrappers
bool remoteSwitchCommand(uint8_t slave) { return requestAisweiRead(slave, 40201); }
bool rtc_Year(uint8_t slave) { return requestAisweiRead(slave, 41001); }
bool rtc_Month(uint8_t slave) { return requestAisweiRead(slave, 41002); }
bool rtc_Day(uint8_t slave) { return requestAisweiRead(slave, 41003); }
bool rtc_Hour(uint8_t slave) { return requestAisweiRead(slave, 41004); }
bool rtc_Minute(uint8_t slave) { return requestAisweiRead(slave, 41005); }
bool rtc_Seconds(uint8_t slave) { return requestAisweiRead(slave, 41006); }

bool storageInverterSwitch(uint8_t slave) { return requestAisweiRead(slave, 41102); }
bool typeSelectionOfEnergyStorageMachine(uint8_t slave) { return requestAisweiRead(slave, 41103); }
bool runMode(uint8_t slave) { return requestAisweiRead(slave, 41104); }
bool batteryManufacturer(uint8_t slave) { return requestAisweiRead(slave, 41105); }
bool smartMeterStatus(uint8_t slave) { return requestAisweiRead(slave, 41108); }
bool smartMeterAdjustmentFlag(uint8_t slave) { return requestAisweiRead(slave, 41109); }
bool setTargetPowerValue_W(uint8_t slave) { return requestAisweiRead(slave, 41110); }
bool currentPowerValueOfSmartMeter_W(uint8_t slave) { return requestAisweiRead(slave, 41112); }
bool antiReverseCurrentFlag(uint8_t slave) { return requestAisweiRead(slave, 41114); }
bool batteryWakeUp(uint8_t slave) { return requestAisweiRead(slave, 41115); }
bool commboxAndCloudCommunicationStatus(uint8_t slave) { return requestAisweiRead(slave, 41151); }
bool chargeDischargeFlagBit(uint8_t slave) { return requestAisweiRead(slave, 41152); }
bool chargeAndDischargePowerCommand_W(uint8_t slave) { return requestAisweiRead(slave, 41153); }

bool activePowerControlFunction(uint8_t slave) { return requestAisweiRead(slave, 44001); }
bool eegControlFunction(uint8_t slave) { return requestAisweiRead(slave, 44002); }
bool slopeLoadFunction(uint8_t slave) { return requestAisweiRead(slave, 44003); }
bool overvoltageReducePowerFunction(uint8_t slave) { return requestAisweiRead(slave, 44004); }
bool overfrequencyReducePowerFunction(uint8_t slave) { return requestAisweiRead(slave, 44005); }
bool reactivePowerControlFunction(uint8_t slave) { return requestAisweiRead(slave, 44006); }
bool LVRTFunction(uint8_t slave) { return requestAisweiRead(slave, 44007); }
bool tenMinutesAverageOvervoltageProtectFunction(uint8_t slave) { return requestAisweiRead(slave, 44009); }
bool islandingProtectFunction(uint8_t slave) { return requestAisweiRead(slave, 44010); }
bool peConnectionCheckFunction(uint8_t slave) { return requestAisweiRead(slave, 44012); }
bool overloadFunction(uint8_t slave) { return requestAisweiRead(slave, 44017); }
bool shadowMPPTFunction(uint8_t slave) { return requestAisweiRead(slave, 44025); }

bool gridCode(uint8_t slave) { return requestAisweiRead(slave, 45201); }
bool overvoltageProtectionValue3_V(uint8_t slave) { return requestAisweiRead(slave, 45202); }
bool overvoltageProtectionValue2_V(uint8_t slave) { return requestAisweiRead(slave, 45203); }
bool overvoltageProtectionValueFreq_Hz(uint8_t slave) { return requestAisweiRead(slave, 45204); }
bool underfrequencyProtectionValue_Hz(uint8_t slave) { return requestAisweiRead(slave, 45205); }
bool gridVoltageHighLimit3_V(uint8_t slave) { return requestAisweiRead(slave, 45206); }
bool gridVoltageHighLimitTime3_ms(uint8_t slave) { return requestAisweiRead(slave, 45207); }
bool gridVoltageHighLimit2_V(uint8_t slave) { return requestAisweiRead(slave, 45209); }
bool gridVoltageHighLimitTime2_ms(uint8_t slave) { return requestAisweiRead(slave, 45210); }
bool gridVoltageHighLimit1_V(uint8_t slave) { return requestAisweiRead(slave, 45212); }
bool gridVoltageHighLimitTime1_ms(uint8_t slave) { return requestAisweiRead(slave, 45213); }
bool gridVoltageLowLimit3_V(uint8_t slave) { return requestAisweiRead(slave, 45215); }
bool gridVoltageLowLimitTime3_ms(uint8_t slave) { return requestAisweiRead(slave, 45216); }
bool gridVoltageLowLimit2_V(uint8_t slave) { return requestAisweiRead(slave, 45218); }
bool gridVoltageLowLimitTime2_ms(uint8_t slave) { return requestAisweiRead(slave, 45219); }
bool gridVoltageLowLimit1_V(uint8_t slave) { return requestAisweiRead(slave, 45221); }
bool gridVoltageLowLimitTime1_ms(uint8_t slave) { return requestAisweiRead(slave, 45222); }
bool tenMinutesAverageOvervoltageThreshold_V(uint8_t slave) { return requestAisweiRead(slave, 45224); }
bool tenMinutesAverageOvervoltageProtectTime_ms(uint8_t slave) { return requestAisweiRead(slave, 45225); }
bool overvoltageRecoverValue_V(uint8_t slave) { return requestAisweiRead(slave, 45226); }
bool undervoltageRecoverValue_V(uint8_t slave) { return requestAisweiRead(slave, 45227); }
bool gridFrequencyHighLimit3_Hz(uint8_t slave) { return requestAisweiRead(slave, 45228); }
bool gridFrequencyHighLimitTime3_ms(uint8_t slave) { return requestAisweiRead(slave, 45229); }
bool gridFrequencyHighLimit2_Hz(uint8_t slave) { return requestAisweiRead(slave, 45231); }
bool gridFrequencyHighLimitTime2_ms(uint8_t slave) { return requestAisweiRead(slave, 45232); }
bool gridFrequencyHighLimit1_Hz(uint8_t slave) { return requestAisweiRead(slave, 45234); }
bool gridFrequencyHighLimitTime1_ms(uint8_t slave) { return requestAisweiRead(slave, 45235); }
bool gridFrequencyLowLimit3_Hz(uint8_t slave) { return requestAisweiRead(slave, 45237); }
bool gridFrequencyLowLimitTime3_ms(uint8_t slave) { return requestAisweiRead(slave, 45238); }
bool gridFrequencyLowLimit2_Hz(uint8_t slave) { return requestAisweiRead(slave, 45240); }
bool gridFrequencyLowLimitTime2_ms(uint8_t slave) { return requestAisweiRead(slave, 45241); }
bool gridFrequencyLowLimit1_Hz(uint8_t slave) { return requestAisweiRead(slave, 45243); }
bool gridFrequencyLowLimitTime1_ms(uint8_t slave) { return requestAisweiRead(slave, 45244); }
bool varyRateOfFrequencyProtectValue_HzPerS(uint8_t slave) { return requestAisweiRead(slave, 45246); }
bool varyRateOfFrequencyProtectTime_ms(uint8_t slave) { return requestAisweiRead(slave, 45247); }
bool overfrequencyRecoverValue_Hz(uint8_t slave) { return requestAisweiRead(slave, 45249); }
bool underfrequencyRecoverValue_Hz(uint8_t slave) { return requestAisweiRead(slave, 45250); }
bool timeOfFirstConnectionToGrid_s(uint8_t slave) { return requestAisweiRead(slave, 45251); }
bool timeOfReconnectionToGrid_s(uint8_t slave) { return requestAisweiRead(slave, 45252); }
bool isoProtectThreshold_kOhm(uint8_t slave) { return requestAisweiRead(slave, 45253); }
bool dciProtectThreshold_mA(uint8_t slave) { return requestAisweiRead(slave, 45254); }
bool dciProtectTime_ms(uint8_t slave) { return requestAisweiRead(slave, 45255); }

bool loadRateOfFirstConnectionToGrid(uint8_t slave) { return requestAisweiRead(slave, 45401); }
bool loadRateOfReconnectionToGrid(uint8_t slave) { return requestAisweiRead(slave, 45402); }
bool activePowerSet_percentPn(uint8_t slave) { return requestAisweiRead(slave, 45403); }
bool increaseRateOfActivePower_percentPnPerMin(uint8_t slave) { return requestAisweiRead(slave, 45404); }
bool decreaseRateOfActivePower_percentPnPerMin(uint8_t slave) { return requestAisweiRead(slave, 45405); }
bool overFrequencyReducePowerMode(uint8_t slave) { return requestAisweiRead(slave, 45408); }
bool overFrequencyReducePowerStartFrequency_Hz(uint8_t slave) { return requestAisweiRead(slave, 45409); }
bool overFrequencyReducePowerStopFrequency_Hz(uint8_t slave) { return requestAisweiRead(slave, 45410); }
bool overFrequencyReducePowerBackFrequency_Hz(uint8_t slave) { return requestAisweiRead(slave, 45411); }
bool reduceRatioOfOverFrequencyReducePower(uint8_t slave) { return requestAisweiRead(slave, 45412); }
bool overFrequencyReducePowerReduceDelayTime_s(uint8_t slave) { return requestAisweiRead(slave, 45413); }
bool overFrequencyReducePowerRecoverDelayTime_s(uint8_t slave) { return requestAisweiRead(slave, 45414); }
bool speedOfOverFrequencyRecoverToPn(uint8_t slave) { return requestAisweiRead(slave, 45416); }
bool overFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave) { return requestAisweiRead(slave, 45417); }

bool overVoltageReducePowerMode(uint8_t slave) { return requestAisweiRead(slave, 45419); }
bool overVoltageReducePowerStartVoltage_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45420); }
bool overVoltageReducePowerStopVoltage_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45422); }
bool overVoltageReducePowerBackVoltage_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45424); }
bool reduceRatioOfOverVoltageReducePower(uint8_t slave) { return requestAisweiRead(slave, 45426); }
bool overVoltageReducePowerDelayTime_s(uint8_t slave) { return requestAisweiRead(slave, 45427); }
bool overVoltageRecoverPowerDelayTime_s(uint8_t slave) { return requestAisweiRead(slave, 45428); }
bool speedOfOverVoltageRecoverToPn(uint8_t slave) { return requestAisweiRead(slave, 45429); }

bool underFrequencyIncreasePowerMode(uint8_t slave) { return requestAisweiRead(slave, 45432); }
bool underFrequencyIncreasePowerStartFrequency_Hz(uint8_t slave) { return requestAisweiRead(slave, 45433); }
bool underFrequencyIncreasePowerStopFrequency_Hz(uint8_t slave) { return requestAisweiRead(slave, 45434); }
bool underFrequencyIncreasePowerBackFrequency_Hz(uint8_t slave) { return requestAisweiRead(slave, 45435); }
bool increaseRatioOfUnderFrequencyIncreasePower(uint8_t slave) { return requestAisweiRead(slave, 45436); }
bool underFrequencyIncreasePowerDelayTime_s(uint8_t slave) { return requestAisweiRead(slave, 45437); }
bool underFrequencyRecoverPowerDelayTime_s(uint8_t slave) { return requestAisweiRead(slave, 45438); }
bool speedOfUnderFrequencyRecoverToPn(uint8_t slave) { return requestAisweiRead(slave, 45440); }
bool underFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave) { return requestAisweiRead(slave, 45441); }

bool underVoltageIncreasePowerMode(uint8_t slave) { return requestAisweiRead(slave, 45443); }
bool underVoltageIncreasePowerStartVoltage_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45444); }
bool underVoltageIncreasePowerStopVoltage_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45445); }
bool underVoltageIncreasePowerBackVoltage_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45446); }
bool increaseRatioOfUnderVoltageIncreasePower(uint8_t slave) { return requestAisweiRead(slave, 45447); }
bool underVoltageIncreasePowerDelayTime_s(uint8_t slave) { return requestAisweiRead(slave, 45448); }
bool underVoltageIncreasePowerDelayTime2_s(uint8_t slave) { return requestAisweiRead(slave, 45449); }
bool speedOfUnderVoltageRecoverToPn(uint8_t slave) { return requestAisweiRead(slave, 45450); }

bool pav_percentPn(uint8_t slave) { return requestAisweiRead(slave, 45451); }
bool drmsPval_percentPn(uint8_t slave) { return requestAisweiRead(slave, 45452); }

bool reactivePowerControlMode(uint8_t slave) { return requestAisweiRead(slave, 45501); }
bool timeConstantOfReactivePowerCurve_s(uint8_t slave) { return requestAisweiRead(slave, 45502); }
bool pfSetValue(uint8_t slave) { return requestAisweiRead(slave, 45503); }
bool cosPPowerCurve_point1_activePercentPn(uint8_t slave) { return requestAisweiRead(slave, 45504); }
bool cosPPowerCurve_point1_cosPhi(uint8_t slave) { return requestAisweiRead(slave, 45505); }
bool cosPPowerCurve_point2_activePercentPn(uint8_t slave) { return requestAisweiRead(slave, 45506); }
bool cosPPowerCurve_point2_cosPhi(uint8_t slave) { return requestAisweiRead(slave, 45507); }
bool cosPPowerCurve_point3_activePercentPn(uint8_t slave) { return requestAisweiRead(slave, 45508); }
bool cosPPowerCurve_point3_cosPhi(uint8_t slave) { return requestAisweiRead(slave, 45509); }
bool cosPPowerCurve_point4_activePercentPn(uint8_t slave) { return requestAisweiRead(slave, 45510); }
bool cosPPowerCurve_point4_cosPhi(uint8_t slave) { return requestAisweiRead(slave, 45511); }
bool lockInVoltageForCosPPowerCurve_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45512); }
bool lockOutVoltageForCosPPowerCurve_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45513); }
bool QSetValue_percentSn(uint8_t slave) { return requestAisweiRead(slave, 45516); }
bool QU_curve_point1_U_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45518); }
bool QU_curve_point1_Q_percentSn(uint8_t slave) { return requestAisweiRead(slave, 45519); }
bool QU_curve_point2_U_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45520); }
bool QU_curve_point2_Q_percentSn(uint8_t slave) { return requestAisweiRead(slave, 45521); }
bool QU_curve_point3_U_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45522); }
bool QU_curve_point3_Q_percentSn(uint8_t slave) { return requestAisweiRead(slave, 45523); }
bool QU_curve_point4_U_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45524); }
bool QU_curve_point4_Q_percentSn(uint8_t slave) { return requestAisweiRead(slave, 45525); }
bool lockInPowerForQU_curve_percentPn(uint8_t slave) { return requestAisweiRead(slave, 45526); }
bool lockOutPowerForQU_curve_percentPn(uint8_t slave) { return requestAisweiRead(slave, 45527); }

bool LVRT_TriggerVoltage_percentUn(uint8_t slave) { return requestAisweiRead(slave, 45606); }
bool LVRT_activePowerLimitMode(uint8_t slave) { return requestAisweiRead(slave, 45609); }

/* --- write wrappers for each RW holding register --- */
bool write_remoteSwitchCommand(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 40201, value); }
bool write_rtc_Year(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41001, value); }
bool write_rtc_Month(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41002, value); }
bool write_rtc_Day(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41003, value); }
bool write_rtc_Hour(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41004, value); }
bool write_rtc_Minute(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41005, value); }
bool write_rtc_Seconds(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41006, value); }

bool write_storageInverterSwitch(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41102, value); }
bool write_typeSelectionOfEnergyStorageMachine(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41103, value); }
bool write_runMode(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41104, value); }
bool write_batteryManufacturer(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41105, value); }
bool write_smartMeterStatus(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41108, value); }
bool write_smartMeterAdjustmentFlag(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41109, value); }
bool write_setTargetPowerValue_W(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 41110, value); }
bool write_currentPowerValueOfSmartMeter_W(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 41112, value); }
bool write_antiReverseCurrentFlag(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41114, value); }
bool write_batteryWakeUp(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41115, value); }
bool write_commboxAndCloudCommunicationStatus(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41151, value); }
bool write_chargeDischargeFlagBit(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41152, value); }
bool write_chargeAndDischargePowerCommand_W(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 41153, value); }

bool write_activePowerControlFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44001, value); }
bool write_eegControlFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44002, value); }
bool write_slopeLoadFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44003, value); }
bool write_overvoltageReducePowerFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44004, value); }
bool write_overfrequencyReducePowerFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44005, value); }
bool write_reactivePowerControlFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44006, value); }
bool write_LVRTFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44007, value); }
bool write_tenMinutesAverageOvervoltageProtectFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44009, value); }
bool write_islandingProtectFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44010, value); }
bool write_peConnectionCheckFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44012, value); }
bool write_overloadFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44017, value); }
bool write_shadowMPPTFunction(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 44025, value); }

bool write_gridCode(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45201, value); }
bool write_overvoltageProtectionValue3_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45202, value); }
bool write_overvoltageProtectionValue2_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45203, value); }
bool write_overvoltageProtectionValueFreq_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45204, value); }
bool write_underfrequencyProtectionValue_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45205, value); }
bool write_gridVoltageHighLimit3_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45206, value); }
bool write_gridVoltageHighLimitTime3_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45207, value); }
bool write_gridVoltageHighLimit2_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45209, value); }
bool write_gridVoltageHighLimitTime2_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45210, value); }
bool write_gridVoltageHighLimit1_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45212, value); }
bool write_gridVoltageHighLimitTime1_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45213, value); }
bool write_gridVoltageLowLimit3_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45215, value); }
bool write_gridVoltageLowLimitTime3_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45216, value); }
bool write_gridVoltageLowLimit2_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45218, value); }
bool write_gridVoltageLowLimitTime2_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45219, value); }
bool write_gridVoltageLowLimit1_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45221, value); }
bool write_gridVoltageLowLimitTime1_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45222, value); }
bool write_tenMinutesAverageOvervoltageThreshold_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45224, value); }
bool write_tenMinutesAverageOvervoltageProtectTime_ms(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45225, value); }
bool write_overvoltageRecoverValue_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45226, value); }
bool write_undervoltageRecoverValue_V(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45227, value); }
bool write_gridFrequencyHighLimit3_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45228, value); }
bool write_gridFrequencyHighLimitTime3_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45229, value); }
bool write_gridFrequencyHighLimit2_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45231, value); }
bool write_gridFrequencyHighLimitTime2_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45232, value); }
bool write_gridFrequencyHighLimit1_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45234, value); }
bool write_gridFrequencyHighLimitTime1_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45235, value); }
bool write_gridFrequencyLowLimit3_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45237, value); }
bool write_gridFrequencyLowLimitTime3_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45238, value); }
bool write_gridFrequencyLowLimit2_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45240, value); }
bool write_gridFrequencyLowLimitTime2_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45241, value); }
bool write_gridFrequencyLowLimit1_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45243, value); }
bool write_gridFrequencyLowLimitTime1_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45244, value); }
bool write_varyRateOfFrequencyProtectValue_HzPerS(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45246, value); }
bool write_varyRateOfFrequencyProtectTime_ms(uint8_t slave, uint32_t value) { return requestAisweiWriteDWord(slave, 45247, value); }
bool write_overfrequencyRecoverValue_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45249, value); }
bool write_underfrequencyRecoverValue_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45250, value); }
bool write_timeOfFirstConnectionToGrid_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45251, value); }
bool write_timeOfReconnectionToGrid_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45252, value); }
bool write_isoProtectThreshold_kOhm(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45253, value); }
bool write_dciProtectThreshold_mA(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45254, value); }
bool write_dciProtectTime_ms(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45255, value); }

bool write_loadRateOfFirstConnectionToGrid(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45401, value); }
bool write_loadRateOfReconnectionToGrid(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45402, value); }
bool write_activePowerSet_percentPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45403, value); }
bool write_increaseRateOfActivePower_percentPnPerMin(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45404, value); }
bool write_decreaseRateOfActivePower_percentPnPerMin(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45405, value); }
bool write_overFrequencyReducePowerMode(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45408, value); }
bool write_overFrequencyReducePowerStartFrequency_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45409, value); }
bool write_overFrequencyReducePowerStopFrequency_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45410, value); }
bool write_overFrequencyReducePowerBackFrequency_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45411, value); }
bool write_reduceRatioOfOverFrequencyReducePower(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45412, value); }
bool write_overFrequencyReducePowerReduceDelayTime_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45413, value); }
bool write_overFrequencyReducePowerRecoverDelayTime_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45414, value); }
bool write_speedOfOverFrequencyRecoverToPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45416, value); }
bool write_overFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45417, value); }

bool write_overVoltageReducePowerMode(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45419, value); }
bool write_overVoltageReducePowerStartVoltage_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45420, value); }
bool write_overVoltageReducePowerStopVoltage_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45422, value); }
bool write_overVoltageReducePowerBackVoltage_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45424, value); }
bool write_reduceRatioOfOverVoltageReducePower(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45426, value); }
bool write_overVoltageReducePowerDelayTime_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45427, value); }
bool write_overVoltageRecoverPowerDelayTime_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45428, value); }
bool write_speedOfOverVoltageRecoverToPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45429, value); }

bool write_underFrequencyIncreasePowerMode(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45432, value); }
bool write_underFrequencyIncreasePowerStartFrequency_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45433, value); }
bool write_underFrequencyIncreasePowerStopFrequency_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45434, value); }
bool write_underFrequencyIncreasePowerBackFrequency_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45435, value); }
bool write_increaseRatioOfUnderFrequencyIncreasePower(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45436, value); }
bool write_underFrequencyIncreasePowerDelayTime_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45437, value); }
bool write_underFrequencyRecoverPowerDelayTime_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45438, value); }
bool write_speedOfUnderFrequencyRecoverToPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45440, value); }
bool write_underFrequencyZeroPowerFrequencyPoint_Hz(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45441, value); }

bool write_underVoltageIncreasePowerMode(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45443, value); }
bool write_underVoltageIncreasePowerStartVoltage_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45444, value); }
bool write_underVoltageIncreasePowerStopVoltage_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45445, value); }
bool write_underVoltageIncreasePowerBackVoltage_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45446, value); }
bool write_increaseRatioOfUnderVoltageIncreasePower(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45447, value); }
bool write_underVoltageIncreasePowerDelayTime_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45448, value); }
bool write_underVoltageIncreasePowerDelayTime2_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45449, value); }
bool write_speedOfUnderVoltageRecoverToPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45450, value); }

bool write_pav_percentPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45451, value); }
bool write_drmsPval_percentPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45452, value); }

bool write_reactivePowerControlMode(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45501, value); }
bool write_timeConstantOfReactivePowerCurve_s(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45502, value); }
bool write_pfSetValue(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45503, value); }
bool write_cosPPowerCurve_point1_activePercentPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45504, value); }
bool write_cosPPowerCurve_point1_cosPhi(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45505, value); }
bool write_cosPPowerCurve_point2_activePercentPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45506, value); }
bool write_cosPPowerCurve_point2_cosPhi(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45507, value); }
bool write_cosPPowerCurve_point3_activePercentPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45508, value); }
bool write_cosPPowerCurve_point3_cosPhi(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45509, value); }
bool write_cosPPowerCurve_point4_activePercentPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45510, value); }
bool write_cosPPowerCurve_point4_cosPhi(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45511, value); }
bool write_lockInVoltageForCosPPowerCurve_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45512, value); }
bool write_lockOutVoltageForCosPPowerCurve_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45513, value); }
bool write_QSetValue_percentSn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45516, value); }
bool write_QU_curve_point1_U_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45518, value); }
bool write_QU_curve_point1_Q_percentSn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45519, value); }
bool write_QU_curve_point2_U_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45520, value); }
bool write_QU_curve_point2_Q_percentSn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45521, value); }
bool write_QU_curve_point3_U_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45522, value); }
bool write_QU_curve_point3_Q_percentSn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45523, value); }
bool write_QU_curve_point4_U_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45524, value); }
bool write_QU_curve_point4_Q_percentSn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45525, value); }
bool write_lockInPowerForQU_curve_percentPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45526, value); }
bool write_lockOutPowerForQU_curve_percentPn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45527, value); }

bool write_LVRT_TriggerVoltage_percentUn(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45606, value); }
bool write_LVRT_activePowerLimitMode(uint8_t slave, uint16_t value) { return requestAisweiWriteWord(slave, 45609, value); }