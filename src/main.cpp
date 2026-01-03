#include <Arduino.h>

#include <WiFiManager.h>

// Web Updater
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>

#define WEBSERVER_PORT 80
AsyncWebServer webServer(WEBSERVER_PORT);
uint32_t shouldReboot = 0;   // after updates or timeouts...
uint32_t delayReboot = 100;  // reboot with a slight delay

// MQTT
#include <PubSubClient.h>

#ifndef MQTT_SERVER
#define MQTT_SERVER "job4"
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif
#ifndef MQTT_TOPIC
#define MQTT_TOPIC "joba_solplanet"
#endif

static WiFiClient wifiMqtt;
static PubSubClient mqttClient(wifiMqtt);
static const char* mqttPrefix = MQTT_TOPIC;

// Time sync
#include <time.h>

#ifndef NTP_SERVER
#define NTP_SERVER "ax3"
#endif

char start_time[30];

// Syslog
#include <Syslog.h>

#ifndef SYSLOG_SERVER
#define SYSLOG_SERVER "job4"
#endif
#ifndef SYSLOG_PORT
#define SYSLOG_PORT 514
#endif

WiFiUDP logUDP;
Syslog syslog(logUDP, SYSLOG_PROTO_IETF);
char msg[512];  // one buffer for all syslog and web messages

void slog(const char *message, uint16_t pri = LOG_INFO) {
    static bool log_infos = true;
    
    if (pri < LOG_INFO || log_infos) {
        Serial.println(message);
        syslog.log(pri, message);
    }

    if (log_infos && millis() > 10 * 60 * 1000) {
        log_infos = false;  // log infos only for first 10 minutes
        slog("Switch off info level messages", LOG_NOTICE);
    }
}

// Modbus RTU
// #include "modbus_sniffer.h"
#include "ModbusRtuDecoder.h"

#include <esp32ModbusRTU.h>
#include "modbus_registers.h"

// esp32ModbusRTU modbus(&Serial1, -1);  // use Serial1 and no pin as RTS

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
        case 0:  return "No error";
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

static void ensureMqttConnected() {
    if (mqttClient.connected()) return;
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    snprintf(msg, sizeof(msg), "Connecting to MQTT %s:%d ...\n", MQTT_SERVER, MQTT_PORT);
    slog(msg);
    msg[0] = '\0';  // clear message after displaying it
    // client id: joba_solplanet + last 4 of chip id
    uint32_t chipid = (uint32_t)ESP.getEfuseMac();
    char clientId[40];
    snprintf(clientId, sizeof(clientId), "%s-%08X", mqttPrefix, (unsigned int)(chipid & 0xFFFFFFFF));
    if (mqttClient.connect(clientId)) {
        slog("MQTT connected");
    } else {
        snprintf(msg, sizeof(msg), "MQTT connect failed, rc=%d\n", mqttClient.state());
        slog(msg, LOG_ERR);
        msg[0] = '\0';  // clear message after displaying it
    }
}

// helper: decode a single Modbus response and publish a human friendly payload to MQTT
static void decodeAndPublish(uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint16_t address, uint8_t* data, size_t length) {
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
            snprintf(slug, sizeof(slug), "%lu", ri->addr);
        } else {
            slug[si] = '\0';
        }
        snprintf(topic, sizeof(topic), "%s/%u/%s", mqttPrefix, serverAddress, slug);
    } else {
        // fallback to numeric register offset (legacy)
        snprintf(topic, sizeof(topic), "%s/%u/%u", mqttPrefix, serverAddress, address);
    }

    char payload[128] = {0};

    if (!ri) {
        // no metadata found -> fallback to previous behavior (float if 4 bytes, else hex)
        if (length == 4) {
            uint8_t tmp[4];
            for (int i = 0; i < 4; ++i) tmp[i] = data[3 - i];
            float value;
            memcpy(&value, tmp, sizeof(value));
            snprintf(payload, sizeof(payload), "%.3f", value);
            if (mqttClient.connected()) mqttClient.publish(topic, payload);
            snprintf(msg, sizeof(msg), "decoded float: %s\n\n", payload);
            slog(msg);
            msg[0] = '\0';  // clear message after displaying it
            return;
        }

        // publish hex payload if unknown
        size_t pos = 0;
        for (size_t i = 0; i < length && pos + 3 < sizeof(payload); ++i) {
            pos += snprintf(payload + pos, sizeof(payload) - pos, "%02x", data[i]);
        }
        payload[pos] = '\0';
        if (mqttClient.connected()) mqttClient.publish(topic, payload);
        snprintf(msg, sizeof(msg), "no register meta -> hex: %s\n", payload);
        slog(msg);
        msg[0] = '\0';  // clear message after displaying it
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
        if (mqttClient.connected()) mqttClient.publish(topic, payload);
        snprintf(msg, sizeof(msg), "%s -> %s\n", ri->name, payload);
        slog(msg);
        msg[0] = '\0';  // clear message after displaying it
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
        if (mqttClient.connected()) mqttClient.publish(topic, payload);
        snprintf(msg, sizeof(msg), "%s -> %s\n", ri->name, payload);
        slog(msg);
        msg[0] = '\0';  // clear message after displaying it
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
        if (mqttClient.connected()) mqttClient.publish(topic, payload);
        snprintf(msg, sizeof(msg), "%s -> %s\n", ri->name, payload);
        slog(msg);
        msg[0] = '\0';  // clear message after displaying it
        return;
    }

    // fallback: publish hex
    {
        size_t pos = 0;
        for (size_t i = 0; i < length && pos + 3 < sizeof(payload); ++i) {
            pos += snprintf(payload + pos, sizeof(payload) - pos, "%02x", data[i]);
        }
        payload[pos] = '\0';
        if (mqttClient.connected()) mqttClient.publish(topic, payload);
        snprintf(msg, sizeof(msg), "%s -> %s (hex)\n", ri->name, payload);
        slog(msg);
        msg[0] = '\0';  // clear message after displaying it
    }
}

// reconnect wlan if needed, return true if connected
bool handle_wifi() {
    static const uint32_t reconnectInterval = 10000;  // try reconnect every 10s
    static const uint32_t reconnectLimit = 60;        // try restart after 10min
    static uint32_t reconnectPrev = 0;
    static uint32_t reconnectCount = 0;

    bool currConnected = WiFi.isConnected();

    if (currConnected) {
        reconnectCount = 0;
    }
    else {
        uint32_t now = millis();
        if (reconnectCount == 0 || now - reconnectPrev > reconnectInterval) {
            WiFi.reconnect();
            reconnectCount++;
            if (reconnectCount > reconnectLimit) {
                Serial.println("Failed to reconnect WLAN, about to reset");
                for (int i = 0; i < 20; i++) {
                    digitalWrite(LED_PIN, (i & 1) ? LOW : HIGH);
                    delay(100);
                }
                Serial1.end();
                ESP.restart();
                while (true)
                    ;
            }
            reconnectPrev = now;
        }
    }

    return currConnected;
}

// check ntp status, return true if time is valid
bool check_ntptime() {
    static bool have_time = false;

    bool valid_time = time(0) > 1582230020;

    if (!have_time && valid_time) {
        have_time = true;
        time_t now = time(NULL);
        strftime(start_time, sizeof(start_time), "%F %T", localtime(&now));
        snprintf(msg, sizeof(msg), "Got valid time at %s", start_time);
        slog(msg, LOG_NOTICE);
        msg[0] = '\0';  // clear message after displaying it
    }

    return have_time;
}

void handle_reboot() {
    if (shouldReboot) {
        uint32_t now = millis();
        if (now - shouldReboot > delayReboot) {
            Serial1.end();
            ESP.restart();
        }
    }
}

// Standard web page
const char *main_page(const char *msg = "", bool refresh = false) {
    static const char fmt[] =
        "<!doctype html>\n"
        "<html lang=\"en\">\n"
        " <head>\n"
        "  <meta charset=\"utf-8\">\n"
        "  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
        "  <link href=\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAABAAAAAQAgMAAABinRfyAAAADFBMVEUqYbutnpTMuq/70SQgIef5AAAAVUlEQVQIHWOAAPkvDAyM3+Y7MLA7NV5g4GVqKGCQYWowYTBhapBhMGB04GE4/0X+M8Pxi+6XGS67XzzO8FH+iz/Dl/q/8gx/2S/UM/y/wP6f4T8QAAB3Bx3jhPJqfQAAAABJRU5ErkJggg==\" rel=\"icon\" type=\"image/x-icon\" />\n"
        "  %s<title>" PROGNAME " v" VERSION "</title>\n"
        " </head>\n"
        " <body>\n"
        "  <h1>" PROGNAME " v" VERSION "</h1>\n"
        "  %s<p>update <a href=\"/update\">here</a>\n"
        "  <p>reset <a href=\"/reset\">here</a>\n"
        "  <p><small>Author: Joachim Banzhaf</small><a href=\"https://github.com/joba-1/Joba_Solplanet\" target=\"_blank\"><small> Github</small></a>\n"
        " </body>\n"
        "</html>\n";
    static char page[sizeof(fmt) + 500] = "";
    const char *meta = refresh ? "  <meta http-equiv=\"refresh\" content=\"7; url=/\"> \n" : "";
    snprintf(page, sizeof(page), fmt, meta, msg);
    return page;
}

void setup() {
    pinMode(LED_PIN, OUTPUT);
    digitalWrite(LED_PIN, HIGH);

    Serial.begin(BAUDRATE);
    Serial.println("\nStarting " PROGNAME " v" VERSION " " __DATE__ " " __TIME__);

    String host(HOSTNAME);
    host.toLowerCase();
    WiFi.hostname(host.c_str());
    WiFi.mode(WIFI_STA);

    // Syslog setup
    syslog.server(SYSLOG_SERVER, SYSLOG_PORT);
    syslog.deviceHostname(WiFi.getHostname());
    syslog.appName("Joba1");
    syslog.defaultPriority(LOG_KERN);

    digitalWrite(LED_PIN, LOW);

    WiFiManager wm;
    wm.setConfigPortalTimeout(180);
    if (!wm.autoConnect(WiFi.getHostname(), WiFi.getHostname())) {
        Serial.println("Failed to connect WLAN, about to reset");
        for (int i = 0; i < 20; i++) {
            digitalWrite(LED_PIN, (i & 1) ? HIGH : LOW);
            delay(100);
        }
        Serial1.end();
        ESP.restart();
        while (true)
            ;
    }

    snprintf(msg, sizeof(msg), "%s WLAN IP is %s", HOSTNAME, WiFi.localIP().toString().c_str());
    slog(msg, LOG_NOTICE);

    configTime(3600, 3600, NTP_SERVER);  // MEZ/MESZ

    webServer.on("/", [](AsyncWebServerRequest *request) {
        request->send(200, "text/html", main_page(msg));
        msg[0] = '\0';  // clear message after displaying it
    });

    webServer.on("/wipe", HTTP_POST, [](AsyncWebServerRequest *request) {
        WiFiManager wm;
        wm.resetSettings();
        static const char body[] = "  Wipe WLAN config. Connect to AP '" HOSTNAME "' and open http://192.168.4.1\n";
        slog(body, LOG_NOTICE);
        request->redirect("/");  
        shouldReboot = millis();
        if( shouldReboot == 0 ) {
            shouldReboot--;
        }
    });

    webServer.on("/reset", [](AsyncWebServerRequest *request) {
        static const char body[] = "Reset now\n";
        slog(body, LOG_NOTICE);
        request->redirect("/");  
        shouldReboot = millis();
        if( shouldReboot == 0 ) {
            shouldReboot--;
        }
    });

    // Firmware Update Form
    webServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
        static const char body[] =
            "  <form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\">\n"
            "    <input type=\"file\" name=\"update\">\n"
            "    <input type=\"submit\" value=\"Update\">\n"
            "  </form>\n";
 
        request->send(200, "text/html", main_page(body));
    });

    webServer.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){
        if( !Update.hasError() ) {
            shouldReboot = millis();
            if( shouldReboot == 0 ) {
                shouldReboot--;
            }
            delayReboot = 5000;
        }
        AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot ? "OK" : "FAIL");
        response->addHeader("Connection", "close");
        request->send(response);
    },[](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final){
        if(!index){
            snprintf(msg, sizeof(msg), "Update Start: %s\n", filename.c_str());
            slog(msg, LOG_NOTICE);
            msg[0] = '\0';  // clear message after displaying it
            if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
                snprintf(msg, sizeof(msg), "%s", Update.errorString());
                slog(msg, LOG_ERR);
                request->redirect("/");  
            }
        }
        if(!Update.hasError()){
            if(Update.write(data, len) != len){
                snprintf(msg, sizeof(msg), "%s", Update.errorString());
                slog(msg, LOG_ERR);
                request->redirect("/"); 
            }
        }
        if(final){
            if(Update.end(true)){
                snprintf(msg, sizeof(msg), "Update Success: %u Bytes", index+len);
            } else {
                snprintf(msg, sizeof(msg), "%s", Update.errorString());
            }
            slog(msg, LOG_NOTICE);
            request->redirect("/"); 
        }
    });

    // Catch all page
    webServer.onNotFound( [](AsyncWebServerRequest *request) { 
        request->send(404, "text/html", main_page("<h2>page not found</h2>\n")); 
    });

    webServer.begin();

    ensureMqttConnected();

    // modbus.onData([](uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint16_t address, uint8_t* data, size_t length) {
    //     size_t pos = snprintf(msg, sizeof(msg), "id 0x%02x fc 0x%02x len %u: 0x", serverAddress, fc, length);
    //     for (size_t i = 0; i < length; ++i) {
    //         pos += snprintf(&msg[pos], sizeof(msg) - pos, "%02x", data[i]);
    //     }
    //     snprintf(&msg[pos], sizeof(msg) - pos, "\n");
    //     slog(msg);
    //     msg[0] = '\0';  // clear message after displaying it

    //     // decode according to AISWEI register metadata and publish readable payload
    //     decodeAndPublish(serverAddress, fc, address, data, length);
    // });

    // modbus.onError([](esp32Modbus::Error error) {
    //     snprintf(msg, sizeof(msg), "error: 0x%02x\n", static_cast<uint8_t>(error));
    //     slog(msg);
    // });

    // modbus.begin();

    Serial1.begin(MODBUS_BAUDRATE, SERIAL_8N1, MODBUS_RX, MODBUS_TX, false, MODBUS_TIMEOUT);  // Modbus connection

    // sniffer_init(MODBUS_RX, MODBUS_TX, MODBUS_BAUDRATE);

    slog(PROGNAME " " VERSION " setup done", LOG_NOTICE);

    digitalWrite(LED_PIN, HIGH);
}

void hex_out(const uint8_t* data, size_t length) {
    size_t pos = snprintf(msg, sizeof(msg), "Modbus RX[%3u]: ", (unsigned int)length);
    for (size_t i = 0; i < length; ++i) {
        pos += snprintf(&msg[pos], sizeof(msg) - pos, "%02x ", (unsigned char)data[i]);
    }
    snprintf(&msg[pos], sizeof(msg) - pos, "\n");
    slog(msg);
    msg[0] = '\0';  // clear message after displaying it
}

void handle_modbus() {
    static uint8_t data[256];
    static size_t pos = 0;
    static uint32_t last = 0;

    uint32_t now = millis();

    while (Serial1.available() && pos < sizeof(data)) {
        int c = Serial1.read();
        if (c < 0) break;
        now = millis();
        last = now;
        data[pos++] = (uint8_t)c;
    }
    
    if (pos > 0) {
        uint32_t timeout = (pos <= 8) ? 300 : MODBUS_TIMEOUT;
        if (now - last >= timeout) {
            ModbusRtuDecoder::FramePair pair = {};
            ModbusRtuDecoder decoder;
            bool decoded = decoder.split_request_response(data, pos, pair);
            if (decoded) {
                decoder.frame_pair_to_string(pair, msg, sizeof(msg));
                slog(msg);
                msg[0] = '\0';  // clear message after displaying it
                if (!pair.has_request || !pair.has_response || pair.request.quantity == 0 || (pair.response.byte_count & 1)) {
                    decoded = false;
                } else {
                    // decode according to AISWEI register metadata and publish readable payload
                    auto & resp = pair.response; 
                    auto & req = pair.request; 
                    esp32Modbus::FunctionCode fc = (esp32Modbus::FunctionCode)req.function_code;
                    uint16_t addr = req.start_address;
                    for (uint16_t i = 0; i < resp.byte_count; i += 2) {
                        uint16_t regOff = addr + (i / 2);
                        size_t regLen = 2;
                        // find matching register definition by comparing register offsets
                        for (size_t j = 0; j < aiswei_registers_count; ++j) {
                            const RegisterInfo &r = aiswei_registers[j];
                            uint16_t rOff = aiswei_dec2reg(r.addr);
                            if (regOff == rOff) {
                                regLen = r.length * 2;  // length in bytes
                                break;
                            }
                        }
                        if (i + regLen > resp.byte_count) {
                            // not enough data left
                            break;
                        }
                        decodeAndPublish(req.slave_id, fc, regOff, &resp.data[i], regLen);
                        i += regLen - 2;  // adjust for next register
                    }
                }
            }
            if (!decoded) {
                hex_out(data, pos);
            }

            pos = 0;
        }
    }
}

void loop() {
    static uint32_t lastMillis = 0;
    
    if (handle_wifi()) {
        check_ntptime();

        if (!mqttClient.connected()) {
            ensureMqttConnected();
        } else {
            mqttClient.loop();
        }
    }

    handle_reboot();
    handle_modbus();
    // sniffer_loop();

    if (millis() - lastMillis > 5000) {
        lastMillis = millis();
        // sniffer_print_frames();

        slog("ping", LOG_NOTICE);

        // snprintf(msg, sizeof(msg), "sending Modbus request...\n");
        // slog(msg);
        // msg[0] = '\0';  // clear message after displaying it
        // modbus.readInputRegisters(0x03, 1358, 6);  // read 6 registers starting at address 1358 from device 3
        
        // modbus.readInputRegisters(0x03, 1001, 1);  // read 1 register starting at address 1001 from device 3
    }
}