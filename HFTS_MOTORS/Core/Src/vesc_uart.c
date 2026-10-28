/*
 * vesc_uart.c - minimal VESC UART interface (packet + COMM_GET_VALUES decode)
 */

#include "vesc_uart.h"

#include <string.h>

/* VESC protocol bytes */
#define VESC_FRAME_SHORT_START   2u
#define VESC_FRAME_LONG_START    3u
#define VESC_FRAME_END           3u

/* Commands */
#define COMM_GET_VALUES          4u
#define COMM_SET_RPM             8u
#define COMM_ALIVE               30u

/* ---------------- CRC16-CCITT (poly 0x1021, init 0) ---------------- */
static uint16_t vesc_crc16_ccitt(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000u) {
                crc = (uint16_t)((crc << 1) ^ 0x1021u);
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

/* ---------------- Big-endian helpers ---------------- */
static void vesc_write_be_i32(uint8_t *dst, int32_t v)
{
    dst[0] = (uint8_t)((uint32_t)v >> 24);
    dst[1] = (uint8_t)((uint32_t)v >> 16);
    dst[2] = (uint8_t)((uint32_t)v >> 8);
    dst[3] = (uint8_t)((uint32_t)v);
}

static int16_t vesc_read_be_i16(const uint8_t *src, uint16_t *idx, uint16_t len)
{
    if ((*idx + 2u) > len) return 0;
    int16_t v = (int16_t)((uint16_t)src[*idx] << 8 | (uint16_t)src[*idx + 1u]);
    *idx += 2u;
    return v;
}

static int32_t vesc_read_be_i32(const uint8_t *src, uint16_t *idx, uint16_t len)
{
    if ((*idx + 4u) > len) return 0;
    int32_t v = (int32_t)(
        ((uint32_t)src[*idx] << 24) |
        ((uint32_t)src[*idx + 1u] << 16) |
        ((uint32_t)src[*idx + 2u] << 8) |
        ((uint32_t)src[*idx + 3u]));
    *idx += 4u;
    return v;
}

static float vesc_decode_f16_scaled(const uint8_t *src, uint16_t *idx, uint16_t len, float scale)
{
    int16_t raw = vesc_read_be_i16(src, idx, len);
    return (float)raw / scale;
}

static float vesc_decode_f32_scaled(const uint8_t *src, uint16_t *idx, uint16_t len, float scale)
{
    int32_t raw = vesc_read_be_i32(src, idx, len);
    return (float)raw / scale;
}

/* ---------------- TX framing ---------------- */
static void vesc_send_payload(VescUart *v, const uint8_t *payload, uint16_t payload_len)
{
    if (!v || !v->tx_cb || !payload || payload_len == 0u) return;

    /* We only implement short frames for our use-case (payload small). */
    if (payload_len >= 256u) return;

    /* Frame: [2][len][payload...][crc_hi][crc_lo][3] */
    uint8_t frame[1u + 1u + 128u + 2u + 1u];
    uint16_t idx = 0;

    frame[idx++] = (uint8_t)VESC_FRAME_SHORT_START;
    frame[idx++] = (uint8_t)payload_len;

    memcpy(&frame[idx], payload, payload_len);
    idx += payload_len;

    uint16_t crc = vesc_crc16_ccitt(payload, payload_len);
    frame[idx++] = (uint8_t)(crc >> 8);
    frame[idx++] = (uint8_t)(crc & 0xFFu);
    frame[idx++] = (uint8_t)VESC_FRAME_END;

    v->tx_cb(frame, idx);
}

/* ---------------- Payload decode: COMM_GET_VALUES ---------------- */
static void vesc_decode_get_values(VescUart *v, const uint8_t *payload, uint16_t payload_len)
{
    if (!v || !payload || payload_len < 1u) return;
    if (payload[0] != COMM_GET_VALUES) return;

    /* Payload layout matches the user's required subset and ordering. */
    uint16_t idx = 1u;
    VescValues out;
    memset(&out, 0, sizeof(out));

    /* float16 tempMosfet (scale 1e1) */
    out.tempMosfet = vesc_decode_f16_scaled(payload, &idx, payload_len, 10.0f);

    /* float16 tempMotor (skip) */
    (void)vesc_decode_f16_scaled(payload, &idx, payload_len, 10.0f);

    /* float32 avgMotorCurrent (scale 1e2) */
    out.avgMotorCurrent = vesc_decode_f32_scaled(payload, &idx, payload_len, 100.0f);

    /* float32 avgInputCurrent (scale 1e2) */
    out.avgInputCurrent = vesc_decode_f32_scaled(payload, &idx, payload_len, 100.0f);

    /* float32 id (skip) */
    (void)vesc_read_be_i32(payload, &idx, payload_len);
    /* float32 iq (skip) */
    (void)vesc_read_be_i32(payload, &idx, payload_len);

    /* float16 duty (skip) scale 1e3 */
    (void)vesc_decode_f16_scaled(payload, &idx, payload_len, 1000.0f);

    /* float32 rpm (scale 1) */
    out.rpm = vesc_decode_f32_scaled(payload, &idx, payload_len, 1.0f);

    /* float16 input voltage (scale 1e1) */
    out.inpVoltage = vesc_decode_f16_scaled(payload, &idx, payload_len, 10.0f);

    /* float32 ampHours (scale 1e4) */
    out.ampHours = vesc_decode_f32_scaled(payload, &idx, payload_len, 10000.0f);

    /* float32 ampHoursCharged (skip) */
    (void)vesc_read_be_i32(payload, &idx, payload_len);
    /* float32 wattHours (skip) */
    (void)vesc_read_be_i32(payload, &idx, payload_len);
    /* float32 wattHoursCharged (skip) */
    (void)vesc_read_be_i32(payload, &idx, payload_len);

    /* int32 tachometer (skip) */
    (void)vesc_read_be_i32(payload, &idx, payload_len);

    /* int32 tachometerAbs */
    out.tachometerAbs = (float)vesc_read_be_i32(payload, &idx, payload_len);

    if (v->values_cb) {
        v->values_cb(&out);
    }
}

/* ---------------- Public API ---------------- */
void vesc_init(VescUart *v, vesc_tx_fn_t tx_cb, vesc_values_cb_t values_cb)
{
    if (!v) return;
    memset(v, 0, sizeof(*v));
    v->tx_cb = tx_cb;
    v->values_cb = values_cb;
    v->rx_state = VESC_RX_WAIT_START;
}

void vesc_set_rpm(VescUart *v, int32_t rpm)
{
    uint8_t payload[1u + 4u];
    payload[0] = (uint8_t)COMM_SET_RPM;
    vesc_write_be_i32(&payload[1], rpm);
    vesc_send_payload(v, payload, (uint16_t)sizeof(payload));
}

void vesc_request_values(VescUart *v)
{
    uint8_t payload[1u];
    payload[0] = (uint8_t)COMM_GET_VALUES;
    vesc_send_payload(v, payload, (uint16_t)sizeof(payload));
}

void vesc_send_alive(VescUart *v)
{
    uint8_t payload[1u];
    payload[0] = (uint8_t)COMM_ALIVE;
    vesc_send_payload(v, payload, (uint16_t)sizeof(payload));
}

void vesc_process_rx_byte(VescUart *v, uint8_t byte)
{
    if (!v) return;

    switch (v->rx_state) {
        case VESC_RX_WAIT_START:
            if (byte == VESC_FRAME_SHORT_START) {
                v->rx_long_frame = false;
                v->rx_len = 0u;
                v->rx_idx = 0u;
                v->rx_crc_rx = 0u;
                v->rx_crc_calc = 0u;
                v->rx_state = VESC_RX_LEN_1;
            } else if (byte == VESC_FRAME_LONG_START) {
                v->rx_long_frame = true;
                v->rx_len = 0u;
                v->rx_idx = 0u;
                v->rx_crc_rx = 0u;
                v->rx_crc_calc = 0u;
                v->rx_state = VESC_RX_LEN_1;
            } else {
                /* stay */
            }
            break;

        case VESC_RX_LEN_1:
            if (!v->rx_long_frame) {
                v->rx_len = (uint16_t)byte;
                if (v->rx_len == 0u || v->rx_len > sizeof(v->rx_buf)) {
                    v->rx_state = VESC_RX_WAIT_START;
                } else {
                    v->rx_state = VESC_RX_PAYLOAD;
                }
            } else {
                /* long frame: first length byte is MSB */
                v->rx_len = (uint16_t)byte << 8;
                v->rx_state = VESC_RX_LEN_2;
            }
            break;

        case VESC_RX_LEN_2:
            v->rx_len |= (uint16_t)byte;
            if (v->rx_len == 0u || v->rx_len > sizeof(v->rx_buf)) {
                v->rx_state = VESC_RX_WAIT_START;
            } else {
                v->rx_state = VESC_RX_PAYLOAD;
            }
            break;

        case VESC_RX_PAYLOAD:
            v->rx_buf[v->rx_idx++] = byte;
            if (v->rx_idx >= v->rx_len) {
                v->rx_state = VESC_RX_CRC_1;
            }
            break;

        case VESC_RX_CRC_1:
            v->rx_crc_rx = (uint16_t)byte << 8;
            v->rx_state = VESC_RX_CRC_2;
            break;

        case VESC_RX_CRC_2:
            v->rx_crc_rx |= (uint16_t)byte;
            v->rx_state = VESC_RX_END;
            break;

        case VESC_RX_END:
            if (byte == VESC_FRAME_END) {
                v->rx_crc_calc = vesc_crc16_ccitt(v->rx_buf, v->rx_len);
                if (v->rx_crc_calc == v->rx_crc_rx) {
                    vesc_decode_get_values(v, v->rx_buf, v->rx_len);
                }
            }
            v->rx_state = VESC_RX_WAIT_START;
            break;

        default:
            v->rx_state = VESC_RX_WAIT_START;
            break;
    }
}

