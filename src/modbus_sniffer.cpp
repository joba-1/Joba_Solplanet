#include <Arduino.h>
#include <HardwareSerial.h>

// Ring buffer for captured data
#define SNIFFER_BUFFER_SIZE 4096
struct SnifferFrame {
    uint32_t timestamp;
    uint16_t length;
    uint8_t direction; // 0 = RX, 1 = TX
    uint8_t data[256];
};

static SnifferFrame frames[16];
static uint8_t frame_index = 0;
static HardwareSerial* sniff_serial = nullptr;
static uint8_t rx_buffer[256];
static uint16_t rx_pos = 0;
static uint32_t last_byte_time = 0;
static const uint32_t MODBUS_FRAME_TIMEOUT = 10; // ms

void sniffer_init(uint8_t rx_pin, uint8_t tx_pin, uint32_t baudrate) {
    sniff_serial = &Serial1;
    sniff_serial->begin(baudrate, SERIAL_8N1, rx_pin, tx_pin);
}

void sniffer_capture_frame(uint8_t direction) {
    if (rx_pos == 0) return;
    
    SnifferFrame* frame = &frames[frame_index % 16];
    frame->timestamp = millis();
    frame->direction = direction;
    frame->length = rx_pos;
    memcpy(frame->data, rx_buffer, rx_pos);
    
    frame_index++;
    rx_pos = 0;
}

void sniffer_loop() {
    if (!sniff_serial) return;
    
    while (sniff_serial->available()) {
        uint8_t byte = sniff_serial->read();
        rx_buffer[rx_pos++] = byte;
        last_byte_time = millis();
        
        if (rx_pos >= 256) {
            sniffer_capture_frame(0); // RX
        }
    }
    
    // Check for frame timeout
    if (rx_pos > 0 && (millis() - last_byte_time) > MODBUS_FRAME_TIMEOUT) {
        sniffer_capture_frame(0); // RX
    }
}

#include <Syslog.h>
extern char msg[512];  // one buffer for all syslog and web messages
void slog(const char *message, uint16_t pri = LOG_INFO);

void sniffer_print_frames() {
    Serial.println("\n=== Modbus Sniffer Frames ===");
    
    for (uint8_t i = 0; i < frame_index && i < 16; i++) {
        SnifferFrame* frame = &frames[i];
        size_t pos = snprintf(msg, sizeof(msg), "[%lu ms] %s (%d bytes): ",
            frame->timestamp,
            frame->direction ? "TX" : "RX",
            frame->length);
        
        for (uint16_t j = 0; j < frame->length; j++) {
            pos += snprintf(msg + pos, sizeof(msg) - pos, "%02X ", frame->data[j]);
        }
        slog(msg);
        msg[0] = '\0';  // clear message after displaying it
    }
}

uint16_t sniffer_get_frame_count() {
    return frame_index;
}