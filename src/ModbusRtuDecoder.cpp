#include "ModbusRtuDecoder.h"
#include <Arduino.h>

ModbusRtuDecoder::ModbusRtuDecoder() {
    frame_data_buffer = new uint8_t[MAX_FRAME_DATA];
    current_frame = nullptr;
}

ModbusRtuDecoder::~ModbusRtuDecoder() {
    if (frame_data_buffer) {
        delete[] frame_data_buffer;
    }
}

uint16_t ModbusRtuDecoder::calculate_crc16(const uint8_t* buffer, uint16_t length) {
    uint16_t crc = 0xFFFF;

    for (uint16_t i = 0; i < length; i++) {
        crc ^= buffer[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc = crc >> 1;
            }
        }
    }

    return crc;
}

bool ModbusRtuDecoder::validate_crc(const uint8_t* buffer, uint16_t length, uint16_t& crc) {
    if (length < 3) {
        return false;
    }

    uint16_t calculated = calculate_crc16(buffer, length - 2);
    uint16_t received = (buffer[length - 1] << 8) | buffer[length - 2];
    crc = calculated;

    return calculated == received;
}

bool ModbusRtuDecoder::decode(const uint8_t* buffer, uint16_t length, Frame& frame) {
    if (length < 5) {
        frame.error = INVALID_LENGTH;
        frame.is_valid = false;
        return false;
    }

    // Validate CRC
    if (!validate_crc(buffer, length, frame.calculated_crc)) {
        frame.error = INVALID_CRC;
        frame.is_valid = false;
        frame.crc = (buffer[length - 1] << 8) | buffer[length - 2];
        return false;
    }

    frame.slave_id = buffer[0];
    frame.function_code = buffer[1];
    frame.crc = (buffer[length - 1] << 8) | buffer[length - 2];
    frame.calculated_crc = frame.calculated_crc;

    // Check if function code is valid
    if (frame.function_code > 0x80) {
        frame.error = INVALID_FUNCTION_CODE;
        frame.is_valid = false;
        return false;
    }

    // Decode based on function code
    switch (frame.function_code) {
        case READ_COILS:
        case READ_DISCRETE_INPUTS:
        case READ_HOLDING_REGISTERS:
        case READ_INPUT_REGISTERS:
            // Request: length = 8 (slave + fc + addr(2) + qty(2) + crc(2))
            // Response: length = 5+ (slave + fc + byte_count + data + crc(2))
            if (length == 8) {
                // Request format
                frame.start_address = (buffer[2] << 8) | buffer[3];
                frame.quantity = (buffer[4] << 8) | buffer[5];
                frame.data_length = 0;
            } else {
                // Response format
                frame.byte_count = buffer[2];
                frame.data_length = frame.byte_count;
                frame.data = frame_data_buffer;
                if (frame.byte_count > 0 && frame.byte_count <= MAX_FRAME_DATA) {
                    memcpy(frame.data, &buffer[3], frame.byte_count);
                }
            }
            break;

        case WRITE_SINGLE_COIL:
        case WRITE_SINGLE_REGISTER:
            frame.start_address = (buffer[2] << 8) | buffer[3];
            frame.data_length = 2;
            frame.data = frame_data_buffer;
            memcpy(frame.data, &buffer[4], 2);
            break;

        case WRITE_MULTIPLE_COILS:
        case WRITE_MULTIPLE_REGISTERS:
            frame.start_address = (buffer[2] << 8) | buffer[3];
            frame.quantity = (buffer[4] << 8) | buffer[5];
            frame.byte_count = buffer[6];
            frame.data_length = frame.byte_count;
            frame.data = frame_data_buffer;
            memcpy(frame.data, &buffer[7], frame.byte_count);
            break;

        default:
            frame.error = INVALID_FUNCTION_CODE;
            frame.is_valid = false;
            return false;
    }

    frame.error = NO_ERROR;
    frame.is_valid = true;
    return true;
}

const char* ModbusRtuDecoder::get_function_name(uint8_t fc) {
    switch (fc) {
        case READ_COILS: return "Read Coils";
        case READ_DISCRETE_INPUTS: return "Read Discrete Inputs";
        case READ_HOLDING_REGISTERS: return "Read Holding Registers";
        case READ_INPUT_REGISTERS: return "Read Input Registers";
        case WRITE_SINGLE_COIL: return "Write Single Coil";
        case WRITE_SINGLE_REGISTER: return "Write Single Register";
        case WRITE_MULTIPLE_COILS: return "Write Multiple Coils";
        case WRITE_MULTIPLE_REGISTERS: return "Write Multiple Registers";
        default: return "Unknown";
    }
}

void ModbusRtuDecoder::print_frame(const Frame& frame) {
    Serial.printf("=== Modbus RTU Frame ===\n");
    Serial.printf("Unit ID: %d\n", frame.slave_id);
    Serial.printf("Function Code: 0x%02X (%s)\n", frame.function_code, get_function_name(frame.function_code));
    
    if (frame.is_valid) {
        if (frame.start_address > 0 || frame.quantity > 0) {
            Serial.printf("Start Address: 0x%04X\n", frame.start_address);
            Serial.printf("Quantity: %d\n", frame.quantity);
        }
        if (frame.data_length > 0) {
            Serial.printf("Data (%d bytes): ", frame.data_length);
            for (uint16_t i = 0; i < frame.data_length; i++) {
                Serial.printf("%02X ", frame.data[i]);
            }
            Serial.printf("\n");
        }
        Serial.printf("CRC: 0x%04X (Valid)\n", frame.crc);
    } else {
        Serial.printf("CRC: 0x%04X (Expected: 0x%04X)\n", frame.crc, frame.calculated_crc);
        Serial.printf("Error: %d\n", frame.error);
    }
    Serial.printf("\n");
}

bool ModbusRtuDecoder::split_request_response(const uint8_t* buffer, uint16_t length, FramePair& pair) {
    pair.has_request = false;
    pair.has_response = false;
    pair.round_trip_time = 0;

    if (length < 10) {
        return false; // Minimum size for request + response
    }

    uint16_t offset = 0;
    Frame* frames[2] = {&pair.request, &pair.response};
    bool decoded[2] = {false, false};

    // Try to find frame boundaries by looking for valid CRC
    for (uint8_t frame_idx = 0; frame_idx < 2 && offset < length; frame_idx++) {
        bool found = false;

        // Minimum frame size is 5 bytes (slave_id + fc + 2 bytes data + 2 CRC)
        for (uint16_t try_len = 5; try_len <= (length - offset) && try_len <= 256; try_len++) {
            uint16_t crc_calc;
            if (validate_crc(&buffer[offset], try_len, crc_calc)) {
                // Found valid CRC, try to decode
                if (decode(&buffer[offset], try_len, *frames[frame_idx])) {
                    offset += try_len;
                    decoded[frame_idx] = true;
                    found = true;
                    break;
                }
            }
        }

        if (!found) {
            break;
        }
    }

    pair.has_request = decoded[0];
    pair.has_response = decoded[1];

    // Validate that request and response match
    if (pair.has_request && pair.has_response) {
        if (pair.request.slave_id != pair.response.slave_id) {
            return false; // Slave ID mismatch
        }
        if (pair.request.function_code != pair.response.function_code) {
            return false; // Function code mismatch
        }
    }

    return (pair.has_request || pair.has_response);
}

void ModbusRtuDecoder::print_frame_pair(const FramePair& pair) {
    Serial.println("\n=== Modbus RTU Request/Response Pair ===");
    
    if (pair.has_request) {
        Serial.println("--- REQUEST ---");
        print_frame(pair.request);
    }
    
    if (pair.has_response) {
        Serial.println("--- RESPONSE ---");
        print_frame(pair.response);
    }
    
    if (pair.has_request && pair.has_response) {
        Serial.printf("Round Trip Time: %lu ms\n", pair.round_trip_time);
    }
    
    Serial.println();
}

uint16_t ModbusRtuDecoder::frame_pair_to_string(const FramePair& pair, char* buffer, uint16_t buffer_size) {
    const uint16_t MAX_BYTES = 128;  // limit for hex data output

    if (!buffer || buffer_size == 0) {
        return 0;
    }

    uint16_t pos = 0;
    int written = 0;

    // Start with slave ID and function code
    if (pair.has_request || pair.has_response) {
        uint8_t slave_id = pair.has_request ? pair.request.slave_id : pair.response.slave_id;
        uint8_t fc = pair.has_request ? pair.request.function_code : pair.response.function_code;
        
        written = snprintf(buffer + pos, buffer_size - pos, "[Unit:%d FC:0x%02X] ", slave_id, fc);
        if (written < 0) return pos;
        pos += written;
    }

    // Request data (compact)
    if (pair.has_request) {
        const Frame& req = pair.request;
        uint16_t addr = req.start_address;
        if (req.slave_id == 3) {
            if (req.function_code == ModbusRtuDecoder::READ_HOLDING_REGISTERS) {
                addr = addr + 40001;
            } else if (req.function_code == ModbusRtuDecoder::READ_INPUT_REGISTERS) {
                addr = addr + 30001;
            }
        }
        written = snprintf(buffer + pos, buffer_size - pos, "REQ[Addr(%2d): %5u-%5u", 
                          req.quantity, addr, addr + req.quantity - 1);
        if (written < 0) return pos;
        pos += written;

        if (req.data_length > 0) {
            written = snprintf(buffer + pos, buffer_size - pos, " Data:");
            if (written < 0) return pos;
            pos += written;

            // Limit data hex output to MAX_BYTES bytes
            uint16_t max_bytes = (req.data_length > MAX_BYTES) ? MAX_BYTES : req.data_length;
            for (uint16_t i = 0; i < max_bytes && pos < buffer_size - 3; i++) {
                const char *sep = (i & 1) ? "" : " ";
                written = snprintf(buffer + pos, buffer_size - pos, "%s%02X", sep, req.data[i]);
                if (written < 0) return pos;
                pos += written;
            }
            if (req.data_length > MAX_BYTES) {
                written = snprintf(buffer + pos, buffer_size - pos, " ...");
                if (written < 0) return pos;
                pos += written;
            }
        }

        written = snprintf(buffer + pos, buffer_size - pos, "] ");
        if (written < 0) return pos;
        pos += written;
    }

    // Response data (compact)
    if (pair.has_response) {
        const Frame& resp = pair.response;
        
        if (resp.is_valid) {
            written = snprintf(buffer + pos, buffer_size - pos, "RESP[Data(%3d)", resp.byte_count);
            if (written < 0) return pos;
            pos += written;

            if (resp.data_length > 0) {
                written = snprintf(buffer + pos, buffer_size - pos, ":");
                if (written < 0) return pos;
                pos += written;

                // Limit data hex output to MAX_BYTES bytes
                uint16_t max_bytes = (resp.data_length > MAX_BYTES) ? MAX_BYTES : resp.data_length;
                for (uint16_t i = 0; i < max_bytes && pos < buffer_size - 3; i++) {
                    const char *sep = (i & 1) ? "" : " ";
                    written = snprintf(buffer + pos, buffer_size - pos, "%s%02X", sep, resp.data[i]);
                    if (written < 0) return pos;
                    pos += written;
                }
                if (resp.data_length > MAX_BYTES) {
                    written = snprintf(buffer + pos, buffer_size - pos, " ...");
                    if (written < 0) return pos;
                    pos += written;
                }
            }

            written = snprintf(buffer + pos, buffer_size - pos, "]");
            if (written < 0) return pos;
            pos += written;
        } else {
            written = snprintf(buffer + pos, buffer_size - pos, "RESP[ERROR:CRC]");
            if (written < 0) return pos;
            pos += written;
        }
    } else if (pair.has_request) {
        written = snprintf(buffer + pos, buffer_size - pos, "RESP[TIMEOUT]");
        if (written < 0) return pos;
        pos += written;
    }

    // Null terminate
    if (pos < buffer_size) {
        buffer[pos] = '\0';
    }

    return pos;
}