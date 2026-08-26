/**
 ******************************************************************************
 * @file    board_pins.h
 * @brief   Board pin definitions for load-cell ADC path (AD7124-8 on SPI2).
 *          LC = Load Cell. CS for the load-cell ADC is labeled LC.
 ******************************************************************************
 * Pins:
 *   SPI2: SCK=PB13, MISO=PB14, MOSI=PB15
 *   LC_CS: PD4 (chip select for AD7124 load-cell ADC)
 *   Optional: LC_DRDY - define AD7124_USE_DRDY to use DRDY polling
 ******************************************************************************
 */
#ifndef BOARD_PINS_H
#define BOARD_PINS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

/* LC (Load Cell) AD7124 chip select - uses SPI2 */
#define LC_CS_Pin        SPI2_CS_LC_Pin
#define LC_CS_GPIO_Port  SPI2_CS_LC_GPIO_Port

/* Optional: define AD7124_USE_DRDY if DRDY pin is connected for conversion-ready polling */
/* #define AD7124_USE_DRDY */

#ifdef __cplusplus
}
#endif

#endif /* BOARD_PINS_H */
