#ifndef CRSF_DEBUG_H
#define CRSF_DEBUG_H

#include <stdint.h>

#ifdef debug

#include <Arduino.h>

typedef struct {
    uint32_t sync_ea;
    uint32_t sync_ef;
    uint32_t sync_c8;
    uint32_t sync_other;
    uint32_t crc_ok;
    uint32_t crc_fail;
    uint32_t crc_recover_0x2b;
    uint32_t rx_0x29;
    uint32_t rx_0x2b;
    uint32_t rx_0x2e;
    uint32_t rx_other;
    uint32_t param_req;
    uint32_t param_chunk_req;
    uint32_t param_timeout;
    uint32_t parse_ok;
    uint32_t parse_short;
    uint32_t parse_bad_origin;
    uint32_t parse_bad_field;
    uint32_t parse_bad_chunk;
    uint8_t last_sync_byte;
    uint8_t last_rx_type;
    uint8_t last_parse_field;
    uint8_t last_parse_chunks;
    char last_parse_name[24];
} crsf_debug_stats_t;

extern crsf_debug_stats_t crsf_dbg;

void crsf_dbg_hex(const char *label, const uint8_t *data, uint16_t len, uint16_t max);
void crsf_dbg_print_param_status(void);

#define CRSF_DBG(...) SerialUSB.printf(__VA_ARGS__)

#else

#define CRSF_DBG(...) ((void)0)
static inline void crsf_dbg_hex(const char *label, const uint8_t *data, uint16_t len, uint16_t max) {
    (void)label;
    (void)data;
    (void)len;
    (void)max;
}
static inline void crsf_dbg_print_param_status(void) {}

#endif

#endif
