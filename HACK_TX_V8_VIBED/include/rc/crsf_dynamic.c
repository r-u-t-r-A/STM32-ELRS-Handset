/*
 * Dynamic Parameter Loading - elrsV3.lua compatible CRSF parameter discovery
 */

#include "crsf_dynamic.h"
#include "crsf_protocol.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef debug
#include "crsf_debug.h"
extern crsf_debug_stats_t crsf_dbg;
#endif

static uint32_t field_get_value_be(const uint8_t *data, uint16_t offset, uint8_t size) {
    uint32_t result = 0;
    for (uint8_t i = 0; i < size; i++) {
        result = (result << 8) + data[offset + i];
    }
    return result;
}

static void field_unsigned_to_signed(dynamic_field_t *field, uint8_t size) {
    uint32_t bandval = 0x80UL << ((size - 1) * 8);
    if (field->value & bandval) {
        field->value -= (int32_t)(bandval * 2);
    }
    if (field->min & bandval) {
        field->min -= (int32_t)(bandval * 2);
    }
    if (field->max & bandval) {
        field->max -= (int32_t)(bandval * 2);
    }
}

static uint16_t extract_cstring(const uint8_t *data, uint16_t offset, uint16_t len,
                                char *out, uint16_t out_size) {
    uint16_t i = 0;
    while (offset < len && data[offset] != 0 && i < out_size - 1) {
        out[i++] = (char)data[offset++];
    }
    out[i] = 0;
    if (offset < len && data[offset] == 0) {
        offset++;
    }
    return offset;
}

static uint8_t count_option_separators(const char *blob) {
    uint8_t count = 0;
    if (!blob || !blob[0]) {
        return 0;
    }
    for (const char *p = blob; *p; p++) {
        if (*p == ';') {
            count++;
        }
    }
    return (uint8_t)(count + 1);
}

static void option_label_at(const char *blob, uint8_t index, char *out, uint16_t out_size);

static void strip_packet_rate_text(char *label) {
    if (!label || !label[0]) {
        return;
    }

    const char *needle = "packet rate";
    const size_t needle_len = 11;

    for (size_t i = 0; label[i]; i++) {
        size_t j = 0;
        while (j < needle_len && label[i + j]) {
            char a = label[i + j];
            char b = needle[j];
            if (a >= 'A' && a <= 'Z') {
                a = (char)(a + ('a' - 'A'));
            }
            if (a != b) {
                break;
            }
            j++;
        }
        if (j == needle_len) {
            memmove(label + i, label + i + needle_len, strlen(label + i + needle_len) + 1);
            if (i > 0) {
                i--;
            }
        }
    }

    size_t len = strlen(label);
    while (len > 0 && label[len - 1] == ' ') {
        label[--len] = 0;
    }
    while (label[0] == ' ') {
        memmove(label, label + 1, len);
        len--;
    }
}

static void chop_packet_rate_label(char *label) {
    if (!label || !label[0]) {
        return;
    }

    char *hz = strstr(label, "Hz");
    if (hz) {
        char *start = hz;
        while (start > label && start[-1] >= '0' && start[-1] <= '9') {
            start--;
        }
        if (start != hz) {
            memmove(label, start, (size_t)(hz + 2 - start));
            label[hz + 2 - start] = 0;
            return;
        }
    }

    const char prefix = label[0];
    if ((prefix == 'F' || prefix == 'D' || prefix == 'f' || prefix == 'd') &&
        label[1] >= '0' && label[1] <= '9') {
        uint16_t i = 1;
        while (label[i] >= '0' && label[i] <= '9') {
            i++;
        }
        label[i] = 0;
    }
}

static uint8_t field_is_telemetry_ratio(const char *name) {
    if (!name) {
        return 0;
    }
    return strncmp(name, "Telem Ratio", 32) == 0
        || strncmp(name, "Telemetry Ratio", 32) == 0;
}

uint8_t dynamic_field_compact_display(const dynamic_field_t *field) {
    if (!field) {
        return 0;
    }
    return strncmp(field->name, "Packet Rate", sizeof(field->name)) == 0
        || field_is_telemetry_ratio(field->name);
}

uint8_t dynamic_menu_dirty = 0;

static dynamic_param_manager_t *cmd_popup_manager;

typedef struct {
    uint8_t field_id;
    uint32_t next_poll_ms;
    uint8_t active;
    char banner[20];
    uint32_t banner_until_ms;
} dynamic_command_popup_t;

static dynamic_command_popup_t cmd_popup;

extern void dynamic_param_send_command(uint8_t device_id, uint8_t field_id, uint8_t status_byte);

static uint32_t command_poll_interval_ms(const dynamic_field_t *field) {
    return field && field->maxlen ? (uint32_t)field->maxlen * 10UL : 2000UL;
}

static void dynamic_command_popup_on_update(dynamic_field_t *field, uint8_t prev_status) {
    if (!cmd_popup.active || !field || field->id != cmd_popup.field_id) {
        return;
    }

    cmd_popup.next_poll_ms = millis() + command_poll_interval_ms(field);

    uint8_t need_dirty = (field->value != prev_status) ? 1 : 0;

    if (field->value == 2 && field->options_blob[0]) {
        if (strncmp(cmd_popup.banner, field->options_blob, sizeof(cmd_popup.banner)) != 0) {
            strncpy(cmd_popup.banner, field->options_blob, sizeof(cmd_popup.banner) - 1);
            cmd_popup.banner[sizeof(cmd_popup.banner) - 1] = 0;
            cmd_popup.banner_until_ms = millis() + 5000;
            need_dirty = 1;
        }
    } else if (field->value == 0 && prev_status == 2) {
        const char *text = field->options_blob[0] ? field->options_blob : "Stopped";
        if (strncmp(cmd_popup.banner, text, sizeof(cmd_popup.banner)) != 0) {
            strncpy(cmd_popup.banner, text, sizeof(cmd_popup.banner) - 1);
            cmd_popup.banner[sizeof(cmd_popup.banner) - 1] = 0;
            cmd_popup.banner_until_ms = millis() + 3000;
            need_dirty = 1;
        }
    }

    if (need_dirty) {
        dynamic_menu_dirty = 1;
    }
}

const char *dynamic_command_popup_banner(void) {
    if (!cmd_popup.banner[0]) {
        return NULL;
    }
    if (millis() >= cmd_popup.banner_until_ms) {
        return NULL;
    }
    return cmd_popup.banner;
}

uint8_t dynamic_command_popup_banner_take_edge(void) {
    static uint8_t was_visible;
    uint8_t now_visible = dynamic_command_popup_banner() ? 1 : 0;
    if (now_visible == was_visible) {
        return 0;
    }
    was_visible = now_visible;
    return 1;
}

void dynamic_command_popup_show_notice(const char *text, uint32_t duration_ms) {
    if (!text || !text[0]) {
        return;
    }
    strncpy(cmd_popup.banner, text, sizeof(cmd_popup.banner) - 1);
    cmd_popup.banner[sizeof(cmd_popup.banner) - 1] = 0;
    cmd_popup.banner_until_ms = millis() + duration_ms;
    dynamic_menu_dirty = 1;
}

uint8_t dynamic_menu_take_dirty(void) {
    if (!dynamic_menu_dirty) {
        return 0;
    }
    dynamic_menu_dirty = 0;
    return 1;
}

void dynamic_command_popup_start(uint8_t field_id, uint8_t timeout_units) {
    cmd_popup.field_id = field_id;
    cmd_popup.active = 1;
    cmd_popup.banner[0] = 0;
    cmd_popup.banner_until_ms = 0;
    cmd_popup.next_poll_ms = millis() + (timeout_units ? (uint32_t)timeout_units * 10UL : 2000UL);
}

void dynamic_command_popup_clear(void) {
    cmd_popup.active = 0;
    cmd_popup.field_id = 0;
    cmd_popup.banner[0] = 0;
    cmd_popup.banner_until_ms = 0;
}

uint8_t dynamic_command_popup_active(void) {
    return cmd_popup.active;
}

uint8_t dynamic_command_popup_field_id(void) {
    return cmd_popup.field_id;
}

uint8_t dynamic_command_popup_tick(unsigned long now, uint8_t device_id) {
    if (!cmd_popup.active || !cmd_popup_manager) {
        return 0;
    }

    dynamic_field_t *field = dynamic_get_field_by_id(cmd_popup_manager, cmd_popup.field_id);
    if (!field || field->type != CRSF_COMMAND) {
        dynamic_command_popup_clear();
        return 1;
    }

    if (field->value == 0) {
        if (dynamic_command_popup_banner()) {
            return 0;
        }
        dynamic_command_popup_clear();
        return 1;
    }

    if (field->value == 3) {
        return 0;
    }

    if ((field->value == 1 || field->value == 2) && now >= cmd_popup.next_poll_ms) {
        dynamic_param_send_command(device_id, cmd_popup.field_id, CRSF_CMD_STATUS_QUERY);
        cmd_popup.next_poll_ms = now + command_poll_interval_ms(field);
    }

    return 0;
}

uint8_t dynamic_param_command_tx_byte(const dynamic_field_t *field) {
    if (!field || field->type != CRSF_COMMAND) {
        return 0;
    }
    if (field->value == 3) {
        return CRSF_CMD_STATUS_CONFIRM;
    }
    if (field->value == 0) {
        return CRSF_CMD_STATUS_START;
    }
    return 0;
}

static void chop_telemetry_ratio_label(char *label) {
    if (!label || !label[0]) {
        return;
    }

    char *ratio = strstr(label, "1:");
    if (!ratio) {
        return;
    }

    char *end = ratio + 2;
    while (*end >= '0' && *end <= '9') {
        end++;
    }

    if (end == ratio + 2) {
        return;
    }

    memmove(label, ratio, (size_t)(end - ratio));
    label[end - ratio] = 0;
}

static void shorten_options_blob_telem_ratio(char *blob, uint16_t blob_size) {
    if (!blob || !blob[0] || blob_size < 2) {
        return;
    }

    const uint8_t option_count = count_option_separators(blob);
    char rebuilt[DYNAMIC_OPTIONS_BLOB_SIZE];
    uint16_t out_pos = 0;

    rebuilt[0] = 0;
    for (uint8_t i = 0; i < option_count; i++) {
        char option[40];
        option_label_at(blob, i, option, sizeof(option));
        if (!option[0]) {
            continue;
        }
        chop_telemetry_ratio_label(option);

        if (out_pos > 0) {
            rebuilt[out_pos++] = ';';
        }
        const uint16_t olen = (uint16_t)strlen(option);
        if (out_pos + olen >= blob_size) {
            break;
        }
        memcpy(rebuilt + out_pos, option, olen);
        out_pos += olen;
    }
    rebuilt[out_pos] = 0;
    strncpy(blob, rebuilt, blob_size - 1);
    blob[blob_size - 1] = 0;
}

static void shorten_options_blob_hz(char *blob, uint16_t blob_size) {
    if (!blob || !blob[0] || blob_size < 2) {
        return;
    }

    const uint8_t option_count = count_option_separators(blob);
    char rebuilt[DYNAMIC_OPTIONS_BLOB_SIZE];
    uint16_t out_pos = 0;

    rebuilt[0] = 0;
    for (uint8_t i = 0; i < option_count; i++) {
        char option[40];
        option_label_at(blob, i, option, sizeof(option));
        if (!option[0]) {
            continue;
        }
        chop_packet_rate_label(option);
        strip_packet_rate_text(option);

        if (out_pos > 0) {
            rebuilt[out_pos++] = ';';
        }
        const uint16_t olen = (uint16_t)strlen(option);
        if (out_pos + olen >= blob_size) {
            break;
        }
        memcpy(rebuilt + out_pos, option, olen);
        out_pos += olen;
    }
    rebuilt[out_pos] = 0;
    strncpy(blob, rebuilt, blob_size - 1);
    blob[blob_size - 1] = 0;
}

static void option_label_at(const char *blob, uint8_t index, char *out, uint16_t out_size) {
    if (!out || out_size < 2 || !blob) {
        if (out && out_size) {
            out[0] = 0;
        }
        return;
    }

    const char *start = blob;
    while (index > 0 && *start) {
        while (*start && *start != ';') {
            start++;
        }
        if (*start == ';') {
            start++;
        }
        index--;
    }

    uint16_t i = 0;
    while (start[i] && start[i] != ';' && i < out_size - 1) {
        out[i] = start[i];
        i++;
    }
    out[i] = 0;
}

static uint16_t extract_options_blob(const uint8_t *data, uint16_t offset, uint16_t len,
                                     char *blob, uint16_t blob_size, uint8_t *option_count) {
    uint16_t opt_end = offset;
    while (opt_end < len && data[opt_end] != 0) {
        opt_end++;
    }

    uint16_t copy_len = opt_end - offset;
    if (copy_len >= blob_size) {
        copy_len = blob_size - 1;
    }
    if (copy_len > 0) {
        memcpy(blob, &data[offset], copy_len);
    }
    blob[copy_len] = 0;
    *option_count = count_option_separators(blob);

    if (opt_end < len && data[opt_end] == 0) {
        opt_end++;
    }
    return opt_end;
}

static void field_unsigned_load(dynamic_field_t *field, const uint8_t *data,
                                uint16_t offset, uint8_t size, uint16_t len,
                                uint16_t unit_offset) {
    field->value = (int32_t)field_get_value_be(data, offset, size);
    field->min = (int32_t)field_get_value_be(data, offset + size, size);
    field->max = (int32_t)field_get_value_be(data, offset + 2 * size, size);
    if (size != 1) {
        field->size = (int8_t)size;
    } else {
        field->size = 1;
    }
    extract_cstring(data, offset + unit_offset, len, field->unit, sizeof(field->unit));
}

static void field_signed_load(dynamic_field_t *field, const uint8_t *data,
                              uint16_t offset, uint8_t size, uint16_t len,
                              uint16_t unit_offset) {
    field_unsigned_load(field, data, offset, size, len, unit_offset);
    field_unsigned_to_signed(field, size);
    field->size = -(int8_t)size;
}

static void field_int_load(dynamic_field_t *field, const uint8_t *data,
                           uint16_t offset, uint16_t len) {
    uint8_t size = (uint8_t)(field->type / 2) + 1;
    if (field->type % 2 == 0) {
        field_unsigned_load(field, data, offset, size, len, 4 * size);
    } else {
        field_signed_load(field, data, offset, size, len, 4 * size);
    }
}

static void field_float_load(dynamic_field_t *field, const uint8_t *data,
                             uint16_t offset, uint16_t len) {
    field_signed_load(field, data, offset, 4, len, 21);
    if (offset + 16 < len) {
        field->precision = data[offset + 16];
        if (field->precision > 3) {
            field->precision = 3;
        }
    }
    field->step = (int32_t)field_get_value_be(data, offset + 17, 4);
    snprintf(field->format_str, sizeof(field->format_str), "%%.%df", field->precision);
}

static void field_text_sel_load(dynamic_field_t *field, const uint8_t *data,
                                uint16_t offset, uint16_t len) {
    offset = extract_options_blob(data, offset, len, field->options_blob,
                                  sizeof(field->options_blob), &field->option_count);
    if (strncmp(field->name, "Packet Rate", sizeof(field->name)) == 0) {
        shorten_options_blob_hz(field->options_blob, sizeof(field->options_blob));
    } else if (field_is_telemetry_ratio(field->name)) {
        shorten_options_blob_telem_ratio(field->options_blob, sizeof(field->options_blob));
    }
    if (field->option_count <= 1) {
        field->grey = 1;
    }
    if (offset < len) {
        field->value = data[offset];
        offset += 4;
    }
    if (offset < len) {
        extract_cstring(data, offset, len, field->unit, sizeof(field->unit));
    }
    if (strncmp(field->name, "Packet Rate", sizeof(field->name)) == 0
        || field_is_telemetry_ratio(field->name)) {
        field->unit[0] = 0;
    }
}

static void field_command_load(dynamic_field_t *field, const uint8_t *data,
                               uint16_t offset, uint16_t len) {
    if (!field || offset >= len) {
        return;
    }
    uint8_t new_status = data[offset];
    uint8_t prev_status = (uint8_t)field->value;

    if (cmd_popup.active && field->id == cmd_popup.field_id
        && new_status == 0 && field->value == CRSF_CMD_STATUS_START) {
        if (offset + 1 < len) {
            field->maxlen = data[offset + 1];
        }
        return;
    }

    field->value = new_status;
    if (offset + 1 < len) {
        field->maxlen = data[offset + 1];
        offset += 2;
        extract_cstring(data, offset, len, field->options_blob,
                        sizeof(field->options_blob));
    }

    if (cmd_popup.active && field->id == cmd_popup.field_id) {
        dynamic_command_popup_on_update(field, prev_status);
    } else if (new_status != prev_status) {
        dynamic_menu_dirty = 1;
    }
}

static void field_string_load(dynamic_field_t *field, const uint8_t *data,
                              uint16_t offset, uint16_t len) {
    char value[32];
    offset = extract_cstring(data, offset, len, value, sizeof(value));
    if (value[0]) {
        strncpy(field->name, value, sizeof(field->name) - 1);
        field->name[sizeof(field->name) - 1] = 0;
    }
    if (offset < len) {
        field->maxlen = data[offset];
    }
}

static uint8_t field_type_is_valid(uint8_t type) {
    return type <= CRSF_COMMAND || type == CRSF_VTX;
}

static void update_fields_loaded_count(dynamic_param_manager_t *manager) {
    uint16_t count = 0;
    for (uint16_t i = 1; i <= manager->fields_count; i++) {
        if (manager->fields[i].loaded) {
            count++;
        }
    }
    manager->fields_loaded = count;
}

static uint8_t parse_field_payload(dynamic_param_manager_t *manager, uint8_t field_id,
                                const uint8_t *data, uint16_t len) {
    if (!manager || !manager->fields || field_id == 0 || field_id > manager->fields_count) {
#ifdef debug
        if (manager && !manager->fields) {
            CRSF_DBG("parse_field_payload: fields not allocated\n");
        }
#endif
        return 0;
    }
    if (len < 3) {
        return 0;
    }

    dynamic_field_t *field = &manager->fields[field_id];
    if (field->loaded) {
        if (field->type == CRSF_COMMAND) {
            uint16_t cmd_offset = extract_cstring(data, 2, len, field->name, sizeof(field->name));
            field_command_load(field, data, cmd_offset, len);
            return 1;
        }
        return 1;
    }
    memset(field, 0, sizeof(*field));
    field->id = field_id;
    field->parent = data[0] ? data[0] : DYNAMIC_ROOT_FOLDER;
    field->type = data[1] & 0x7F;
    field->hidden = (data[1] & 0x80) ? 1 : 0;
    field->step = 1;

    uint16_t offset = extract_cstring(data, 2, len, field->name, sizeof(field->name));

    switch (field->type) {
    case CRSF_UINT8:
    case CRSF_INT8:
    case CRSF_UINT16:
    case CRSF_INT16:
    case CRSF_UINT32:
    case CRSF_INT32:
        field_int_load(field, data, offset, len);
        break;
    case CRSF_FLOAT:
        field_float_load(field, data, offset, len);
        break;
    case CRSF_TEXT_SELECTION:
        field_text_sel_load(field, data, offset, len);
        break;
    case CRSF_STRING:
    case CRSF_INFO:
        field_string_load(field, data, offset, len);
        break;
    case CRSF_FOLDER:
        break;
    case CRSF_COMMAND:
        field_command_load(field, data, offset, len);
        break;
    default:
        return 0;
    }

    if (!field_type_is_valid(field->type)) {
        memset(field, 0, sizeof(*field));
        return 0;
    }

    field->loaded = 1;
    update_fields_loaded_count(manager);
    return 1;
}

static void reload_all_fields(dynamic_param_manager_t *manager) {
    manager->load_queue_len = 0;
    // CRSF parameter indices are 1-based (1 .. fields_count), same as elrsV3.lua
    for (int16_t id = (int16_t)manager->fields_count; id >= 1; id--) {
        if (manager->load_queue_len < sizeof(manager->load_queue)) {
            manager->load_queue[manager->load_queue_len++] = (uint8_t)id;
        }
    }
    manager->field_chunk = 0;
    manager->chunk_need_request = 0;
    manager->expect_chunks_remain = -1;
    manager->field_data_len = 0;
    manager->fields_loaded = 0;
    manager->params_loaded = 0;
    manager->pending_field_id = PARAM_PENDING_NONE;
    manager->pending_since = 0;

    if (manager->fields && manager->fields_alloc_count > 0) {
        memset(manager->fields, 0,
               (size_t)manager->fields_alloc_count * sizeof(dynamic_field_t));
    }
}

void dynamic_param_init(dynamic_param_manager_t *manager, uint16_t max_fields) {
    if (!manager) {
        return;
    }
    memset(manager, 0, sizeof(*manager));
    manager->max_fields = max_fields;
    manager->fields = NULL;
    manager->expect_chunks_remain = -1;
    manager->pending_field_id = PARAM_PENDING_NONE;
    cmd_popup_manager = manager;
}

void dynamic_param_cleanup(dynamic_param_manager_t *manager) {
    if (!manager) {
        return;
    }
    if (manager->fields) {
        free(manager->fields);
        manager->fields = NULL;
    }
    manager->fields_alloc_count = 0;
    memset(manager, 0, sizeof(*manager));
}

void dynamic_param_reset_for_reload(dynamic_param_manager_t *manager) {
    if (!manager) {
        return;
    }
    if (manager->fields) {
        free(manager->fields);
        manager->fields = NULL;
    }
    uint16_t max_fields = manager->max_fields;
    memset(manager, 0, sizeof(*manager));
    manager->max_fields = max_fields;
    manager->expect_chunks_remain = -1;
    manager->pending_field_id = PARAM_PENDING_NONE;
    cmd_popup_manager = manager;
}

void dynamic_param_on_device_info(dynamic_param_manager_t *manager, uint8_t param_count) {
    if (!manager || param_count == 0) {
        return;
    }
    if (manager->params_loaded || manager->fields_count > 0) {
        return;
    }
    if (param_count > manager->max_fields) {
        param_count = (uint8_t)manager->max_fields;
    }
    if (!manager->fields) {
        manager->fields_alloc_count = (uint16_t)(param_count + 1);
        manager->fields = (dynamic_field_t *)calloc(manager->fields_alloc_count, sizeof(dynamic_field_t));
        if (!manager->fields) {
            manager->fields_alloc_count = 0;
#ifdef debug
            CRSF_DBG("fields alloc failed for %u params\n", param_count);
#endif
            return;
        }
    }
    manager->fields_count = param_count;
    reload_all_fields(manager);
}

uint8_t dynamic_param_tick(dynamic_param_manager_t *manager, uint8_t device_id,
                           void (*read_param_fn)(uint8_t field_id, uint8_t chunk, uint8_t device_id)) {
    if (!manager || manager->params_loaded || manager->fields_count == 0 || !read_param_fn) {
        return manager->params_loaded;
    }
    if (manager->pending_field_id != PARAM_PENDING_NONE) {
        return 0;
    }
    if (manager->load_queue_len == 0) {
        manager->params_loaded = 1;
        return 1;
    }

    uint8_t field_id = manager->load_queue[manager->load_queue_len - 1];
    manager->pending_field_id = field_id;
    read_param_fn(field_id, manager->field_chunk, device_id);
    return 0;
}

static void param_reset_chunk_state(dynamic_param_manager_t *manager) {
    manager->field_chunk = 0;
    manager->chunk_need_request = 0;
    manager->expect_chunks_remain = -1;
    manager->field_data_len = 0;
}

uint8_t dynamic_param_parse_message(dynamic_param_manager_t *manager,
                                    const uint8_t *buffer, uint16_t buffer_len,
                                    uint8_t expected_device_id) {
    if (!manager || !buffer || buffer_len < 5) {
#ifdef debug
        crsf_dbg.parse_short++;
#endif
        return 0;
    }

    // Extended frame payload: [dest][origin][fieldId][chunksRemain][data...]
    uint8_t origin = buffer[1];
    uint8_t field_id = buffer[2];
    uint8_t chunks_remain = buffer[3];

    if (origin != expected_device_id) {
#ifdef debug
        crsf_dbg.parse_bad_origin++;
        CRSF_DBG("parse reject: origin 0x%02X != 0x%02X\n", origin, expected_device_id);
#endif
        param_reset_chunk_state(manager);
        return 0;
    }

    if (manager->pending_field_id != PARAM_PENDING_NONE &&
        field_id != manager->pending_field_id) {
        if (field_id > 0 && field_id <= manager->fields_count &&
            manager->fields[field_id].loaded &&
            manager->fields[field_id].type != CRSF_COMMAND) {
            return 0;
        }
#ifdef debug
        crsf_dbg.parse_bad_field++;
        CRSF_DBG("parse reject: field %u != pending %u\n",
                 field_id, manager->pending_field_id);
#endif
        return 0;
    }

    if (manager->field_data_len > 0 &&
        manager->expect_chunks_remain >= 0 &&
        chunks_remain != (uint8_t)manager->expect_chunks_remain) {
#ifdef debug
        crsf_dbg.parse_bad_chunk++;
#endif
        return 0;
    }

    if (manager->field_data_len == 0 && manager->field_chunk > 0) {
#ifdef debug
        CRSF_DBG("parse reject: missing chunk 0 before chunk %u\n", manager->field_chunk);
#endif
        param_reset_chunk_state(manager);
        manager->chunk_need_request = 1;
        return 0;
    }

    uint16_t data_offset = 4;
    if (chunks_remain > 0 || manager->field_chunk > 0) {
        for (uint16_t i = data_offset; i < buffer_len; i++) {
            if (manager->field_data_len < DYNAMIC_FIELD_DATA_SIZE) {
                manager->field_data[manager->field_data_len++] = buffer[i];
            }
        }
        if (chunks_remain > 0) {
            manager->field_chunk++;
            manager->expect_chunks_remain = (int8_t)(chunks_remain - 1);
            manager->chunk_need_request = 1;
            manager->last_load_time = 0;
            return 0;
        }
    } else {
        manager->field_data_len = 0;
        for (uint16_t i = data_offset; i < buffer_len; i++) {
            if (manager->field_data_len < DYNAMIC_FIELD_DATA_SIZE) {
                manager->field_data[manager->field_data_len++] = buffer[i];
            }
        }
    }

    uint16_t assembled_len = manager->field_data_len;
    uint8_t parsed_ok = parse_field_payload(manager, field_id, manager->field_data, assembled_len);

    if (!parsed_ok) {
#ifdef debug
        if (!manager->fields) {
            CRSF_DBG("parse failed: fields not allocated id=%u len=%u\n", field_id, assembled_len);
        } else {
            CRSF_DBG("parse failed validation id=%u len=%u type=%u opts=%u\n",
                     field_id, assembled_len,
                     assembled_len > 1 ? (manager->field_data[1] & 0x7F) : 0,
                     manager->fields[field_id].option_count);
        }
#endif
        param_reset_chunk_state(manager);
        manager->chunk_need_request = 1;
        return 0;
    }

    param_reset_chunk_state(manager);
    manager->pending_field_id = PARAM_PENDING_NONE;

#ifdef debug
    crsf_dbg.parse_ok++;
    crsf_dbg.last_parse_field = field_id;
    crsf_dbg.last_parse_chunks = chunks_remain;
    if (field_id > 0 && field_id <= manager->fields_count) {
        strncpy(crsf_dbg.last_parse_name, manager->fields[field_id].name,
                sizeof(crsf_dbg.last_parse_name) - 1);
        crsf_dbg.last_parse_name[sizeof(crsf_dbg.last_parse_name) - 1] = 0;
    }
    CRSF_DBG("parsed id=%u type=%u name=%s (%u/%u)\n",
             field_id,
             manager->fields[field_id].type,
             manager->fields[field_id].name,
             manager->fields_loaded,
             manager->fields_count);
#endif

    if (manager->load_queue_len > 0) {
        manager->load_queue_len--;
    }
    if (manager->load_queue_len == 0) {
        manager->params_loaded = 1;
    }

    return 1;
}

dynamic_field_t *dynamic_get_field_by_id(dynamic_param_manager_t *manager, uint8_t field_id) {
    if (!manager || !manager->fields || field_id == 0 || field_id > manager->fields_count) {
        return NULL;
    }
    dynamic_field_t *field = &manager->fields[field_id];
    return field->loaded ? field : NULL;
}

dynamic_field_t *dynamic_find_field_by_name(dynamic_param_manager_t *manager, const char *name) {
    if (!manager || !name || !manager->fields) {
        return NULL;
    }
    for (uint16_t i = 1; i <= manager->fields_count; i++) {
        dynamic_field_t *field = &manager->fields[i];
        if (field->loaded && strncmp(field->name, name, 31) == 0) {
            return field;
        }
    }
    return NULL;
}

static uint8_t field_is_visible_menu_item(const dynamic_field_t *field) {
    if (!field || !field->loaded || field->hidden) {
        return 0;
    }
    if (field->type == CRSF_INFO) {
        return 0;
    }
    return 1;
}

uint8_t dynamic_get_visible_fields(dynamic_param_manager_t *manager, uint8_t folder_id,
                                   dynamic_field_t **out_fields, uint8_t max_count) {
    if (!manager || !manager->fields || !out_fields) {
        return 0;
    }

    uint8_t count = 0;
    for (uint16_t i = 1; i <= manager->fields_count && count < max_count; i++) {
        dynamic_field_t *field = &manager->fields[i];
        if (field->loaded && field->parent == folder_id && field_is_visible_menu_item(field)) {
            out_fields[count++] = field;
        }
    }
    return count;
}

dynamic_field_t *dynamic_get_field_in_folder(dynamic_param_manager_t *manager,
                                             uint8_t folder_id, uint8_t index) {
    dynamic_field_t *visible[32];
    uint8_t count = dynamic_get_visible_fields(manager, folder_id, visible, 32);
    if (index >= count) {
        return NULL;
    }
    return visible[index];
}

void dynamic_field_increment(dynamic_field_t *field, int8_t step) {
    if (!field || field->grey) {
        return;
    }

    if (field->type == CRSF_TEXT_SELECTION) {
        int32_t new_val = field->value + step;
        int32_t min_val = 0;
        int32_t max_val = field->option_count > 0 ? field->option_count - 1 : 0;
        if (new_val < min_val) {
            new_val = max_val;
        } else if (new_val > max_val) {
            new_val = min_val;
        }
        field->value = new_val;
        return;
    }

    int32_t inc = (field->step ? field->step : 1) * step;
    int32_t new_val = field->value + inc;
    if (field->max != field->min || field->max != 0) {
        if (new_val < field->min) {
            new_val = field->min;
        }
        if (new_val > field->max) {
            new_val = field->max;
        }
    }
    field->value = new_val;
}

void dynamic_field_display_value(dynamic_field_t *field, char *buffer, uint16_t buffer_size) {
    if (!field || !buffer || buffer_size < 2) {
        return;
    }
    buffer[0] = 0;

    switch (field->type) {
    case CRSF_UINT8:
    case CRSF_INT8:
    case CRSF_UINT16:
    case CRSF_INT16:
    case CRSF_UINT32:
    case CRSF_INT32:
        snprintf(buffer, buffer_size, "%ld%s", (long)field->value, field->unit);
        break;
    case CRSF_FLOAT: {
        int32_t divisor = 1;
        for (uint8_t i = 0; i < field->precision; i++) {
            divisor *= 10;
        }
        snprintf(buffer, buffer_size, field->format_str[0] ? field->format_str : "%.2f",
                 (double)field->value / (double)divisor);
        strncat(buffer, field->unit, buffer_size - strlen(buffer) - 1);
        break;
    }
    case CRSF_TEXT_SELECTION: {
        char option_label[40];
        if (field->value >= 0 && field->value < field->option_count) {
            option_label_at(field->options_blob, (uint8_t)field->value, option_label, sizeof(option_label));
            if (dynamic_field_compact_display(field)) {
                snprintf(buffer, buffer_size, "%s", option_label);
            } else {
                snprintf(buffer, buffer_size, "%s%s", option_label, field->unit);
            }
        } else {
            strncpy(buffer, "ERR", buffer_size - 1);
        }
        break;
    }
    case CRSF_STRING:
    case CRSF_INFO:
        strncpy(buffer, field->name, buffer_size - 1);
        break;
    case CRSF_FOLDER:
        strncpy(buffer, ">", buffer_size - 1);
        break;
    case CRSF_COMMAND:
        if (field->value > 0 && field->value < 4 && field->options_blob[0]) {
            snprintf(buffer, buffer_size, "%s", field->options_blob);
        } else {
            snprintf(buffer, buffer_size, "[%s]", field->name);
        }
        break;
    default:
        snprintf(buffer, buffer_size, "%ld", (long)field->value);
    }
    buffer[buffer_size - 1] = 0;
}

const char *dynamic_field_type_name(uint8_t type) {
    switch (type) {
    case CRSF_UINT8:
        return "UInt8";
    case CRSF_INT8:
        return "Int8";
    case CRSF_UINT16:
        return "UInt16";
    case CRSF_INT16:
        return "Int16";
    case CRSF_UINT32:
        return "UInt32";
    case CRSF_INT32:
        return "Int32";
    case CRSF_FLOAT:
        return "Float";
    case CRSF_TEXT_SELECTION:
        return "Selection";
    case CRSF_STRING:
        return "String";
    case CRSF_FOLDER:
        return "Folder";
    case CRSF_INFO:
        return "Info";
    case CRSF_COMMAND:
        return "Command";
    default:
        return "Unknown";
    }
}
