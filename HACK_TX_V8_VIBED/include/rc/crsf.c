/*
 =======================================================================================================
 * CRSF protocol - is the files?
 *
 * CRSF protocol uses a single wire half duplex uart connection.
 * The master sends one frame every 4ms and the slave replies between two frames from the master.
 *
 * 420000 baud
 * not inverted
 * 8 Bit
 * 1 Stop bit
 * Big endian
 * ELRS uses crossfire protocol at many different baud rates supported by EdgeTX i.e. 115k, 400k, 921k, 1.87M, 3.75M
 * 115000 bit/s = 14400 byte/s 
 * 420000 bit/s = 46667 byte/s (including stop bit) = 21.43us per byte
 * Max frame size is 64 bytes
 * A 64 byte frame plus 1 sync byte can be transmitted in 1393 microseconds.
 *
 * CRSF_TIME_NEEDED_PER_FRAME_US is set conservatively at 1500 microseconds
 *
 * Every frame has the structure:
 * <Device address><Frame length><Type><Payload><CRC>
 *
 * Device address: (uint8_t)
 * Frame length:   length in  bytes including Type (uint8_t)
 * Type:           (uint8_t)
 * CRC:            (uint8_t)
 *
 */
#include "crsf.h"
#include <stdint.h>
#include "crsf_dynamic.h"
#include "crsf_debug.h"

void serialtelemetryevent_2400(void);
uint8_t getCrossfireTelemetryValue(uint8_t index, int32_t *value, uint8_t len);
void CRSF_serial_rcv(uint8_t *buffer, uint8_t num_bytes);
void CRSF_broadcast_ping(void);

// from https://github.com/DeviationTX/deviation/pull/1009/ ELRS menu implement in deviation TX

dynamic_param_manager_t param_manager;

uint32_t crsf_param_boot_ready_at = 0;

uint8_t crsf_param_discovery_allowed(void) {
  return (int32_t)(millis() - crsf_param_boot_ready_at) >= 0;
}

//static uint8_t currentPower[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};//  list =  {'10 mW', '25 mW', '50 mW', '100 mW', '250 mW', '500 mW', '1000 mW', '2000 mW'},
  //                                            0        1         2        3        4   
//static uint8_t currentPktRate[4] = {0x06, 0x05, 0x04, 0x02}; //  '25Hz(-123dbm)', '50Hz(-120dbm)', '100Hz(-117dbm)', '200Hz(-112dbm)'
  //                                           0              1               3                 5      
uint32_t crsfTime = 0;
uint32_t lastCrsfTime = 0;
uint32_t updateInterval = CRSF_TIME_BETWEEN_FRAMES_US;

int32_t correction = 0;

crsf_device_t crsf_devices[CRSF_MAX_DEVICES];

crsf_param_list_t tx_module_params = {0};

elrs_info_t local_info;

elrs_info_t elrs_info;

module_type_t module_type;

handset_menu_state_t handset_menu = {0};

char recv_param_buffer[CRSF_MAX_CHUNKS * CRSF_MAX_CHUNK_SIZE];
char *recv_param_ptr;

uint8_t device_idx = 0; // current device index

uint8_t SerialInBuffer[CRSF_MAX_PACKET_LEN + 2];

crsfPayloadLinkstatistics_s LinkStatistics;

volatile crsf_sensor_battery_s batteryVoltage;


uint8_t crsfPacket[CRSF_PACKET_SIZE];
int rcChannels[CRSF_MAX_CHANNEL];
 // crc implementation from CRSF protocol document rev7
static uint8_t crc8tab[256] = {
    0x00, 0xD5, 0x7F, 0xAA, 0xFE, 0x2B, 0x81, 0x54, 0x29, 0xFC, 0x56, 0x83, 0xD7, 0x02, 0xA8, 0x7D,
    0x52, 0x87, 0x2D, 0xF8, 0xAC, 0x79, 0xD3, 0x06, 0x7B, 0xAE, 0x04, 0xD1, 0x85, 0x50, 0xFA, 0x2F,
    0xA4, 0x71, 0xDB, 0x0E, 0x5A, 0x8F, 0x25, 0xF0, 0x8D, 0x58, 0xF2, 0x27, 0x73, 0xA6, 0x0C, 0xD9,
    0xF6, 0x23, 0x89, 0x5C, 0x08, 0xDD, 0x77, 0xA2, 0xDF, 0x0A, 0xA0, 0x75, 0x21, 0xF4, 0x5E, 0x8B,
    0x9D, 0x48, 0xE2, 0x37, 0x63, 0xB6, 0x1C, 0xC9, 0xB4, 0x61, 0xCB, 0x1E, 0x4A, 0x9F, 0x35, 0xE0,
    0xCF, 0x1A, 0xB0, 0x65, 0x31, 0xE4, 0x4E, 0x9B, 0xE6, 0x33, 0x99, 0x4C, 0x18, 0xCD, 0x67, 0xB2,
    0x39, 0xEC, 0x46, 0x93, 0xC7, 0x12, 0xB8, 0x6D, 0x10, 0xC5, 0x6F, 0xBA, 0xEE, 0x3B, 0x91, 0x44,
    0x6B, 0xBE, 0x14, 0xC1, 0x95, 0x40, 0xEA, 0x3F, 0x42, 0x97, 0x3D, 0xE8, 0xBC, 0x69, 0xC3, 0x16,
    0xEF, 0x3A, 0x90, 0x45, 0x11, 0xC4, 0x6E, 0xBB, 0xC6, 0x13, 0xB9, 0x6C, 0x38, 0xED, 0x47, 0x92,
    0xBD, 0x68, 0xC2, 0x17, 0x43, 0x96, 0x3C, 0xE9, 0x94, 0x41, 0xEB, 0x3E, 0x6A, 0xBF, 0x15, 0xC0,
    0x4B, 0x9E, 0x34, 0xE1, 0xB5, 0x60, 0xCA, 0x1F, 0x62, 0xB7, 0x1D, 0xC8, 0x9C, 0x49, 0xE3, 0x36,
    0x19, 0xCC, 0x66, 0xB3, 0xE7, 0x32, 0x98, 0x4D, 0x30, 0xE5, 0x4F, 0x9A, 0xCE, 0x1B, 0xB1, 0x64,
    0x72, 0xA7, 0x0D, 0xD8, 0x8C, 0x59, 0xF3, 0x26, 0x5B, 0x8E, 0x24, 0xF1, 0xA5, 0x70, 0xDA, 0x0F,
    0x20, 0xF5, 0x5F, 0x8A, 0xDE, 0x0B, 0xA1, 0x74, 0x09, 0xDC, 0x76, 0xA3, 0xF7, 0x22, 0x88, 0x5D,
    0xD6, 0x03, 0xA9, 0x7C, 0x28, 0xFD, 0x57, 0x82, 0xFF, 0x2A, 0x80, 0x55, 0x01, 0xD4, 0x7E, 0xAB,
    0x84, 0x51, 0xFB, 0x2E, 0x7A, 0xAF, 0x05, 0xD0, 0xAD, 0x78, 0xD2, 0x07, 0x53, 0x86, 0x2C, 0xF9};

// CRC8 implementation with polynom = 0xBA
static const uint8_t crc8tab_BA[256] = {
    0x00, 0xBA, 0xCE, 0x74, 0x26, 0x9C, 0xE8, 0x52, 0x4C, 0xF6, 0x82, 0x38, 0x6A, 0xD0, 0xA4, 0x1E,
    0x98, 0x22, 0x56, 0xEC, 0xBE, 0x04, 0x70, 0xCA, 0xD4, 0x6E, 0x1A, 0xA0, 0xF2, 0x48, 0x3C, 0x86,
    0x8A, 0x30, 0x44, 0xFE, 0xAC, 0x16, 0x62, 0xD8, 0xC6, 0x7C, 0x08, 0xB2, 0xE0, 0x5A, 0x2E, 0x94,
    0x12, 0xA8, 0xDC, 0x66, 0x34, 0x8E, 0xFA, 0x40, 0x5E, 0xE4, 0x90, 0x2A, 0x78, 0xC2, 0xB6, 0x0C,
    0xAE, 0x14, 0x60, 0xDA, 0x88, 0x32, 0x46, 0xFC, 0xE2, 0x58, 0x2C, 0x96, 0xC4, 0x7E, 0x0A, 0xB0,
    0x36, 0x8C, 0xF8, 0x42, 0x10, 0xAA, 0xDE, 0x64, 0x7A, 0xC0, 0xB4, 0x0E, 0x5C, 0xE6, 0x92, 0x28,
    0x24, 0x9E, 0xEA, 0x50, 0x02, 0xB8, 0xCC, 0x76, 0x68, 0xD2, 0xA6, 0x1C, 0x4E, 0xF4, 0x80, 0x3A,
    0xBC, 0x06, 0x72, 0xC8, 0x9A, 0x20, 0x54, 0xEE, 0xF0, 0x4A, 0x3E, 0x84, 0xD6, 0x6C, 0x18, 0xA2,
    0xE6, 0x5C, 0x28, 0x92, 0xC0, 0x7A, 0x0E, 0xB4, 0xAA, 0x10, 0x64, 0xDE, 0x8C, 0x36, 0x42, 0xF8,
    0x7E, 0xC4, 0xB0, 0x0A, 0x58, 0xE2, 0x96, 0x2C, 0x32, 0x88, 0xFC, 0x46, 0x14, 0xAE, 0xDA, 0x60,
    0x6C, 0xD6, 0xA2, 0x18, 0x4A, 0xF0, 0x84, 0x3E, 0x20, 0x9A, 0xEE, 0x54, 0x06, 0xBC, 0xC8, 0x72,
    0xF4, 0x4E, 0x3A, 0x80, 0xD2, 0x68, 0x1C, 0xA6, 0xB8, 0x02, 0x76, 0xCC, 0x9E, 0x24, 0x50, 0xEA,
    0x48, 0xF2, 0x86, 0x3C, 0x6E, 0xD4, 0xA0, 0x1A, 0x04, 0xBE, 0xCA, 0x70, 0x22, 0x98, 0xEC, 0x56,
    0xD0, 0x6A, 0x1E, 0xA4, 0xF6, 0x4C, 0x38, 0x82, 0x9C, 0x26, 0x52, 0xE8, 0xBA, 0x00, 0x74, 0xCE,
    0xC2, 0x78, 0x0C, 0xB6, 0xE4, 0x5E, 0x2A, 0x90, 0x8E, 0x34, 0x40, 0xFA, 0xA8, 0x12, 0x66, 0xDC,
    0x5A, 0xE0, 0x94, 0x2E, 0x7C, 0xC6, 0xB2, 0x08, 0x16, 0xAC, 0xD8, 0x62, 0x30, 0x8A, 0xFE, 0x44};

static uint8_t crsf_crc(const uint8_t crctab[], const uint8_t *ptr, uint8_t len)  {

  uint8_t crc = 0;
  for (uint8_t i = 0; i < len; i++)
  {
    crc = crctab[crc ^ *ptr++];
  }
  return crc;
}

uint8_t crsf_crc8(const uint8_t *ptr, uint8_t len)  {
  return crsf_crc(crc8tab, ptr, len);
}

uint8_t crsf_crc8_BA(const uint8_t *ptr, uint8_t len) {
  return crsf_crc(crc8tab_BA, ptr, len);
}

//prepare data packet
void crsfPreparePacket(uint8_t packet[], int channels[]){

    static int output[CRSF_MAX_CHANNEL] = {0};
    const uint8_t crc = crsf_crc8(&packet[2], CRSF_PACKET_SIZE-3);
    /*
     * Map 1000-2000 with middle at 1500 chanel values to
     * 172-1811 with middle at 998 CRSF protocol requires
     */
    for (uint8_t i = 0; i < CRSF_MAX_CHANNEL; i++) {
        output[i] = map(channels[i], RC_CHANNEL_MIN, RC_CHANNEL_MAX, CRSF_DIGITAL_CHANNEL_MIN, CRSF_DIGITAL_CHANNEL_MAX);
    }    


    // packet[0] = UART_SYNC; //Header
    packet[0] = ADDR_MODULE; //Header
    packet[1] = 24;   // length of type (24) + payload + crc
    packet[2] = TYPE_CHANNELS;
    packet[3] = (uint8_t) (channels[0] & 0x07FF);
    packet[4] = (uint8_t) ((channels[0] & 0x07FF)>>8 | (channels[1] & 0x07FF)<<3);
    packet[5] = (uint8_t) ((channels[1] & 0x07FF)>>5 | (channels[2] & 0x07FF)<<6);
    packet[6] = (uint8_t) ((channels[2] & 0x07FF)>>2);
    packet[7] = (uint8_t) ((channels[2] & 0x07FF)>>10 | (channels[3] & 0x07FF)<<1);
    packet[8] = (uint8_t) ((channels[3] & 0x07FF)>>7 | (channels[4] & 0x07FF)<<4);
    packet[9] = (uint8_t) ((channels[4] & 0x07FF)>>4 | (channels[5] & 0x07FF)<<7);
    packet[10] = (uint8_t) ((channels[5] & 0x07FF)>>1);
    packet[11] = (uint8_t) ((channels[5] & 0x07FF)>>9 | (channels[6] & 0x07FF)<<2);
    packet[12] = (uint8_t) ((channels[6] & 0x07FF)>>6 | (channels[7] & 0x07FF)<<5);
    packet[13] = (uint8_t) ((channels[7] & 0x07FF)>>3);
    packet[14] = (uint8_t) ((channels[8] & 0x07FF));
    packet[15] = (uint8_t) ((channels[8] & 0x07FF)>>8 | (channels[9] & 0x07FF)<<3);
    packet[16] = (uint8_t) ((channels[9] & 0x07FF)>>5 | (channels[10] & 0x07FF)<<6);  
    packet[17] = (uint8_t) ((channels[10] & 0x07FF)>>2);
    packet[18] = (uint8_t) ((channels[10] & 0x07FF)>>10 | (channels[11] & 0x07FF)<<1);
    packet[19] = (uint8_t) ((channels[11] & 0x07FF)>>7 | (channels[12] & 0x07FF)<<4);
    packet[20] = (uint8_t) ((channels[12] & 0x07FF)>>4  | (channels[13] & 0x07FF)<<7);
    packet[21] = (uint8_t) ((channels[13] & 0x07FF)>>1);
    packet[22] = (uint8_t) ((channels[13] & 0x07FF)>>9  | (channels[14] & 0x07FF)<<2);
    packet[23] = (uint8_t) ((channels[14] & 0x07FF)>>6  | (channels[15] & 0x07FF)<<5);
    packet[24] = (uint8_t) ((channels[15] & 0x07FF)>>3);
    
    packet[25] = crsf_crc8(&packet[2], CRSF_PACKET_SIZE-3); //CRC

}

//prepare elrs setup packet (power, packet rate...)
uint8_t crsfCmdPacket[CRSF_CMD_PACKET_SIZE];
void buildElrsPacket(uint8_t packetCmd[],uint8_t command, uint8_t value)
{
  packetCmd[0] = ADDR_MODULE;
  packetCmd[1] = 6; // length of Command (4) + payload + crc
  packetCmd[2] = TYPE_SETTINGS_WRITE;
  packetCmd[3] = ELRS_ADDRESS;
  packetCmd[4] = ADDR_RADIO;
  packetCmd[5] = command;
  packetCmd[6] = value;
  packetCmd[7] = crsf_crc8(&packetCmd[2], packetCmd[1]-1);

}


void protocol_module_type(module_type_t type)
{
  module_type = type;
};
uint8_t protocol_module_is_elrs() { return MODULE_IS_ELRS; }

uint32_t get_update_interval()
{
  if (correction == 0)
    return updateInterval;

  uint32_t update = updateInterval + correction;
  update = constrain(update, CRSF_FRAME_PERIOD_MIN, CRSF_FRAME_PERIOD_MAX);
  correction -= update - updateInterval;
  return update;
}


static void crsf_rx_reset(void)
{
  CRSFframeActive = false;
  SerialInPacketPtr = 0;
  SerialInPacketLen = 0;
}

static uint8_t crsf_is_valid_sync_byte(uint8_t inChar);

static void crsf_rx_flush(void)
{
  while (ELRS_Serial_2400.available()) {
    ELRS_Serial_2400.read();
  }
  crsf_rx_reset();
}

static uint8_t crsf_0x2b_response_valid(void)
{
  return SerialInBuffer[2] == CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY
      && SerialInBuffer[3] == CRSF_ADDRESS_RADIO_TRANSMITTER
      && SerialInBuffer[4] == ADDR_MODULE;
}

static uint8_t crsf_crc_looks_like_sync(uint8_t b)
{
  return b == CRSF_SYNC_BYTE
      || b == CRSF_ADDRESS_RADIO_TRANSMITTER
      || b == CRSF_ADDRESS_ELRS_LUA;
}

static uint8_t crsf_pending_field_matches_0x2b(void)
{
  const uint8_t field_id = SerialInBuffer[5];
  return param_manager.pending_field_id == PARAM_PENDING_NONE
      || field_id == param_manager.pending_field_id;
}

static uint8_t crsf_should_recover_truncated_0x2b(uint8_t calculated_crc, uint8_t received_crc)
{
  if (!crsf_0x2b_response_valid() || !crsf_crc_looks_like_sync(received_crc)) {
    return 0;
  }
  if (calculated_crc == received_crc) {
    return 0;
  }
  return crsf_param_load_active() || crsf_pending_field_matches_0x2b();
}

static void crsf_process_serial_frame(void)
{
  const uint8_t crc_len = (uint8_t)(SerialInPacketLen - 1);
  uint8_t calculated_crc = crsf_crc8(SerialInBuffer + 2, crc_len);
  uint8_t received_crc = SerialInBuffer[SerialInPacketPtr - 1];
  uint8_t crc_ok = (calculated_crc == received_crc);

  if (!crc_ok && crsf_should_recover_truncated_0x2b(calculated_crc, received_crc)) {
    if (ELRS_Serial_2400.available()) {
      const uint8_t extra = (uint8_t)ELRS_Serial_2400.read();
      if (extra == calculated_crc) {
        SerialInBuffer[SerialInPacketPtr - 1] = extra;
        received_crc = extra;
      }
    }
    crc_ok = 1;
#ifdef debug
    crsf_dbg.crc_recover_0x2b++;
    CRSF_DBG("Recovered truncated 0x2B id=%u chunks=%u calc=0x%02X got=0x%02X\n",
             SerialInBuffer[5], SerialInBuffer[6], calculated_crc, received_crc);
#endif
  }

  if (crc_ok)
  {
#ifdef debug
    crsf_dbg.crc_ok++;
#endif
    int32_t value;
    uint8_t id = SerialInBuffer[2];
    CRSF_serial_rcv(SerialInBuffer + 2, SerialInBuffer[1] - 1);

    if (id == CRSF_FRAMETYPE_BATTERY_SENSOR)
    {
      if (getCrossfireTelemetryValue(3, &value, 2))
      {
        batteryVoltage.voltage = value;
      }
    }
    if (id == CRSF_FRAMETYPE_RADIO_ID)
    {
      if (SerialInBuffer[3] == CRSF_ADDRESS_RADIO_TRANSMITTER
          && SerialInBuffer[5] == CRSF_FRAMETYPE_OPENTX_SYNC
      )
      {
        if (getCrossfireTelemetryValue(6, (int32_t *)&updateInterval, 4) &&
            getCrossfireTelemetryValue(10, (int32_t *)&correction, 4))
        {
          updateInterval /= 10;
          correction /= 10;
          if (correction >= 0)
            correction %= updateInterval;
          else
            correction = -((-correction) % updateInterval);
        }
      }
      if (MODULE_IS_UNKNOWN) {
        CRSF_broadcast_ping();
      }
    }

    if (id == CRSF_FRAMETYPE_LINK_STATISTICS)
    {
      if (getCrossfireTelemetryValue(2 + TELEM_CRSF_RX_RSSI1, &value, 1))
      {
        LinkStatistics.uplink_RSSI_1 = value;
      }
      if (getCrossfireTelemetryValue(2 + TELEM_CRSF_RX_RSSI2, &value, 1))
      {
        LinkStatistics.uplink_RSSI_2 = value;
      }
      if (getCrossfireTelemetryValue(2 + TELEM_CRSF_RX_QUALITY, &value, 1))
      {
        LinkStatistics.uplink_Link_quality = value;
      }
      if (getCrossfireTelemetryValue(2 + TELEM_CRSF_RF_MODE, &value, 1))
      {
        LinkStatistics.rf_Mode = value;
      }
      if (getCrossfireTelemetryValue(2 + TELEM_CRSF_TX_POWER, &value, 1))
      {
        static const int32_t power_values[] = {0, 10, 25, 100, 500, 1000, 2000, 250, 50};
        value = power_values[value];
        LinkStatistics.uplink_TX_Power = value;
      }
      if (getCrossfireTelemetryValue(2 + TELEM_CRSF_TX_RSSI, &value, 1))
      {
        LinkStatistics.downlink_RSSI = value;
      }
      if (getCrossfireTelemetryValue(2 + TELEM_CRSF_TX_QUALITY, &value, 1))
      {
        LinkStatistics.downlink_Link_quality = value;
      }
    }
  }
  else
  {
#ifdef debug
    crsf_dbg.crc_fail++;
    if (SerialInBuffer[2] == CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY) {
      CRSF_DBG("CRC fail on 0x2B frame len=%u calc=0x%02X got=0x%02X\n",
               SerialInPacketLen, calculated_crc, received_crc);
      crsf_dbg_hex("  raw", SerialInBuffer, SerialInPacketPtr, 64);
    }
#endif
  }
}

void sync_crsf(int32_t add_delay)
{
  crsfTime = micros();                        // set current micros
  int32_t offset = (crsfTime - lastCrsfTime); // get dif between pckt send
  uint32_t updated_interval = get_update_interval();
// debug timing

  // if (add_delay>0)
  // //dbout.printf("delay:%u:%u\n",add_delay,offset);
  crsfTime += ((updated_interval + add_delay) - offset); // set current micros
  lastCrsfTime = crsfTime;                               // set time that we send last packet
}

void CRSF_write(uint8_t crsfPacket[], uint8_t size, int32_t add_delay)  {
  //duplex_set_TX();


    ELRS_Serial_2400.write(crsfPacket, size);
    ELRS_Serial_2400.flush();
  
  //ELRS_Serial.write(crsfPacket, size);
  //ELRS_Serial.flush();
  // if (add_delay>0)
  // //dbout.printf("del:%u\n",add_delay);

  // set last time packet send
  sync_crsf(add_delay);
}

void CRSF_broadcast_ping()  {
  uint8_t packetCmd[6];

  packetCmd[0] = ADDR_MODULE;
  packetCmd[1] = 4; // length of Command (4) + payload + crc
  packetCmd[2] = TYPE_PING_DEVICES;
  packetCmd[3] = ADDR_BROADCAST;
  packetCmd[4] = ADDR_RADIO;
  packetCmd[5] = crsf_crc8(&packetCmd[2], packetCmd[1] - 1);

  CRSF_write(packetCmd, 6, 0);
}
// Request parameter info from known device
// CRSF Config Protocol (wiki): <sync> 06 2C EE EF <idx> <chunk> <crc>
// UART to module uses EE address byte; logical CRSF bus uses C8 (see wiki example)
void CRSF_read_param(uint8_t n_param, uint8_t n_chunk, uint8_t target)    {
  uint8_t packetCmd[8];

  packetCmd[0] = ADDR_MODULE;
  packetCmd[1] = 6;
  packetCmd[2] = TYPE_SETTINGS_READ;
  packetCmd[3] = target;
  packetCmd[4] = CRSF_ADDRESS_ELRS_LUA;
  packetCmd[5] = n_param;
  packetCmd[6] = n_chunk;
  packetCmd[7] = crsf_crc8(&packetCmd[2], packetCmd[1] - 1);

#ifdef debug
  if (n_chunk == 0) {
    crsf_dbg.param_req++;
  } else {
    crsf_dbg.param_chunk_req++;
  }
  CRSF_DBG("TX 2C read id=%u chunk=%u\n", n_param, n_chunk);
  crsf_dbg_hex("  pkt", packetCmd, 8, 8);
#endif

  CRSF_write(packetCmd, 8, 0);
}
// request ELRS_info message (0x2E bad/good packet counts)
void CRSF_get_elrs_info(uint8_t target) {
  uint8_t packetCmd[8];

  packetCmd[0] = ELRS_ADDRESS;
  packetCmd[1] = 6;
  packetCmd[2] = TYPE_SETTINGS_WRITE;
  packetCmd[3] = target;
  packetCmd[4] = ADDR_RADIO;
  packetCmd[5] = TYPE_ELRS_INFO;
  packetCmd[6] = 0;
  packetCmd[7] = crsf_crc8(&packetCmd[2], packetCmd[1] - 1);

  CRSF_write(packetCmd, 8, 0);
}

void CRSF_send_id(uint8_t modelId)  {

  uint8_t packetCmd[LinkStatisticsFrameLength];

  packetCmd[0] = ELRS_ADDRESS;
  packetCmd[1] = 8;
  packetCmd[2] = TYPE_COMMAND_ID;
  packetCmd[3] = ELRS_ADDRESS;
  packetCmd[4] = ADDR_RADIO;
  packetCmd[5] = CRSF_SUBCOMMAND;
  packetCmd[6] = COMMAND_MODEL_SELECT_ID;
  packetCmd[7] = modelId; // modelID TODO
  packetCmd[8] = crsf_crc8_BA(&packetCmd[2], packetCmd[1] - 2);
  packetCmd[9] = crsf_crc8(&packetCmd[2], packetCmd[1] - 1);
  
  CRSF_write(packetCmd, LinkStatisticsFrameLength, 0);
}

uint32_t parse_u32(const uint8_t *buffer)
{
  return (buffer[0] << 24) | (buffer[1] << 16) | (buffer[2] << 8) | buffer[3];
}
void parse_device(uint8_t *buffer, crsf_device_t *device)
{
  buffer += 2;
  device->address = (uint8_t)*buffer++;
  strlcpy(device->name, (const char *)buffer, CRSF_MAX_NAME_LEN);
  buffer += strlen((const char *)buffer) + 1;
  device->serial_number = parse_u32(buffer);
  buffer += 4;
  device->hardware_id = parse_u32(buffer);
  buffer += 4;
  device->firmware_id = parse_u32(buffer);
  buffer += 4;
  device->number_of_params = *buffer;
  buffer += 1;
  device->params_version = *buffer;
  if (device->address == ADDR_MODULE)
  {
    if (device->serial_number == 0x454C5253)
    {
      //dbout.println("Module type: elrs");
      protocol_module_type(MODULE_ELRS);
    }
    else
    {
      //dbout.println("Module type: not elrs");
      protocol_module_type(MODULE_OTHER);
    }
  }
  /* dbout.printf("device details:%s,0x%x,%u,%u,%u,%u,%u\n",
                  device->name,
                  device->address,
                  device->number_of_params,
                  device->params_version,
                  device->serial_number,
                  device->firmware_id,
                  device->hardware_id); */
}

void parse_elrs_info(uint8_t *buffer)
{
  local_info.bad_pkts = buffer[3];                     // bad packet rate (should be 0)
  local_info.good_pkts = (buffer[4] << 8) + buffer[5]; // good packet rate (configured rate)

  // flags bit 0 indicates receiver connected
  // other bits indicate errors - error text in flags_info
  local_info.flags = buffer[6];
  strlcpy(local_info.flag_info, (const char *)&buffer[7], CRSF_MAX_NAME_LEN); // null-terminated text of flags

  local_info.update = elrs_info.update;
  if (memcmp((void *)&elrs_info, (void *)&local_info, sizeof(elrs_info_t) - CRSF_MAX_NAME_LEN))
  {
    if (local_info.flag_info[0] && strncmp(local_info.flag_info, elrs_info.flag_info, CRSF_MAX_NAME_LEN))
    {
      //dbout.printf("error: %s\n", local_info.flag_info);
      // example: error: Model Mismatch
      // error: [ ! Armed ! ]
    }

    memcpy((void *)&elrs_info, (void *)&local_info, sizeof(elrs_info_t) - CRSF_MAX_NAME_LEN);
    elrs_info.update++;
  }

  // example bad_pckts : good_pckts ; flag ; flag_info ; info_update ;
  //  0 : 100 ; 5 ; Model Mismatch ; 0
  //  0 : 200 ; 8 ; [ ! Armed ! ] ; 0
  // //dbout.printf("%u : %u ; %u ; %s ; %u\n ",local_info.bad_pkts,local_info.good_pkts,local_info.flags,local_info.flag_info,local_info.update);
}

void add_device(uint8_t *buffer)
{
  static uint8_t model_id_sent = 0;

  for (int i = 0; i < CRSF_MAX_DEVICES; i++)
  {
    if (crsf_devices[i].address == buffer[2] //  device already in table
        || crsf_devices[i].address == 0      //  not found, add to table
        || crsf_devices[i].address == ADDR_RADIO)
    { //  replace deviation device if necessary

#if defined(debug)
      //dbout.printf("device pong: 0x%x\n", buffer[2]);
#endif
      parse_device(buffer, &crsf_devices[i]);
      if (buffer[2] == ADDR_MODULE && crsf_devices[i].number_of_params > 0 &&
          !param_manager.params_loaded && param_manager.fields_count == 0) {
#ifdef debug
        CRSF_DBG("DEVICE_INFO: %s params=%u serial=0x%08lX\n",
                 crsf_devices[i].name,
                 crsf_devices[i].number_of_params,
                 (unsigned long)crsf_devices[i].serial_number);
#endif
        if (!model_id_sent) {
          CRSF_send_id(0);
          model_id_sent = 1;
        }
        dynamic_param_on_device_info(&param_manager, crsf_devices[i].number_of_params);
      }
      break;
    }
  }
  //  no new device added if no more space in table
}

void CRSF_discover_parameters(uint8_t device_id)
{
  uint32_t now = millis();

  if (!crsf_param_discovery_allowed()) {
    return;
  }

  if (param_manager.params_loaded || param_manager.fields_count == 0 || !param_manager.fields) {
    return;
  }

  if (param_manager.pending_field_id != PARAM_PENDING_NONE) {
    if (param_manager.field_chunk > 0) {
      if (!param_manager.chunk_need_request) {
        if ((now - param_manager.pending_since) < 1000) {
          return;
        }
      } else if ((now - param_manager.last_load_time) < 20) {
        return;
      }
      CRSF_read_param(param_manager.pending_field_id, param_manager.field_chunk, device_id);
      param_manager.chunk_need_request = 0;
      param_manager.pending_since = now;
      param_manager.last_load_time = now;
      return;
    }
    if ((now - param_manager.pending_since) < 1000) {
      return;
    }
#ifdef debug
    crsf_dbg.param_timeout++;
    CRSF_DBG("TIMEOUT waiting for param id=%u chunk=%u\n",
             param_manager.pending_field_id, param_manager.field_chunk);
#endif
    param_manager.field_chunk = 0;
    param_manager.field_data_len = 0;
    param_manager.expect_chunks_remain = -1;
    param_manager.pending_field_id = PARAM_PENDING_NONE;
  } else if ((now - param_manager.last_load_time) < 50) {
    return;
  }

  dynamic_param_tick(&param_manager, device_id, CRSF_read_param);
  if (param_manager.pending_field_id != PARAM_PENDING_NONE) {
    param_manager.pending_since = now;
  }
  param_manager.last_load_time = now;
}

void parse_parameter_entry(uint8_t *buffer, uint16_t buffer_len)
{
#ifdef debug
  crsf_dbg_hex("RX 2B", buffer, buffer_len, 32);
  CRSF_DBG("  dest=0x%02X origin=0x%02X id=%u chunks=%u len=%u pending=%u\n",
           buffer[0], buffer[1], buffer[2], buffer[3], buffer_len,
           param_manager.pending_field_id == PARAM_PENDING_NONE ? 255 : param_manager.pending_field_id);
#endif
  dynamic_param_parse_message(&param_manager, buffer, buffer_len, ADDR_MODULE);
}

static uint8_t param_value_size(const dynamic_field_t *field)
{
  if (!field) {
    return 1;
  }
  switch (field->type) {
  case CRSF_UINT16:
  case CRSF_INT16:
    return 2;
  case CRSF_UINT32:
  case CRSF_INT32:
  case CRSF_FLOAT:
    return 4;
  default:
    return 1;
  }
}

void dynamic_param_send_command(uint8_t device_id, uint8_t field_id, uint8_t status_byte)
{
  (void)device_id;
  uint8_t packet[8];
  buildElrsPacket(packet, field_id, status_byte);

#ifdef debug
  CRSF_DBG("TX 2D cmd id=%u status=%u\n", field_id, status_byte);
  crsf_dbg_hex("  pkt", packet, 8, 8);
#endif

  crsf_rc_timer_pause();
  CRSF_write(packet, 8, 0);
  delay(4);
  crsf_rc_timer_resume();
}

void dynamic_param_send_value(uint8_t device_id, dynamic_field_t *field)
{
  if (!field || field->type == CRSF_FOLDER || field->type == CRSF_INFO) {
    return;
  }

  if (field->type == CRSF_COMMAND) {
    uint8_t status_byte = dynamic_param_command_tx_byte(field);
    if (status_byte == 0) {
      return;
    }
    dynamic_param_send_command(device_id, field->id, status_byte);
    if (status_byte == CRSF_CMD_STATUS_START) {
      field->value = CRSF_CMD_STATUS_START;
      dynamic_command_popup_start(field->id, field->maxlen);
    }
    return;
  }

  uint8_t handset_id = CRSF_ADDRESS_ELRS_LUA;
  uint8_t size = param_value_size(field);
  int32_t value = field->value;

  if (field->size < 0) {
    size = (uint8_t)(-field->size);
    if (value < 0) {
      value = (int32_t)((1UL << (size * 8)) + value);
    }
  }

  uint8_t packet[16];
  packet[0] = ADDR_MODULE;
  packet[1] = (uint8_t)(5 + size);
  packet[2] = TYPE_SETTINGS_WRITE;
  packet[3] = device_id;
  packet[4] = handset_id;
  packet[5] = field->id;

  for (uint8_t i = 0; i < size; i++) {
    packet[6 + size - 1 - i] = (uint8_t)((value >> (8 * i)) & 0xFF);
  }

  packet[6 + size] = crsf_crc8(&packet[2], (uint8_t)(4 + size));
  CRSF_write(packet, (uint8_t)(7 + size), 0);
}

void send_parameter_value(uint8_t param_id, int32_t value, uint8_t size)
{
  const uint8_t handset_id = ADDR_RADIO;
  uint8_t packet[16];
  packet[0] = ADDR_MODULE;
  packet[1] = (uint8_t)(5 + size);
  packet[2] = TYPE_SETTINGS_WRITE;
  packet[3] = ELRS_ADDRESS;
  packet[4] = handset_id;
  packet[5] = param_id;

  for (uint8_t i = 0; i < size; i++) {
    packet[6 + size - 1 - i] = (value >> (8 * i)) & 0xFF;
  }

  packet[6 + size] = crsf_crc8(&packet[2], (uint8_t)(4 + size));
  CRSF_write(packet, (uint8_t)(7 + size), 0);
}

void CRSF_serial_rcv(uint8_t *buffer, uint8_t num_bytes)
{
  /*for (int i=0;i<num_bytes;i++) {
    //dbout.printf("0x%x:",buffer[i]);
    }
  //dbout.println(""); */

  if ((buffer[0] != CRSF_FRAMETYPE_RADIO_ID) && (buffer[0] != CRSF_FRAMETYPE_LINK_STATISTICS))
  {
    // //dbout.printf("CRSF FRAMETYPE: 0x%x : L:%u : ",buffer[0],num_bytes);
  }
  else
  {
#if !defined(DEBUG_CRSF_FRAMETYPE_RADIO_ID)
    if (buffer[0] != CRSF_FRAMETYPE_RADIO_ID)
    {
      /*  for (int i=0;i<num_bytes;i++) {
         //dbout.printf("0x%x:",buffer[i]);
       }
       //dbout.println(""); */
      //if (buffer[0] == TYPE_LINK)
        //rxConected++;

      // //dbout.printf("rx conn: %u\n",rxConected);
    }
#endif
  }
  switch (buffer[0])
  {
  case CRSF_FRAMETYPE_DEVICE_INFO:
#ifdef debug
    crsf_dbg.rx_0x29++;
#endif
    add_device(buffer);

    break;

  case CRSF_FRAMETYPE_ELRS_STATUS:
#ifdef debug
    crsf_dbg.rx_0x2e++;
#endif
    parse_elrs_info(buffer);
    break;

  case CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY:
#ifdef debug
    crsf_dbg.rx_0x2b++;
    crsf_dbg.last_rx_type = buffer[0];
#endif
    parse_parameter_entry((uint8_t *)(buffer + 1), (uint16_t)(num_bytes - 1));
    break;

  default:
#ifdef debug
    crsf_dbg.rx_other++;
    crsf_dbg.last_rx_type = buffer[0];
#endif
    break;
  }
}


uint8_t getCrossfireTelemetryValue(uint8_t index, int32_t *value, uint8_t len)
{
  uint8_t result = 0;
  uint8_t *byte = &SerialInBuffer[index];
  *value = (*byte & 0x80) ? -1 : 0;
  for (int i = 0; i < len; i++)
  {
    *value <<= 8;
    if (*byte != 0xff)
      result = 1;
    *value += *byte++;
  }
  return result;
}

static uint8_t crsf_is_valid_sync_byte(uint8_t inChar)
{
  if (inChar == CRSF_SYNC_BYTE) {
    return 1;
  }
  if (inChar == CRSF_ADDRESS_RADIO_TRANSMITTER) {
    return 1;
  }
  if (inChar == CRSF_ADDRESS_ELRS_LUA) {
    return 1;
  }
  return 0;
}

void serialtelemetryevent_2400()
{
  uint8_t bytes_budget = 64;

  while (ELRS_Serial_2400.available() && bytes_budget > 0)
  {
    bytes_budget--;
    const uint8_t inChar = (uint8_t)ELRS_Serial_2400.read();

    if (!CRSFframeActive)
    {
      if (!crsf_is_valid_sync_byte(inChar)) {
#ifdef debug
        crsf_dbg.sync_other++;
        crsf_dbg.last_sync_byte = inChar;
#endif
        continue;
      }
#ifdef debug
      if (inChar == CRSF_SYNC_BYTE) {
        crsf_dbg.sync_c8++;
      } else if (inChar == CRSF_ADDRESS_RADIO_TRANSMITTER) {
        crsf_dbg.sync_ea++;
      } else {
        crsf_dbg.sync_ef++;
      }
#endif
      CRSFframeActive = true;
      SerialInPacketPtr = 0;
      SerialInBuffer[SerialInPacketPtr++] = inChar;
      continue;
    }

    if (SerialInPacketPtr >= sizeof(SerialInBuffer)) {
      crsf_rx_reset();
      continue;
    }

    SerialInBuffer[SerialInPacketPtr++] = inChar;

    if (SerialInPacketPtr == 2) {
      SerialInPacketLen = SerialInBuffer[1];
      if (SerialInPacketLen < 2 || SerialInPacketLen > CRSF_PAYLOAD_SIZE_MAX) {
        crsf_rx_reset();
      }
      continue;
    }

    const uint8_t expected_total = (uint8_t)(SerialInPacketLen + 2);
    if (SerialInPacketPtr < expected_total) {
      continue;
    }

    if (SerialInPacketPtr == expected_total
        && SerialInBuffer[2] == CRSF_FRAMETYPE_PARAMETER_SETTINGS_ENTRY
        && SerialInPacketLen == CRSF_PAYLOAD_SIZE_MAX)
    {
      const uint8_t calc_crc = crsf_crc8(SerialInBuffer + 2, (uint8_t)(SerialInPacketLen - 1));
      const uint8_t got_crc = SerialInBuffer[SerialInPacketPtr - 1];
      if (got_crc != calc_crc
          && crsf_crc_looks_like_sync(got_crc)
          && ELRS_Serial_2400.available())
      {
        const uint8_t extra = (uint8_t)ELRS_Serial_2400.read();
        if (extra == calc_crc && SerialInPacketPtr < sizeof(SerialInBuffer)) {
          SerialInBuffer[SerialInPacketPtr++] = extra;
        }
      }
    }

    if (SerialInPacketPtr >= expected_total) {
      crsf_process_serial_frame();
    }
    crsf_rx_reset();
  }
}

// Build dynamic menus from discovered parameters
void build_handset_menus_from_params(void) {
  if (!tx_module_params.params_loaded) {
    return;  // Parameters not yet loaded
  }

  handset_menu.item_count = 0;
  handset_menu.folder_count = 0;
  handset_menu.current_folder_id = 0;

  // Track which folders we've seen
  uint8_t folder_map[16] = {0};  // Maps folder_id to folder index
  uint8_t folder_indices[16];

  // First pass: collect all unique folders and populate root items
  for (uint8_t i = 0; i < tx_module_params.param_count && handset_menu.item_count < CRSF_MAX_PARAMS; i++) {
    crsf_parameter_t *param = &tx_module_params.params[i];

    // Skip hidden parameters
    if (param->hidden) {
      continue;
    }

    // Skip folder types - they're navigation not content
    if (param->type == 11) {  // CRSF_FOLDER
      uint8_t folder_id = param->id;
      if (folder_map[folder_id] == 0) {
        // New folder
        handset_menu_folder_t *folder = &handset_menu.folders[handset_menu.folder_count];
        folder->folder_id = folder_id;
        strncpy(folder->folder_name, param->name, 31);
        folder->first_item_idx = handset_menu.item_count;
        folder->item_count = 0;
        folder_map[folder_id] = handset_menu.folder_count + 1;
        folder_indices[handset_menu.folder_count] = folder_id;
        handset_menu.folder_count++;
      }
      continue;
    }

    // Add this parameter as a menu item
    handset_menu_item_t *item = &handset_menu.items[handset_menu.item_count];
    item->param_id = param->id;
    item->param_ptr = param;
    strncpy(item->display_name, param->name, 31);
    item->folder_id = param->parent;
    item->is_visible = 1;

    // Update folder item count if this is in a folder
    if (param->parent > 0 && folder_map[param->parent] > 0) {
      uint8_t folder_idx = folder_map[param->parent] - 1;
      handset_menu.folders[folder_idx].item_count++;
    }

    handset_menu.item_count++;
  }
}

// Get visible items in a specific folder
uint8_t get_visible_items_in_folder(uint8_t folder_id, handset_menu_item_t **out_items) {
  uint8_t count = 0;

  for (uint8_t i = 0; i < handset_menu.item_count && count < CRSF_MAX_PARAMS; i++) {
    if (handset_menu.items[i].folder_id == folder_id && handset_menu.items[i].is_visible) {
      out_items[count] = &handset_menu.items[i];
      count++;
    }
  }

  return count;
}

// Get parameter at menu position (in current folder)
crsf_parameter_t* get_param_at_menu_position(uint8_t menu_idx) {
  uint8_t count = 0;

  for (uint8_t i = 0; i < handset_menu.item_count; i++) {
    if (handset_menu.items[i].folder_id == handset_menu.current_folder_id && handset_menu.items[i].is_visible) {
      if (count == menu_idx) {
        return handset_menu.items[i].param_ptr;
      }
      count++;
    }
  }

  return NULL;
}

#include "crsf_debug.c"
#include "crsf_dynamic.c"