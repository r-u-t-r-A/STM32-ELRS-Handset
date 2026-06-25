/*
 * Dynamic Parameter Loading for DIY ExpressLRS Handset
 * Adapted from elrsV3.lua - runtime parameter discovery via CRSF 0x2B/0x2C
 */

#ifndef CRSF_DYNAMIC_H
#define CRSF_DYNAMIC_H

#include <stdint.h>
#include <string.h>

#define DYNAMIC_ROOT_FOLDER 0
#define DYNAMIC_OPTIONS_BLOB_SIZE 160
#define DYNAMIC_FIELD_DATA_SIZE 320
#define PARAM_PENDING_NONE 0xFF

typedef struct {
    uint8_t id;
    uint8_t type;
    char name[32];
    int32_t value;
    int32_t min;
    int32_t max;
    uint8_t parent;
    char unit[16];
    uint8_t hidden;
    uint8_t grey;
    int32_t step;
    int8_t size;  // positive = unsigned bytes, negative = signed bytes

    char options_blob[DYNAMIC_OPTIONS_BLOB_SIZE];
    uint8_t option_count;

    uint8_t maxlen;
    uint8_t precision;
    char format_str[16];
    uint8_t loaded;
} dynamic_field_t;

typedef struct {
    dynamic_field_t *fields;
    uint16_t max_fields;
    uint16_t fields_count;   // reported by device (param slots 1..fields_count)
    uint16_t fields_alloc_count;
    uint16_t fields_loaded;  // fully parsed parameters

    uint8_t params_loaded;
    uint32_t last_load_time;

    uint8_t load_queue[64];
    uint8_t load_queue_len;
    uint8_t pending_field_id;
    uint32_t pending_since;
    uint8_t field_chunk;
    uint8_t chunk_need_request;
    int8_t expect_chunks_remain;
    uint8_t field_data[DYNAMIC_FIELD_DATA_SIZE];
    uint16_t field_data_len;
} dynamic_param_manager_t;

typedef struct {
    uint8_t current_folder;
    uint8_t line_index;
    uint8_t page_offset;
    dynamic_field_t *selected_field;
    uint8_t edit_mode;
    int32_t edit_value;
} dynamic_menu_state_t;

void dynamic_param_init(dynamic_param_manager_t *manager, uint16_t max_fields);
void dynamic_param_cleanup(dynamic_param_manager_t *manager);
void dynamic_param_reset_for_reload(dynamic_param_manager_t *manager);

void dynamic_param_on_device_info(dynamic_param_manager_t *manager, uint8_t param_count);
uint8_t dynamic_param_tick(dynamic_param_manager_t *manager, uint8_t device_id,
                           void (*read_param_fn)(uint8_t field_id, uint8_t chunk, uint8_t device_id));

uint8_t dynamic_param_parse_message(dynamic_param_manager_t *manager,
                                    const uint8_t *buffer, uint16_t buffer_len,
                                    uint8_t expected_device_id);

dynamic_field_t *dynamic_get_field_by_id(dynamic_param_manager_t *manager, uint8_t field_id);
dynamic_field_t *dynamic_find_field_by_name(dynamic_param_manager_t *manager, const char *name);

uint8_t dynamic_get_visible_fields(dynamic_param_manager_t *manager, uint8_t folder_id,
                                   dynamic_field_t **out_fields, uint8_t max_count);
dynamic_field_t *dynamic_get_field_in_folder(dynamic_param_manager_t *manager,
                                             uint8_t folder_id, uint8_t index);

void dynamic_field_increment(dynamic_field_t *field, int8_t step);
uint8_t dynamic_field_compact_display(const dynamic_field_t *field);
void dynamic_field_display_value(dynamic_field_t *field, char *buffer, uint16_t buffer_size);
const char *dynamic_field_type_name(uint8_t type);

void dynamic_param_send_value(uint8_t device_id, dynamic_field_t *field);

/* ELRS command status bytes in CRSF 0x2D (see elrsV3.lua fieldCommandSave) */
#define CRSF_CMD_STATUS_START    1
#define CRSF_CMD_STATUS_CONFIRM  4
#define CRSF_CMD_STATUS_CANCEL   5
#define CRSF_CMD_STATUS_QUERY    6

uint8_t dynamic_param_command_tx_byte(const dynamic_field_t *field);
void dynamic_param_send_command(uint8_t device_id, uint8_t field_id, uint8_t status_byte);

void dynamic_command_popup_start(uint8_t field_id, uint8_t timeout_units);
void dynamic_command_popup_clear(void);
uint8_t dynamic_command_popup_active(void);
uint8_t dynamic_command_popup_field_id(void);
uint8_t dynamic_command_popup_tick(unsigned long now, uint8_t device_id);
const char *dynamic_command_popup_banner(void);
uint8_t dynamic_command_popup_banner_take_edge(void);
void dynamic_command_popup_show_notice(const char *text, uint32_t duration_ms);

extern uint8_t dynamic_menu_dirty;
uint8_t dynamic_menu_take_dirty(void);

#endif // CRSF_DYNAMIC_H
