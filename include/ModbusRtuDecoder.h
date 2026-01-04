#ifndef MODBUS_RTU_DECODER_H
#define MODBUS_RTU_DECODER_H

/* usage example

#include "ModbusRtuDecoder.h"

ModbusRtuDecoder decoder;

void analyze_modbus_frame(const uint8_t* buffer, uint16_t length) {
    ModbusRtuDecoder::Frame frame = {};
    
    if (decoder.decode(buffer, length, frame)) {
        decoder.print_frame(frame);
    } else {
        Serial.println("Failed to decode frame!");
        decoder.print_frame(frame);
    }
}

void analyze_request_response(const uint8_t* buffer, uint16_t length) {
    ModbusRtuDecoder::FramePair pair = {};
    
    if (decoder.split_request_response(buffer, length, pair)) {
        decoder.print_frame_pair(pair);
        
        if (pair.has_request && pair.has_response) {
            Serial.println("✓ Valid request/response pair");
        } else if (pair.has_request) {
            Serial.println("⚠ Request only");
        } else if (pair.has_response) {
            Serial.println("⚠ Response only");
        }
    } else {
        Serial.println("✗ Failed to parse request/response");
    }
}
    
*/

#include <stdint.h>
#include <string.h>

class ModbusRtuDecoder {
public:
    enum FunctionCode {
        READ_COILS = 0x01,
        READ_DISCRETE_INPUTS = 0x02,
        READ_HOLDING_REGISTERS = 0x03,
        READ_INPUT_REGISTERS = 0x04,
        WRITE_SINGLE_COIL = 0x05,
        WRITE_SINGLE_REGISTER = 0x06,
        WRITE_MULTIPLE_COILS = 0x0F,
        WRITE_MULTIPLE_REGISTERS = 0x10
    };

    enum ErrorCode {
        NO_ERROR = 0,
        INVALID_LENGTH = 1,
        INVALID_CRC = 2,
        INVALID_SLAVE_ID = 3,
        INVALID_FUNCTION_CODE = 4,
        INVALID_DATA = 5
    };

    struct Frame {
        uint8_t slave_id;
        uint8_t function_code;
        uint16_t start_address;
        uint16_t quantity;
        uint8_t byte_count;
        uint8_t* data;
        uint16_t data_length;
        uint16_t crc;
        uint16_t calculated_crc;
        ErrorCode error;
        bool is_valid;
    };

    struct FramePair {
        Frame request;
        Frame response;
        bool has_request;
        bool has_response;
        uint32_t round_trip_time; // ms
    };

    ModbusRtuDecoder();
    ~ModbusRtuDecoder();

    // Decode request or response frame
    bool decode(const uint8_t* buffer, uint16_t length, Frame& frame);

    // Validate frame CRC
    bool validate_crc(const uint8_t* buffer, uint16_t length, uint16_t& crc);

    // Calculate CRC16 (Modbus)
    static uint16_t calculate_crc16(const uint8_t* buffer, uint16_t length);

    // Get human-readable function code name
    static const char* get_function_name(uint8_t fc);

    // Print frame details
    void print_frame(const Frame& frame);

    // Split combined buffer into request and response frames
    bool split_request_response(const uint8_t* buffer, uint16_t length, FramePair& pair);

    // Print frame pair details
    void print_frame_pair(const FramePair& pair);

    // Print frame pair to char buffer in compact format (single line)
    uint16_t frame_pair_to_string(const FramePair& pair, char* buffer, uint16_t buffer_size);

private:
    Frame* current_frame;
    uint8_t* frame_data_buffer;
    static const uint16_t MAX_FRAME_DATA = 252;

    bool decode_request(const uint8_t* buffer, uint16_t length, Frame& frame);
    bool decode_response(const uint8_t* buffer, uint16_t length, Frame& frame);
};

#endif