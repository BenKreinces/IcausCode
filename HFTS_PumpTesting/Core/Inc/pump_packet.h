/**
 * @file pump_packet.h
 * @brief UART packet format for HFTS_PumpTesting data stream.
 *
 * TX packet (binary, little-endian): sent every 1 ms.
 *   [0:1]   Sync 0xAA 0x55
 *   [2:5]   Timestamp (uint32_t, ms since boot)
 *   [6:7]   AD7689 IN0 raw (uint16_t)
 *   [8:9]   AD7689 IN1 raw (uint16_t)
 *   [10:11] AD7689 IN2 raw (uint16_t)
 *   [12:15] Flowrate (float, Hz)
 *   [16:19] AD7124 voltage (float, V)
 * Total: 20 bytes.
 *
 * RX solenoid command (placeholder; scheme may be updated later):
 *   Single byte 0x01..0x0C = toggle solenoid 1..12.
 */
#ifndef PUMP_PACKET_H
#define PUMP_PACKET_H

#include <stdint.h>

#define PUMP_PACKET_SYNC0  0xAA
#define PUMP_PACKET_SYNC1  0x55
#define PUMP_PACKET_SIZE   20

#define SOLENOID_CMD_MIN   0x01
#define SOLENOID_CMD_MAX   0x0C

#endif /* PUMP_PACKET_H */
