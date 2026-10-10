
#include "rc_limits.h"
 // Basic setup
#define CRSF_MAX_CHANNEL 16
//#define CRSF_FRAME_SIZE_MAX 64
//#define SERIAL_BAUDRATE 420000 //low baud for Arduino Nano , the TX module will auto detect baud. max packet rate is 250Hz.

 // Device address & type
#define RADIO_ADDRESS                  0xEA
#define ADDR_MODULE                    0xEE  //  Crossfire transmitter
#define TYPE_CHANNELS                  0x16

// Define RC input limite
//#define RC_CHANNEL_MIN 172
//#define RC_CHANNEL_MID 991
//#define RC_CHANNEL_MAX 1811

//Define AUX channel input limite
#define CRSF_DIGITAL_CHANNEL_MIN 172
#define CRSF_DIGITAL_CHANNEL_MAX 1811

// internal crsf variables
#define CRSF_TIME_NEEDED_PER_FRAME_US   1100 // 700 ms + 400 ms for potential ad-hoc request
//#define CRSF_TIME_BETWEEN_FRAMES_US     4000 // 4 ms 250Hz
#define CRSF_PAYLOAD_OFFSET offsetof(crsfFrameDef_t, type)
#define CRSF_MSP_RX_BUF_SIZE 128
#define CRSF_MSP_TX_BUF_SIZE 128
#define CRSF_PAYLOAD_SIZE_MAX_RC   60
#define CRSF_PACKET_LENGTH 22
#define CRSF_PACKET_SIZE  26
//#define CRSF_FRAME_LENGTH 24;   // length of type + payload + crc


#define CRSF_CMD_PACKET_SIZE  8

// ELRS command
#define ELRS_ADDRESS                   0xEE
#define ELRS_BIND_COMMAND              0xFF
#define ELRS_WIFI_COMMAND              0xFE
#define ELRS_PKT_RATE_COMMAND          1
#define ELRS_TLM_RATIO_COMMAND         2
#define ELRS_POWER_COMMAND             3
#define TYPE_SETTINGS_WRITE            0x2D
#define ADDR_RADIO                     0xEA  //  Radio Transmitter

//#include <Arduino.h>
#include "crsf_protocol.h"
#include "crsf_dynamic.h"
//#pragma once

#define CRSF_MAX_PARAMS 55 // one extra required, max observed is 47 in Diversity Nano RX
#define CRSF_MAX_DEVICES 4
#define CRSF_MAX_NAME_LEN 16
#define CRSF_MAX_STRING_BYTES 2500 // max observed is 2010 in Nano RX
#define CRSF_STRING_BYTES_AVAIL(current) (CRSF_MAX_STRING_BYTES - ((char *)(current)-mp->strings))

typedef struct
{
    uint8_t address;
    uint8_t number_of_params;
    uint8_t params_version;
    uint32_t serial_number;
    uint32_t hardware_id;
    uint32_t firmware_id;
    char name[CRSF_MAX_NAME_LEN];
} crsf_device_t;

typedef struct
{
    uint8_t id;
    uint8_t type;
    char name[32];
    int32_t value;
    int32_t min;
    int32_t max;
    char unit[16];
    uint8_t num_options;
    char options[8][32];
    uint8_t hidden;
    uint8_t parent;
} crsf_parameter_t;

typedef struct
{
    crsf_parameter_t params[CRSF_MAX_PARAMS];
    uint8_t param_count;
    uint8_t params_loaded;
    uint8_t param_request_idx;
    uint32_t param_request_time;
} crsf_param_list_t;

// Dynamic menu structures for UI
typedef struct {
    uint8_t param_id;
    char display_name[32];
    crsf_parameter_t *param_ptr;
    uint8_t folder_id;
    uint8_t is_visible;
} handset_menu_item_t;

typedef struct {
    uint8_t folder_id;
    char folder_name[32];
    uint8_t item_count;
    uint8_t first_item_idx;
} handset_menu_folder_t;

typedef struct {
    handset_menu_item_t items[CRSF_MAX_PARAMS];
    uint8_t item_count;
    handset_menu_folder_t folders[16];
    uint8_t folder_count;
    uint8_t current_folder_id;
} handset_menu_state_t;

typedef enum
{
    MODULE_UNKNOWN,
    MODULE_ELRS,
    MODULE_OTHER,
} module_type_t;

uint8_t protocol_module_is_elrs();

#define CRSF_MAX_CHANNEL 16

//extern int rcChannels[CRSF_MAX_CHANNEL];

#define CRSF_MAX_CHUNK_SIZE 58 // 64 - header - type - destination - origin
#define CRSF_MAX_CHUNKS 5      // not in specification. Max observed is 3 for Nano RX

extern module_type_t module_type;
extern uint8_t device_idx; // current device index
extern crsf_param_list_t tx_module_params;
extern handset_menu_state_t handset_menu;
extern dynamic_param_manager_t param_manager;

extern char recv_param_buffer[];
extern char *recv_param_ptr;

#define CRSF_PARAM_BOOT_DELAY_MS 3000

extern uint32_t crsf_param_boot_ready_at;

static inline uint8_t crsf_param_load_active(void) {
  return param_manager.fields_count > 0 && !param_manager.params_loaded;
}

uint8_t crsf_param_discovery_allowed(void);

void CRSF_discover_parameters(uint8_t device_id);
void crsf_rc_timer_pause(void);
void crsf_rc_timer_resume(void);
void CRSF_send_id(uint8_t modelId);
void CRSF_get_elrs_info(uint8_t target);
void parse_parameter_entry(uint8_t *buffer, uint16_t buffer_len);
void send_parameter_value(uint8_t param_id, int32_t value, uint8_t size);
void build_handset_menus_from_params(void);
uint8_t get_visible_items_in_folder(uint8_t folder_id, handset_menu_item_t **out_items);
crsf_parameter_t* get_param_at_menu_position(uint8_t menu_idx);

// Basic setup
#ifdef DEBUG
#define SERIAL_BAUDRATE 115200 // low baud for Arduino Nano , the TX module will auto detect baud. max packet rate is 250Hz.
#else
#define SERIAL_BAUDRATE 400000 // testing 3750000//1870000
#endif
// Device address & type
#define RADIO_ADDRESS 0xEA
#define ADDR_MODULE 0xEE //  Crossfire transmitter
#define TYPE_CHANNELS 0x16

// Define RC input limite
#define RC_CHANNEL_MIN 172
#define RC_CHANNEL_MID 991
#define RC_CHANNEL_MAX 1811

// Define AUX channel input limite
#define CRSF_DIGITAL_CHANNEL_MIN 172
#define CRSF_DIGITAL_CHANNEL_MAX 1811

#define CRSF_FRAME_PERIOD_MIN 850   // 1000Hz 1ms, but allow shorter for offset cancellation
#define CRSF_FRAME_PERIOD_MAX 50000 // 25Hz  40ms, but allow longer for offset cancellation
// internal crsf variables
#define CRSF_TIME_NEEDED_PER_FRAME_US 1100 // 700 ms + 400 ms for potential ad-hoc request
#define CRSF_TIME_BETWEEN_FRAMES_US 5000   // 4 ms 250Hz
#define CRSF_PAYLOAD_OFFSET offsetof(crsfFrameDef_t, type)
#define CRSF_MSP_RX_BUF_SIZE 128
#define CRSF_MSP_TX_BUF_SIZE 128
#define CRSF_PACKET_LENGTH 22
#define CRSF_PACKET_SIZE 26
#define CRSF_FRAME_LENGTH 24 // length of type + payload + crc
#define CRSF_CMD_PACKET_SIZE 8
#define LinkStatisticsFrameLength 10 //

// ELRS command
#define ELRS_ADDRESS 0xEE
#define ELRS_RX_ADDRESS 0xEC
#define ELRS_BIND_COMMAND 0xFF
#define ELRS_WIFI_COMMAND 0xFE
#define ELRS_PKT_RATE_COMMAND 1
#define ELRS_TLM_RATIO_COMMAND 2
#define ELRS_POWER_COMMAND 3

#define ADDR_RADIO 0xEA //  Radio Transmitter

// Frame Type
#define TYPE_GPS 0x02
#define TYPE_VARIO 0x07
#define TYPE_BATTERY 0x08
#define TYPE_HEARTBEAT 0x0b
#define TYPE_VTX 0x0F
#define TYPE_VTX_TELEM 0x10
#define TYPE_LINK 0x14
#define TYPE_CHANNELS 0x16
#define TYPE_RX_ID 0x1C
#define TYPE_TX_ID 0x1D
#define TYPE_ATTITUDE 0x1E
#define TYPE_FLIGHT_MODE 0x21
#define TYPE_PING_DEVICES 0x28
#define TYPE_DEVICE_INFO 0x29
#define TYPE_REQUEST_SETTINGS 0x2A
#define TYPE_SETTINGS_ENTRY 0x2B
#define TYPE_SETTINGS_READ 0x2C
#define TYPE_SETTINGS_WRITE 0x2D
#define TYPE_ELRS_INFO 0x2E
#define TYPE_COMMAND_ID 0x32
#define TYPE_RADIO_ID 0x3A

// Frame Subtype
#define UART_SYNC 0xC8
#define CRSF_SUBCOMMAND 0x10
#define COMMAND_MODEL_SELECT_ID 0x05

#define TELEMETRY_RX_PACKET_SIZE 64

#define CRSF_CRC_POLY 0xd5

#define CRSF_MAX_PACKET_LEN 64

#define SEND_MSG_BUF_SIZE 64 // don't send more than one chunk
#define ADDR_BROADCAST 0x00  //  Broadcast address

#define MODULE_IS_ELRS (module_type == MODULE_ELRS)
#define MODULE_IS_UNKNOWN (module_type == MODULE_UNKNOWN)

typedef struct
{
    uint8_t update;
    uint8_t bad_pkts;
    uint16_t good_pkts;
    uint8_t flags;
    char flag_info[CRSF_MAX_NAME_LEN];
} elrs_info_t;

extern elrs_info_t elrs_info;

/// UART Handling ///
static volatile uint8_t SerialInPacketLen; // length of the CRSF packet as measured
static volatile uint8_t SerialInPacketPtr; // index where we are reading/writing
static volatile bool CRSFframeActive;      // = false; //since we get a copy of the serial data use this flag to know when to ignore it


