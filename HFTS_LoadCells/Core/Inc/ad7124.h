/**
 * @file ad7124.h
 * @brief Driver for AD7124-8BCPZ 24-bit sigma-delta ADC on SPI.
 *
 * Uses SPI2. SWO output on PB3 (TRACESWO) - do not use SPI1 so PB3 is free.
 */

#ifndef AD7124_H
#define AD7124_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _SPI_HandleTypeDef;
struct _GPIO_TypeDef;

typedef bool (*ad7124_spi_xfer_fn)(void *user, const uint8_t *tx, uint8_t *rx, uint16_t len);

typedef struct ad7124_dev {
    void *user;
    ad7124_spi_xfer_fn spi_xfer;
} ad7124_dev_t;

void ad7124_swo_init(void);


bool ad7124_init(ad7124_dev_t *dev);
bool ad7124_read_data(ad7124_dev_t *dev, uint32_t *code);
float ad7124_code_to_voltage(uint32_t code);

void ad7124_swo_send_u32(uint32_t value);
/** Print string to SWO (ITM port 0). View in debugger SWV ITM console. PB3 = SWO. */
void ad7124_swo_print(const char *s);

#ifdef __cplusplus
}
#endif

#endif /* AD7124_H */
