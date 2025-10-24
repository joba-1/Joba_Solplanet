#include <Arduino.h>
#include <esp32ModbusRTU.h>
#include <WiFi.h>
#include <PubSubClient.h>

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

static WiFiClient espClient;
static PubSubClient mqttClient(espClient);
static const char* mqttPrefix = "joba_aiswei";

#ifndef MQTT_SERVER
#define MQTT_SERVER "job4"
#endif
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

// helper: attempt WiFi connection only if credentials provided at build time
static void connectWiFiIfNeeded() {
#ifdef WIFI_SSID
    if (WiFi.status() == WL_CONNECTED) return;
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.printf("Connecting to WiFi '%s' ", WIFI_SSID);
    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("WiFi connected, IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("WiFi not connected (timed out)");
    }
#else
    Serial.println("WIFI_SSID not defined at build time — assuming network already configured.");
#endif
}

static void ensureMqttConnected() {
    if (mqttClient.connected()) return;
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    Serial.printf("Connecting to MQTT %s:%d ...\n", MQTT_SERVER, MQTT_PORT);
    // client id: joba_aiswei + last 4 of chip id
    uint32_t chipid = (uint32_t)ESP.getEfuseMac();
    char clientId[40];
    snprintf(clientId, sizeof(clientId), "%s-%08X", mqttPrefix, (unsigned int)(chipid & 0xFFFFFFFF));
    if (mqttClient.connect(clientId)) {
        Serial.println("MQTT connected");
    } else {
        Serial.printf("MQTT connect failed, rc=%d\n", mqttClient.state());
    }
}

// extern modbus object
esp32ModbusRTU modbus(&Serial1, 16);  // use Serial1 and pin 16 as RTS

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600, SERIAL_8N1, 17, 4, true);  // Modbus connection

    connectWiFiIfNeeded();
    ensureMqttConnected();

    modbus.onData([](uint8_t serverAddress, esp32Modbus::FunctionCode fc, uint16_t address, uint8_t* data, size_t length) {
        Serial.printf("id 0x%02x fc 0x%02x len %u: 0x", serverAddress, fc, length);
        for (size_t i = 0; i < length; ++i) {
            Serial.printf("%02x", data[i]);
        }
        Serial.printf("\n");

        // prepare topic: joba_aiswei/modbus/<server>/<addr>
        char topic[128];
        snprintf(topic, sizeof(topic), "%s/modbus/%u/%u", mqttPrefix, serverAddress, address);

        // If payload is 4 bytes, try to interpret as float (fix endianness first)
        if (length == 4) {
            uint8_t tmp[4];
            // copy and reverse for little-endian float assumed by device
            for (int i = 0; i < 4; ++i) tmp[i] = data[3 - i];
            float value;
            memcpy(&value, tmp, sizeof(value));
            char payload[64];
            snprintf(payload, sizeof(payload), "%.3f", value);
            if (mqttClient.connected()) {
                mqttClient.publish(topic, payload);
            }
            Serial.printf("val: %.3f\n\n", value);
            return;
        }

        // otherwise publish hex payload
        char payloadHex[128];
        size_t pos = 0;
        for (size_t i = 0; i < length && pos + 3 < sizeof(payloadHex); ++i) {
            pos += snprintf(payloadHex + pos, sizeof(payloadHex) - pos, "%02x", data[i]);
        }
        payloadHex[pos] = '\0';
        if (mqttClient.connected()) {
            mqttClient.publish(topic, payloadHex);
        }
        Serial.print("\n");
    });

    modbus.onError([](esp32Modbus::Error error) {
        Serial.printf("error: 0x%02x\n\n", static_cast<uint8_t>(error));
    });

    modbus.begin();
}

void loop() {
    static uint32_t lastMillis = 0;
    // maintain MQTT connection
    if (!mqttClient.connected()) {
        ensureMqttConnected();
    } else {
        mqttClient.loop();
    }

    if (millis() - lastMillis > 30000) {
        lastMillis = millis();
        Serial.print("sending Modbus request...\n");
        modbus.readInputRegisters(0x01, 52, 2);  // read 2 registers starting at address 52 from slave 1
    }
}