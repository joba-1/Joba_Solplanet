#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <thread>
#include <chrono>
#include <atomic>
#include "mqtt/client.h"

#include "modbus_registers.h"

// Logging helper
#define LOG(fmt, ...) printf("[%s] " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

// MQTT configuration
#ifndef MQTT_SERVER
#define MQTT_SERVER "job4"
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

// Modbus TCP configuration
#ifndef MODBUS_TCP_HOST
#define MODBUS_TCP_HOST "192.168.1.191"
#endif
#ifndef MODBUS_TCP_PORT
#define MODBUS_TCP_PORT 502
#endif

// Global state
int modbusSocket = -1;
uint16_t transactionId = 0;
static const char* mqttPrefix = "joba_solplanet";
static mqtt::client* mqttClient = nullptr;
static std::atomic<bool> running(true);

// translate AISWEI warning codes to descriptive strings
const char* warnCodeToString(uint16_t code) {
    switch (code) {
        case 0:   return "No warning";
        case 30:  return "Recover from warning";
        case 150: return "SPD Damaged";
        case 156: return "Internal fan warning";
        case 157: return "External fan warning";
        case 163: return "String current abnormal";
        case 165: return "Ground connect warning";
        case 166: return "CPU self-test: Register abnormal";
        case 167: return "CPU self-test: RAM abnormal";
        case 168: return "CPU self-test: ROM abnormal";
        case 174: return "Low air temperature";
        case 175: return "Battery SOC low";
        case 176: return "Battery fault status";
        case 177: return "Battery communication disconnect";
        case 178: return "EPS output over";
        case 179: return "Combox and cloud disconnect";
        case 180: return "PV string inverse";
        default:  return "Unknown warning code";
    }
}

// translate AISWEI error codes to descriptive strings
const char* errorCodeToString(uint16_t code) {
    switch (code) {
        case 1:  return "Communication fails between M-S";
        case 3:  return "Relay check fail";
        case 4:  return "DC injection high";
        case 5:  return "Auto test function result fail";
        case 6:  return "DC bus too high";
        case 8:  return "AC HCT failure";
        case 9:  return "GFCI device failure";
        case 10: return "Device fault";
        case 32: return "ROCOF fault";
        case 33: return "Fac failure: Fac out of range";
        case 34: return "AC voltage out of range";
        case 35: return "Utility loss";
        case 36: return "GFCI failure";
        case 37: return "PV over voltage";
        case 38: return "Isolation fault";
        case 40: return "Over temperature in inverter";
        case 41: return "Consistent fault: Vac differs for M-S";
        case 42: return "Consistent fault: Fac differs for M-S";
        case 43: return "Consistent fault: Ground I differs for M-S";
        case 44: return "Consistent fault: DC inj. differs for M-S";
        case 45: return "Consistent fault: Fac, Vac differs for M-S";
        case 46: return "High DC bus";
        case 47: return "Consistent fault";
        case 48: return "Average volt of ten minutes fault";
        case 56: return "GFCI protect fault: 30mA level";
        case 57: return "GFCI protect fault: 60mA level";
        case 58: return "GFCI protect fault: 150mA level";
        case 61: return "DRMS communication fails (S9 open)";
        case 62: return "DRMS order disconnection device (S0 close)";
        case 65: return "PE connection fault";
        default: return "Unknown error code";
    }
}

// translate AISWEI grid codes to descriptive strings
const char* gridCodeToString(uint16_t code) {
    switch (code) {
        case 35: return "NB/T32004:2018";
        case 47: return "AU AS 4777.2 : 2015";
        case 48: return "NZ AS 4777.2 : 2015";
        case 49: return "ENGG-50Hz";
        case 50: return "ENGG-60Hz";
        case 59: return "CNS15382:2018";
        case 64: return "EN 50549-1";
        case 65: return "NL EN50549-1:2019";
        case 66: return "BR NBR 16149:2013";
        case 67: return "VDE0126-1-1/A1/VFR";
        case 68: return "IEC 61727 50Hz";
        case 69: return "C10/11:2019";
        case 70: return "VDE-AR-N4105:2018";
        case 71: return "IEC 61727 60Hz";
        case 72: return "G98/1";
        case 73: return "G99/1";
        case 74: return "AU AS/NZS4777.2:2020 A";
        case 75: return "AU AS/NZS4777.2:2020 B";
        case 76: return "AU AS/NZS4777.2:2020 C";
        case 77: return "NZ AS/NZS4777.2:2020";
        case 78: return "IL SI4777.3";
        case 79: return "KR KS C 8565:2020";
        case 80: return "ES UNE206007-1";
        case 81: return "CY EN50549-1";
        case 82: return "CS PPDS A1";
        case 83: return "PL EN50549-1";
        case 84: return "CEI 0-21:2019";
        case 85: return "DK EN50549-1";
        case 86: return "CH NA/EEA-NE7";
        case 87: return "SE EIFS:2018";
        case 88: return "FI EN50549-1";
        case 89: return "RO Order208";
        case 90: return "SI EN50549-1";
        case 91: return "LV EN50549-1";
        case 92: return "VDE0126/VFR2019 IS (50Hz)";
        case 93: return "VDE0126/VFR2019 IS (60Hz)";
        default: return "Unknown grid code";
    }
}

// Forward declaration
static void decodeAndPublish(uint8_t serverAddress, uint8_t fc, uint16_t address, uint8_t* data, size_t length);

// Modbus TCP connection management
static bool connectModbusTCP() {
    if (modbusSocket != -1) {
        return true;  // already connected
    }

    struct hostent* host = gethostbyname(MODBUS_TCP_HOST);
    if (!host) {
        LOG("Failed to resolve hostname: %s", MODBUS_TCP_HOST);
        return false;
    }

    modbusSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (modbusSocket < 0) {
        LOG("Failed to create socket");
        return false;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(MODBUS_TCP_PORT);
    server_addr.sin_addr.s_addr = ((struct in_addr*)host->h_addr)->s_addr;

    if (connect(modbusSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        LOG("Failed to connect to Modbus TCP server %s:%d", MODBUS_TCP_HOST, MODBUS_TCP_PORT);
        close(modbusSocket);
        modbusSocket = -1;
        return false;
    }

    LOG("Connected to Modbus TCP server %s:%d", MODBUS_TCP_HOST, MODBUS_TCP_PORT);
    return true;
}

// Modbus TCP request builder and sender
bool sendModbusTCPRequest(uint8_t unitId, uint8_t functionCode, uint16_t startAddress, uint16_t quantity) {
    if (!connectModbusTCP()) {
        return false;
    }

    // Build Modbus TCP frame
    uint8_t frame[12];
    uint16_t tid = transactionId++;
    
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

// Modbus TCP response parser
static void parseModbusTCPResponse() {
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

    // Check for exception response (bit 7 set)
    if (fc & 0x80) {
        uint8_t exceptionCode = buffer[8];
        LOG("Modbus exception: fc=0x%02x, exception=0x%02x", fc, exceptionCode);
        return;
    }

    // Parse PDU for function code 0x03 (Read Holding Registers)
    if (fc == 0x03) {
        uint8_t byteCount = buffer[8];
        if (bytesRead < 9 + byteCount) {
            LOG("Response data incomplete");
            return;
        }

        uint8_t* registerData = &buffer[9];
        uint16_t startAddress = 0;
        
        printf("[parseModbusTCPResponse] id 0x%02x fc 0x%02x len %u: 0x", unitId, fc, byteCount);
        for (int i = 0; i < byteCount; ++i) {
            printf("%02x", registerData[i]);
        }
        printf("\n");

        // Decode and publish
        decodeAndPublish(unitId, fc, startAddress, registerData, byteCount);
    }
}

// helper: decode a single Modbus response and publish a human friendly payload to MQTT
static void decodeAndPublish(uint8_t serverAddress, uint8_t fc, uint16_t address, uint8_t* data, size_t length) {
    // find matching register definition by comparing register offsets
    const RegisterInfo* ri = nullptr;
    for (size_t i = 0; i < aiswei_registers_count; ++i) {
        const RegisterInfo &r = aiswei_registers[i];
        uint16_t regOff = aiswei_dec2reg(r.addr);
        if (address >= regOff && address < regOff + r.length) {
            ri = &r;
            break;
        }
    }

    // build topic using human readable slug derived from register name when available
    char topic[128];
    if (ri && ri->name && ri->name[0]) {
        const char* name = ri->name;
        char slug[64];
        size_t si = 0;
        bool lastUnderscore = false;
        for (size_t i = 0; name[i] && si + 1 < sizeof(slug); ++i) {
            unsigned char c = (unsigned char)name[i];
            if (isalnum(c)) {
                slug[si++] = (char)tolower(c);
                lastUnderscore = false;
            } else {
                if (!lastUnderscore) {
                    slug[si++] = '_';
                    lastUnderscore = true;
                }
            }
        }
        // trim trailing underscore
        while (si > 0 && slug[si-1] == '_') --si;
        if (si == 0) {
            // fallback to numeric address if slug empty
            snprintf(slug, sizeof(slug), "%u", ri->addr);
        } else {
            slug[si] = '\0';
        }
        snprintf(topic, sizeof(topic), "%s/modbus/%u/%s", mqttPrefix, serverAddress, slug);
    } else {
        // fallback to numeric register offset (legacy)
        snprintf(topic, sizeof(topic), "%s/modbus/%u/%u", mqttPrefix, serverAddress, address);
    }

    char payload[128] = {0};

    if (!ri) {
        // no metadata found -> fallback to previous behavior (float if 4 bytes, else hex)
        if (length == 4) {
            uint8_t tmp[4];
            for (int i = 0; i < 4; ++i) tmp[i] = data[3 - i];
            float value;
            memcpy(&value, tmp, sizeof(value));
            size_t payload_len = snprintf(payload, sizeof(payload), "%.3f", value);
            if (mqttClient && mqttClient->is_connected()) {
                mqttClient->publish(topic, payload, payload_len);
            }
            LOG("decoded float: %s", payload);
            return;
        }

        // publish hex payload if unknown
        size_t pos = 0;
        for (size_t i = 0; i < length && pos + 3 < sizeof(payload); ++i) {
            pos += snprintf(payload + pos, sizeof(payload) - pos, "%02x", data[i]);
        }
        payload[pos] = '\0';
        if (mqttClient && mqttClient->is_connected()) {
            mqttClient->publish(topic, payload, pos);
        }
        LOG("no register meta -> hex: %s", payload);
        return;
    }

    // At this point we have a register info 'ri'. Decode according to ri->type.
    const char* type = ri->type ? ri->type : "";
    // helper to format numeric with gain
    auto fmt_with_gain = [&](double raw) {
        double val = raw * ri->gain;
        // choose precision heuristically
        int prec = 0;
        if (ri->gain < 1.0) {
            // simple mapping for common gains
            if (ri->gain == 0.1) prec = 1;
            else if (ri->gain == 0.01) prec = 2;
            else if (ri->gain == 0.001) prec = 3;
            else prec = 3;
        }
        if (prec == 0) snprintf(payload, sizeof(payload), "%.0f", val);
        else {
            char fmt[8];
            snprintf(fmt, sizeof(fmt), "%%.%df", prec);
            snprintf(payload, sizeof(payload), fmt, val);
        }
    };

    if (strcmp(type, "String") == 0) {
        // interpret register bytes as ASCII characters (high byte then low byte per register)
        size_t pos = 0;
        for (size_t i = 0; i + 1 < length && pos + 1 < sizeof(payload); i += 2) {
            char hi = (char)data[i];
            char lo = (char)data[i + 1];
            if (hi >= 32 && hi <= 126) payload[pos++] = hi;
            if (lo >= 32 && lo <= 126 && pos + 1 < sizeof(payload)) payload[pos++] = lo;
        }
        payload[pos] = '\0';
        if (payload[0] == '\0') strncpy(payload, "<empty>", sizeof(payload));
        if (mqttClient && mqttClient->is_connected()) {
            mqttClient->publish(topic, payload, pos);
        }
        LOG("%s -> %s", ri->name, payload);
        return;
    }

    if (strcmp(type, "U16") == 0 || strcmp(type, "E16") == 0 || strcmp(type, "B16") == 0 || strcmp(type, "S16") == 0) {
        if (length < 2) {
            snprintf(payload, sizeof(payload), "<too short>");
        } else {
            uint16_t raw = (uint16_t)data[0] << 8 | data[1];
            if (strcmp(type, "U16") == 0) {
                fmt_with_gain(raw);
            } else if (strcmp(type, "S16") == 0) {
                int16_t s = (int16_t)raw;
                fmt_with_gain(s);
            } else if (strcmp(type, "E16") == 0) {
                // choose appropriate enum translation by name hint
                if (strstr(ri->name, "Warning") || strstr(ri->name, "warning")) {
                    const char* s = warnCodeToString(raw);
                    snprintf(payload, sizeof(payload), "%u (%s)", raw, s);
                } else if (strstr(ri->name, "Error") || strstr(ri->name, "error")) {
                    const char* s = errorCodeToString(raw);
                    snprintf(payload, sizeof(payload), "%u (%s)", raw, s);
                } else if (strstr(ri->name, "Grid") || strstr(ri->name, "grid")) {
                    const char* s = gridCodeToString(raw);
                    snprintf(payload, sizeof(payload), "%u (%s)", raw, s);
                } else {
                    snprintf(payload, sizeof(payload), "%u", raw);
                }
            } else { // B16
                snprintf(payload, sizeof(payload), "0x%04x", raw);
            }
        }
        if (mqttClient && mqttClient->is_connected()) {
            mqttClient->publish(topic, payload, strlen(payload));
        }
        LOG("%s -> %s", ri->name, payload);
        return;
    }

    if (strcmp(type, "U32") == 0 || strcmp(type, "S32") == 0) {
        if (length < 4) {
            snprintf(payload, sizeof(payload), "<too short>");
        } else {
            uint32_t raw = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
            if (strcmp(type, "S32") == 0) {
                int32_t s = (int32_t)raw;
                fmt_with_gain(s);
            } else {
                fmt_with_gain(raw);
            }
        }
        if (mqttClient && mqttClient->is_connected()) {
            mqttClient->publish(topic, payload, strlen(payload));
        }
        LOG("%s -> %s", ri->name, payload);
        return;
    }

    // fallback: publish hex
    {
        size_t pos = 0;
        for (size_t i = 0; i < length && pos + 3 < sizeof(payload); ++i) {
            pos += snprintf(payload + pos, sizeof(payload) - pos, "%02x", data[i]);
        }
        payload[pos] = '\0';
        if (mqttClient && mqttClient->is_connected()) {
            mqttClient->publish(topic, payload, pos);
        }
        LOG("%s -> %s (hex)", ri->name, payload);
    }
}

// MQTT polling thread
static void mqttThread() {
    try {
        auto opts = mqtt::connect_options_builder()
                        .keep_alive_interval(std::chrono::seconds(60))
                        .clean_session(false)
                        .automatic_reconnect(true)
                        .finalize();
        
        mqttClient = new mqtt::client(
            std::string("tcp://") + MQTT_SERVER + ":" + std::to_string(MQTT_PORT),
            "joba_solplanet_linux"
        );
        
        LOG("Connecting to MQTT %s:%d", MQTT_SERVER, MQTT_PORT);
        mqttClient->connect(opts);
        LOG("MQTT connected");
        
    } catch (const mqtt::exception& exc) {
        LOG("MQTT connection failed: %s", exc.what());
    }
}

// Modbus polling thread
static void modbusThread() {
    int pollCount = 0;
    while (running) {
        if (++pollCount % 10 == 0) {  // Every 1 seconds
            LOG("Sending Modbus TCP request...");
            // sendModbusTCPRequest(0x01, 0x03, 52, 2);  // Read 2 registers starting at address 52
            // currentPowerValueOfSmartMeter_W(1);
            sendModbusTCPRequest(1, 0x03, 31001, 100);
        }
        
        // Try to parse response
        parseModbusTCPResponse();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int main(int argc, char** argv) {
    std::cout << "Starting Joba Solplanet (Linux)..." << std::endl;

    // Start MQTT thread
    std::thread mqtt_th(mqttThread);
    mqtt_th.detach();

    // Start Modbus polling thread
    std::thread modbus_th(modbusThread);

    // Main thread: handle signals/commands
    std::string command;
    std::cout << "Type 'quit' to exit..." << std::endl;
    
    while (running && std::getline(std::cin, command)) {
        if (command == "quit" || command == "exit") {
            running = false;
            break;
        }
    }

    // Cleanup
    running = false;
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    if (modbusSocket != -1) {
        close(modbusSocket);
    }
    
    if (mqttClient) {
        try {
            mqttClient->disconnect();
            delete mqttClient;
        } catch (...) {}
    }

    std::cout << "Shutdown complete." << std::endl;
    return 0;
}