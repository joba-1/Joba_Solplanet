#ifndef MODBUS_SNIFFER_H
#define MODBUS_SNIFFER_H

#include <stdint.h>

void sniffer_init(uint8_t rx_pin, uint8_t tx_pin, uint32_t baudrate);
void sniffer_loop();
void sniffer_print_frames();
uint16_t sniffer_get_frame_count();

#endif