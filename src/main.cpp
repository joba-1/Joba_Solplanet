#include <iostream>
#include <string>
#include <cstring>
#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <chrono>
#include <atomic>
#include <map>
#include <mutex>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <set>
#include <nlohmann/json.hpp>

#include "mqtt/client.h"

// Networking for InfluxDB line-protocol sender
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>

#include "modbus_registers.h"

// Logging helper
#define LOG(fmt, ...) printf("[%s] " fmt "\n", __FUNCTION__, ##__VA_ARGS__)

// MQTT configuration
#include "mqtt_config.h"

// INFLUX configuration
#include "influx_config.h"

// Modbus TCP configuration
#include "modbus_config.h"

using json = nlohmann::json;

static const char* mqttPrefix = MQTT_TOPIC_PREFIX;
static mqtt::client* mqttClient = nullptr;
static std::atomic<bool> running(true);
static std::string influxMeasurement;
static const char* CHANGED_ADDRESSES_FILE = ".joba_changed_addresses.json";

// Track register values and changes
struct RegisterValue {
    std::string payload;
    std::string previousPayload;  // Track previous value
    std::chrono::system_clock::time_point lastChangeTime;  // Timestamp of last change
    bool hasChanged = false;
    bool published = false;
};

static std::map<uint32_t, RegisterValue> registerValues;  // key: (unitId << 16) | addr
static std::set<uint32_t> changedAddresses;  // persistent list of changed addresses
static std::mutex registerValuesMutex;

// Helper function to format system_clock::time_point as ISO 8601 string
static std::string formatISO8601(const std::chrono::system_clock::time_point& tp) {
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(tp);
    auto tt = std::chrono::system_clock::to_time_t(sctp);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()) % 1000;
    
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&tt), "%FT%T");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count() << 'Z';
    return oss.str();
}

// Load persistent changed addresses from file
static void loadChangedAddresses() {
    std::ifstream file(CHANGED_ADDRESSES_FILE);
    if (!file.is_open()) {
        LOG("No persistent changed addresses file found");
        return;
    }

    try {
        json data = json::parse(file);
        if (data.is_array()) {
            for (const auto& addr : data) {
                if (addr.is_number_unsigned()) {
                    changedAddresses.insert(addr.get<uint32_t>());
                }
            }
            LOG("Loaded %zu persistent changed addresses", changedAddresses.size());
        }
    } catch (const std::exception& e) {
        LOG("Error loading changed addresses: %s", e.what());
    }
    file.close();
}

// Save persistent changed addresses to file
static void saveChangedAddresses() {
    try {
        json data = json::array();
        for (uint32_t addr : changedAddresses) {
            data.push_back(addr);
        }
        
        std::ofstream file(CHANGED_ADDRESSES_FILE);
        file << data.dump(2);
        file.close();
        LOG("Saved %zu changed addresses to %s", changedAddresses.size(), CHANGED_ADDRESSES_FILE);
    } catch (const std::exception& e) {
        LOG("Error saving changed addresses: %s", e.what());
    }
}

// escape tag value for Influx line protocol (commas, spaces, equals)
static std::string escapeInfluxTag(const std::string &s) {
    std::string out; out.reserve(s.size());
    for (char c: s) {
        if (c == ',' || c == ' ' || c == '=') { out.push_back('\\'); out.push_back(c); }
        else out.push_back(c);
    }
    return out;
}

// escape field string for Influx line protocol (backslash and double quote)
static std::string escapeInfluxFieldString(const std::string &s) {
    std::string out; out.reserve(s.size());
    for (char c: s) {
        if (c == '\\' || c == '"') { out.push_back('\\'); out.push_back(c); }
        else out.push_back(c);
    }
    return out;
}

// send a line to InfluxDB using HTTP POST to /write?db=<INFLUX_DB>
static bool sendInfluxLine(const char* host, int port, const std::string &line) {
    // Build HTTP request
    std::string path = std::string("/write?db=") + INFLUX_DB;
    std::string body = line;
    char headers[256];
    snprintf(headers, sizeof(headers), "POST %s HTTP/1.1\r\nHost: %s:%d\r\nContent-Type: text/plain; charset=utf-8\r\nContent-Length: %zu\r\nConnection: close\r\n\r\n",
             path.c_str(), host, port, body.size());

    struct addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    char portbuf[16]; snprintf(portbuf, sizeof(portbuf), "%d", port);
    struct addrinfo *res = nullptr;
    if (getaddrinfo(host, portbuf, &hints, &res) != 0) return false;
    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sock < 0) { freeaddrinfo(res); return false; }
    bool ok = true;
    if (connect(sock, res->ai_addr, res->ai_addrlen) != 0) ok = false;
    if (ok) {
        // send headers
        size_t hlen = strlen(headers);
        ssize_t sent = send(sock, headers, hlen, 0);
        if (sent != (ssize_t)hlen) ok = false;
        // send body
        if (ok && !body.empty()) {
            const char *buf = body.c_str();
            size_t remaining = body.size();
            while (remaining > 0) {
                ssize_t s = send(sock, buf, remaining, 0);
                if (s <= 0) { ok = false; break; }
                remaining -= s;
                buf += s;
            }
        }

        // set a short receive timeout so we don't block forever
        struct timeval tv;
        tv.tv_sec = 2; tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        // read response (headers and optional body) into a buffer
        std::string resp;
        char rbuf[1024];
        while (true) {
            ssize_t r = recv(sock, rbuf, sizeof(rbuf), 0);
            if (r > 0) resp.append(rbuf, r);
            else break;
        }

        // parse status code from response start: "HTTP/1.1 204 ..."
        if (!resp.empty()) {
            // find end of status line
            size_t nl = resp.find("\r\n");
            std::string statusLine = (nl == std::string::npos) ? resp : resp.substr(0, nl);
            // tokenise
            size_t sp1 = statusLine.find(' ');
            int status = 0;
            if (sp1 != std::string::npos) {
                size_t sp2 = statusLine.find(' ', sp1 + 1);
                std::string code = (sp2 == std::string::npos) ? statusLine.substr(sp1 + 1) : statusLine.substr(sp1 + 1, sp2 - sp1 - 1);
                status = atoi(code.c_str());
            }
            if (status == 204) {
                // success
                close(sock);
                freeaddrinfo(res);
                return true;
            } else {
                // log response for debugging
                LOG("Influx HTTP error: %d response=%s", status, resp.c_str());
                ok = false;
            }
        } else {
            // no response
            LOG("Influx: no HTTP response received");
            ok = false;
        }
    }
    close(sock);
    freeaddrinfo(res);
    return ok;
}

// Publish to MQTT and Influx only on change or first publish.
static void publishToMqttAndInfluxOnChange(const char* topic, const char* payload, size_t payload_len, uint8_t unitId, uint16_t addr) {
    uint32_t key = ((uint32_t)unitId << 16) | addr;
    std::string payloadStr(payload, payload_len);
    auto now = std::chrono::system_clock::now();
    
    {
        std::lock_guard<std::mutex> lock(registerValuesMutex);
        auto it = registerValues.find(key);
        
        // Check if value has changed
        bool changed = false;
        if (it == registerValues.end()) {
            // First time seeing this register - just store it, don't mark as changed yet
            registerValues[key] = {payloadStr, "", now, false, false};
            // Check if this address was previously marked as changed
            if (changedAddresses.find(key) != changedAddresses.end()) {
                // This address changed in a previous run, mark as changed
                registerValues[key].hasChanged = true;
                registerValues[key].lastChangeTime = now;
            }
        } else if (it->second.payload != payloadStr) {
            // Value changed after first reading
            changed = true;
            it->second.previousPayload = it->second.payload;
            it->second.payload = payloadStr;
            it->second.hasChanged = true;
            it->second.lastChangeTime = now;
            // Add to persistent list
            size_t size = changedAddresses.size();
            changedAddresses.insert(key);
            if (changedAddresses.size() != size) {
                // New entry added, save to file
                saveChangedAddresses();
            }
        }
        
        // Only publish if changed
        if (changed) {
            // MQTT publish
            try {
                if (mqttClient && mqttClient->is_connected()) {
                    mqttClient->publish(topic, payload, payload_len);
                }
            } catch (const mqtt::exception &e) {
                LOG("MQTT publish failed: %s", e.what());
            }

            // Build Influx line
            std::string topicStr(topic);
            size_t p = topicStr.find_last_of('/');
            std::string lastTopic = (p == std::string::npos) ? topicStr : topicStr.substr(p+1);
            std::string measurement = influxMeasurement.empty() ? std::string(MQTT_TOPIC_PREFIX) : influxMeasurement;

            // Determine if payload is numeric
            char *endptr = nullptr;
            double num = strtod(payloadStr.c_str(), &endptr);
            bool isNum = (endptr && *endptr == '\0');

            std::string line;
            line.reserve(128 + payload_len);
            line += measurement;
            line += ",unit=" + std::to_string(unitId);
            line += ",addr=" + std::to_string(addr);
            line += ",name=" + escapeInfluxTag(lastTopic);
            line += ' ';
            if (isNum) {
                char numbuf[64]; snprintf(numbuf, sizeof(numbuf), "%.6g", num);
                line += std::string("value=") + numbuf;
            } else {
                line += "text=\"" + escapeInfluxFieldString(payloadStr) + "\"";
            }
            line += '\n';

            try {
                if (!sendInfluxLine(INFLUX_SERVER, INFLUX_PORT, line)) {
                    LOG("Influx publish failed for %s", topic);
                }
            } catch (...) {
                LOG("Influx publish exception for %s", topic);
            }

            LOG("Published change: %s -> %s", topic, payload);
        }
    }
}

// Publish summary every minute to MQTT (JSON) and Influx
static void publishSummary() {
    std::lock_guard<std::mutex> lock(registerValuesMutex);
    
    if (registerValues.empty()) {
        LOG("No values to summarize");
        return;
    }

    // Build JSON summary for MQTT with hierarchy: unit -> address -> name/value/timestamp
    json summary = json::object();
    std::map<uint8_t, json> influxFields;  // unitId -> field map

    for (const auto& [key, value] : registerValues) {
        // Only include registers that have changed (from current session or persistent list)
        if (!value.hasChanged) continue;

        uint8_t unitId = (key >> 16) & 0xFF;
        uint16_t addr = key & 0xFFFF;

        // Ensure unit exists in summary
        if (summary.find(std::to_string(unitId)) == summary.end()) {
            summary[std::to_string(unitId)] = json::object();
        }

        // Ensure address exists under unit
        std::string addrStr = std::to_string(addr);
        if (summary[std::to_string(unitId)].find(addrStr) == summary[std::to_string(unitId)].end()) {
            summary[std::to_string(unitId)][addrStr] = json::object();
        }

        // Find register name for this address
        std::string registerName = addrStr;  // fallback to address
        for (size_t i = 0; i < aiswei_registers_count; ++i) {
            if (aiswei_registers[i].addr == addr) {
                if (aiswei_registers[i].name && aiswei_registers[i].name[0]) {
                    registerName = aiswei_registers[i].name;
                }
                break;
            }
        }

        // Add register data: name, value, and ISO 8601 timestamp
        summary[std::to_string(unitId)][addrStr]["name"] = registerName;
        summary[std::to_string(unitId)][addrStr]["value"] = value.payload;
        summary[std::to_string(unitId)][addrStr]["changed"] = formatISO8601(value.lastChangeTime);

        // Determine if numeric for Influx
        char *endptr = nullptr;
        double num = strtod(value.payload.c_str(), &endptr);
        bool isNum = (endptr && *endptr == '\0');

        // Build field name: addr_<address> or addr_<address>_<payload_slug>
        std::string fieldName = "addr_" + std::to_string(addr);
        if (!isNum) {
            // For non-numeric, append slug from payload
            std::string slug = value.payload;
            // Remove/replace non-alphanumeric
            for (auto& c : slug) {
                if (!isalnum(c)) c = '_';
            }
            fieldName += "_" + slug;
        }

        // Add to Influx field map
        if (influxFields.find(unitId) == influxFields.end()) {
            influxFields[unitId] = json::object();
        }
        
        if (isNum) {
            influxFields[unitId][fieldName] = num;
        } else {
            influxFields[unitId][fieldName] = value.payload;
        }
    }

    // Only publish if there are changed values
    if (summary.empty()) {
        LOG("No changed values to summarize");
        return;
    }

    // Publish MQTT summary
    std::string summaryJson = summary.dump();
    std::string summaryTopic = std::string(mqttPrefix) + "/summary";
    try {
        if (mqttClient && mqttClient->is_connected()) {
            mqttClient->publish(summaryTopic, summaryJson.c_str(), summaryJson.size());
            LOG("Published MQTT summary to %s with %zu units", summaryTopic.c_str(), summary.size());
        }
    } catch (const mqtt::exception &e) {
        LOG("MQTT summary publish failed: %s", e.what());
    }

    // Publish Influx summary (one data point per unitId)
    for (const auto& [unitId, fields] : influxFields) {
        std::string line = "summary,unit=" + std::to_string(unitId);
        
        bool first = true;
        for (const auto& [fieldName, fieldValue] : fields.items()) {
            if (first) {
                line += " ";
            } 
            else {
                line += ",";
            }
            line += escapeInfluxTag(fieldName) + "=";
            
            if (fieldValue.is_number()) {
                char numbuf[64];
                snprintf(numbuf, sizeof(numbuf), "%.6g", (double)fieldValue);
                line += numbuf;
            } else {
                line += "\"" + escapeInfluxFieldString(fieldValue.get<std::string>()) + "\"";
            }
            first = false;
        }
        line += '\n';

        try {
            if (!sendInfluxLine(INFLUX_SERVER, INFLUX_PORT, line)) {
                LOG("Influx summary publish failed for unit %d", unitId);
            } else {
                LOG("Published Influx summary for unit %d with %zu fields", unitId, fields.size());
            }
        } catch (...) {
            LOG("Influx summary publish exception for unit %d", unitId);
        }
    }
}

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

// helper: decode a single Modbus response and publish a human friendly payload to MQTT
void decodeAndPublish(uint8_t unitId, uint16_t addr, uint8_t* data, size_t length) {
    // find matching register definition by comparing register offsets
    const RegisterInfo* ri = nullptr;
    for (size_t i = 0; i < aiswei_registers_count; ++i) {
        const RegisterInfo &r = aiswei_registers[i];
        if (addr >= r.addr && addr < r.addr + r.length) {
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
        snprintf(topic, sizeof(topic), "%s/%u/%s", mqttPrefix, unitId, slug);
    } else {
        // fallback to numeric register offset (legacy)
        snprintf(topic, sizeof(topic), "%s/%u/%u", mqttPrefix, unitId, addr);
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
            publishToMqttAndInfluxOnChange(topic, payload, payload_len, unitId, addr);
            // LOG("0x%02x no info on %u: %s (float)", unitId, addr, payload);
            return;
        }

        // publish hex payload if unknown
        size_t pos = 0;
        for (size_t i = 0; i < length && pos + 3 < sizeof(payload); ++i) {
            pos += snprintf(payload + pos, sizeof(payload) - pos, "%02x", data[i]);
        }
        payload[pos] = '\0';
        publishToMqttAndInfluxOnChange(topic, payload, pos, unitId, addr);
        // LOG("0x%02x no info on %u: %s (hex)", unitId, addr, payload);
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
        publishToMqttAndInfluxOnChange(topic, payload, pos, unitId, addr);
        // LOG("0x%02x %s -> %s (%s)", unitId, ri->name, payload, type);
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
        publishToMqttAndInfluxOnChange(topic, payload, strlen(payload), unitId, addr);
        // LOG("0x%02x %s -> %s (%s)", unitId, ri->name, payload, type);
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
        publishToMqttAndInfluxOnChange(topic, payload, strlen(payload), unitId, addr);
        // LOG("0x%02x %s -> %s (%s)", unitId, ri->name, payload, type);
        return;
    }

    // fallback: publish hex
    {
        size_t pos = 0;
        for (size_t i = 0; i < length && pos + 3 < sizeof(payload); ++i) {
            pos += snprintf(payload + pos, sizeof(payload) - pos, "%02x", data[i]);
        }
        payload[pos] = '\0';
        publishToMqttAndInfluxOnChange(topic, payload, pos, unitId, addr);
        // LOG("0x%02x %s -> %s (hex)", unitId, ri->name, payload);
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
            std::string("tcp://") + MQTT_SERVER + ":" + std::to_string(MQTT_PORT), MQTT_TOPIC_PREFIX
        );
        
        LOG("Connecting to MQTT %s:%d", MQTT_SERVER, MQTT_PORT);
        mqttClient->connect(opts);
        LOG("MQTT connected for topics %s/#", MQTT_TOPIC_PREFIX);
        
    } catch (const mqtt::exception& exc) {
        LOG("MQTT connection failed: %s", exc.what());
    }
}

// Modbus polling thread
static void modbusThread() {
    unsigned pollCount = 0;
    unsigned index = 0;
    unsigned prev_index = index;
    /// uint8_t id = 0;
    bool requested = false;
    
    while (running) {
        if (++pollCount % 4 == 0) {  // Every 0.2 seconds
            // Build a contiguous batch starting at `index` covering up to MODBUS_BATCH_SIZE 16-bit registers
            unsigned startIdx = index;
            uint16_t startAddrDec = aiswei_registers[startIdx].addr;
            uint16_t startReg = aiswei_dec2reg(startAddrDec);
            uint16_t totalRegs = aiswei_registers[startIdx].length;
            unsigned k = 1;
            uint16_t prevReg = startReg + totalRegs;
            bool startIsHolding = (startAddrDec >= 40000 && startAddrDec < 50000);

            while (totalRegs < MODBUS_BATCH_SIZE && k < aiswei_registers_count) {
                unsigned idx = (startIdx + k) % aiswei_registers_count;
                uint16_t addr_dec = aiswei_registers[idx].addr;
                uint16_t reg = aiswei_dec2reg(addr_dec);
                uint16_t len = aiswei_registers[idx].length;
                bool isHolding = (addr_dec >= 40000 && addr_dec < 50000);
                // stop if non-contiguous or different register type
                if (reg != prevReg) break;
                if (isHolding != startIsHolding) break;
                if (totalRegs + len > MODBUS_BATCH_SIZE) break;
                totalRegs += len;
                prevReg = reg + len;
                ++k;
            }

            // request the batch (totalRegs = number of 16-bit registers)
            // startAddrDec is decimal AISWEI address (e.g. 31301)
            // requestAisweiReadRange will set internal transaction address
            // so the response parser can dispatch each register.
            // LOG("Sending Modbus batch: idx=%u addr=%u regs=%u entries=%u", startIdx, startAddrDec, totalRegs, k);
            requested = requestAisweiReadRange(MODBUS_UNIT_ID, startAddrDec, totalRegs);
            // advance index by number of entries consumed
            prev_index = index;
            index = (startIdx + k) % aiswei_registers_count;

            // TODO check currentPowerValueOfSmartMeter_W(MODBUS_UNIT_ID);
            // serialNumber(MODBUS_UNIT_ID);
            // manufacturerName(MODBUS_UNIT_ID);
            // brandName(MODBUS_UNIT_ID);
            // requested = batterySOC(id++);
            // TODO check batteryVoltage_V(MODBUS_UNIT_ID);
            // TODO check Dword batteryEChargeToday_kWh(MODBUS_UNIT_ID);
            // numbers are always zero (matching byte results), strings work
        }
        
        // Try to parse response
        if (requested && parseModbusTCPResponse()) {
            requested = false;
        }

        // Check if we finished a sweep for summary publication
        if (index < prev_index && !requested) {
            // completed a full sweep
            prev_index = index;
            publishSummary();
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

int main(int argc, char** argv) {
    std::cout << "Starting Joba Solplanet Gateway..." << std::endl;

    // Load persistent changed addresses
    loadChangedAddresses();

    // derive Influx measurement from the last part of MQTT topic prefix
    {
        std::string prefix(MQTT_TOPIC_PREFIX);
        size_t pos = prefix.find_last_of('/');
        if (pos == std::string::npos) influxMeasurement = prefix; else influxMeasurement = prefix.substr(pos+1);
    }

    // Start MQTT thread
    std::thread mqtt_th(mqttThread);
    mqtt_th.detach();

    // Initialize aiswei_registers array for testing (iterating through all entries): 
    // each word as unsigned 16-bit at addresses 30000..49999
    for (unsigned i=0; i < aiswei_registers_count; i++) {
        aiswei_registers[i].addr = 30000 + i;
        aiswei_registers[i].length = 1;
        std::string *name = new std::string("Register " + std::to_string(30000 + i));
        aiswei_registers[i].name = name->c_str();
        aiswei_registers[i].type = "B16";
        aiswei_registers[i].unit = NULL;
        aiswei_registers[i].gain = 1.0f;
        aiswei_registers[i].access = "RO";
    }

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
    
    cleanupModbusTCP();

    if (mqttClient) {
        try {
            mqttClient->disconnect();
            delete mqttClient;
        } catch (...) {}
    }

    std::cout << "Shutdown complete." << std::endl;
    return 0;
}