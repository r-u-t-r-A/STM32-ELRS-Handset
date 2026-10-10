#ifdef debug

#include "crsf_debug.h"
#include "crsf_dynamic.h"
#include <Arduino.h>
#include <stdio.h>

extern dynamic_param_manager_t param_manager;

crsf_debug_stats_t crsf_dbg = {0};

void crsf_dbg_hex(const char *label, const uint8_t *data, uint16_t len, uint16_t max) {
    if (!label || !data || len == 0) {
        return;
    }
    if (max > 0 && len > max) {
        len = max;
    }
    SerialUSB.print(label);
    SerialUSB.print(":");
    for (uint16_t i = 0; i < len; i++) {
        SerialUSB.printf(" %02X", data[i]);
    }
    SerialUSB.println();
}

void crsf_dbg_print_param_status(void) {
    SerialUSB.println("--- CRSF param debug ---");
    SerialUSB.printf("fields %u/%u loaded=%u queue=%u pending=%u chunk=%u\n",
                     param_manager.fields_loaded,
                     param_manager.fields_count,
                     param_manager.params_loaded,
                     param_manager.load_queue_len,
                     param_manager.pending_field_id,
                     param_manager.field_chunk);
    SerialUSB.printf("sync C8=%lu EA=%lu EF=%lu other=%lu (last=0x%02X)\n",
                     (unsigned long)crsf_dbg.sync_c8,
                     (unsigned long)crsf_dbg.sync_ea,
                     (unsigned long)crsf_dbg.sync_ef,
                     (unsigned long)crsf_dbg.sync_other,
                     crsf_dbg.last_sync_byte);
    SerialUSB.printf("rx CRC ok=%lu fail=%lu recover_0x2B=%lu | 0x29=%lu 0x2B=%lu 0x2E=%lu other=%lu\n",
                     (unsigned long)crsf_dbg.crc_ok,
                     (unsigned long)crsf_dbg.crc_fail,
                     (unsigned long)crsf_dbg.crc_recover_0x2b,
                     (unsigned long)crsf_dbg.rx_0x29,
                     (unsigned long)crsf_dbg.rx_0x2b,
                     (unsigned long)crsf_dbg.rx_0x2e,
                     (unsigned long)crsf_dbg.rx_other);
    SerialUSB.printf("tx req=%lu chunk_req=%lu timeout=%lu\n",
                     (unsigned long)crsf_dbg.param_req,
                     (unsigned long)crsf_dbg.param_chunk_req,
                     (unsigned long)crsf_dbg.param_timeout);
    SerialUSB.printf("parse ok=%lu short=%lu bad_origin=%lu bad_field=%lu bad_chunk=%lu\n",
                     (unsigned long)crsf_dbg.parse_ok,
                     (unsigned long)crsf_dbg.parse_short,
                     (unsigned long)crsf_dbg.parse_bad_origin,
                     (unsigned long)crsf_dbg.parse_bad_field,
                     (unsigned long)crsf_dbg.parse_bad_chunk);
    if (crsf_dbg.last_parse_name[0]) {
        SerialUSB.printf("last parsed: id=%u chunks=%u name=%s\n",
                         crsf_dbg.last_parse_field,
                         crsf_dbg.last_parse_chunks,
                         crsf_dbg.last_parse_name);
    }
    SerialUSB.printf("last rx type=0x%02X\n", crsf_dbg.last_rx_type);
    SerialUSB.println("------------------------");
}

#endif
