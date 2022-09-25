//Inputs outputs

bool ELRS_RF = false;

bool usb_data = 0;

int mixer_selected = 0;
unsigned long previousMillis = 0;
unsigned long lcdMillis = 0;

#define I2CEEPROM_ADDR 0x57

uint8_t ELRS_TX_Power;
uint8_t ELRS_Pkt_rate;
uint8_t ELRS_Tlm_ratio;

//Variables
//float battery_level = 0;
int throttle_fine = 127;
int yaw_fine = 127;
int pitch_fine = 127;
int roll_fine = 127;

int throttle = 0;
int yaw = 512;
int roll = 512;
int pitch = 512;

bool lcd_en = true;
bool command_to_send = false;

typedef struct {
  String label;
  int minVal;
  int maxVal;
  int currentVal;
  void (*handler)();
} STRUCT_MENUPOS;

typedef enum {
  BACK, NEXT, PREV, OK, NONE
} ENUM_BUTTON;

STRUCT_MENUPOS menu[19];

int currentMenuPos = 0;
int menuSize;
bool isInLowerLevel = false;
int tempVal;

uint8_t vtx_power;
uint8_t vtx_channel;
uint8_t vtx_band;
uint8_t vtx_pit;

#define bat_volt_div_ratio 2.751

float rx_voltage = 0.0;

//Team900
        // buildElrsPacket(crsfCmdPacket,X, 3);
        // 0 : ELRS status request => ??
        // 1 : Set Lua [Packet Rate]= 0 - 25Hz / 1 - 50Hz / 2 - 100Hz / 3 - 200Hz
        // 2 : Set Lua [Telem Ratio]= 0 - off / 1 - 1:128 / 2 - 1:64 / 3 - 1:32 / 4 - 1:16 / 5 - 1:8 / 6 - 1:4 / 7 - 1:2
        // 3 : Set Lua [Switch Mode]=0 -> Hybrid;Wide 
        // 4 : Set Lua [Model Match]=0 -> Off;On
        // 5 : Set Lua [TX Power]=0 - 10mW / 1 - 25mW
        // 6 : Set Lua [Max Power]=0 - 10mW / 1 - 25mW *dont force to change, but change after reboot if last power was greater
        // 7 : Set Lua [Dynamic]=0 -> Off;On;AUX9;AUX10;AUX11;AUX12 -> * @ ttgo screen
        // 8 : Set Lua [VTX Administrator]=0
        // 9 : Set Lua [Pitmode]=0 -> Off;On 
        // 10 : Set Lua [Band]=0 -> Off;A;B;E;F;R;L
        // 11:  Set Lua [Channel]=0 -> 1;2;3;4;5;6;7;8
        // 12 : Set Lua [Pwr Lvl]=0 -> -;1;2;3;4;5;6;7;8
        // 13 : Set Lua [Send VTx]=0 sending response for [Send VTx] chunk=0 step=2
        // 14 : Set Lua [WiFi Connectivity]=0
        // 15 : Set Lua [Enable WiFi]=0 sending response for [Enable WiFi] chunk=0 step=0
        // 16 : Set Lua [Enable Rx WiFi]=0 sending response for [Enable Rx WiFi] chunk=0 step=2
        // 17 : Set Lua [BLE Joystick]=0 sending response for [BLE Joystick] chunk=0 step=0
        //     Set Lua [BLE Joystick]=1 sending response for [BLE Joystick] chunk=0 step=3
        //     Set Lua [BLE Joystick]=2 sending response for [BLE Joystick] chunk=0 step=3
        // 19: Set Lua [Bad/Good]=0
        // 20: Set Lua [2.1.0 EU868]=0 =1 ?? get 
 

#define ELRS_LUA_COMMAND_REQUEST 0
#define ELRS_LUA_COMMAND_PACKET_RATE 1
#define ELRS_LUA_COMMAND_TELEM_RATIO 2
#define ELRS_LUA_COMMAND_SWITCH_MODE 3
#define ELRS_LUA_COMMAND_MODEL_MATCH 4
#define ELRS_LUA_COMMAND_TX_POWER_SET 5
#define ELRS_LUA_COMMAND_MAX_TX_POWER 6
#define ELRS_LUA_COMMAND_DYNAMIC_POWER 7
#define ELRS_LUA_COMMAND_VTX_ADMIN 9
#define ELRS_LUA_COMMAND_VTX_PIT_MODE 13
#define ELRS_LUA_COMMAND_VTX_BAND 10
#define ELRS_LUA_COMMAND_VTX_CH 11
#define ELRS_LUA_COMMAND_VTX_PWR_LVL 12
#define ELRS_LUA_COMMAND_SEND_VTX 14
//#define ELRS_LUA_COMMAND_WIFI_CONN 14
#define ELRS_LUA_COMMAND_ENABLE_WIFI 15
#define ELRS_LUA_COMMAND_EN_RX_WIFI 16
#define ELRS_LUA_COMMAND_EN_BLE 17


uint8_t elrs_tx_pwr_lvl[7] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
const char* elrs_tx_pwr_lvl_labels[7] = {"10mW   ", "25mW   ", "50mW   ", "100mW  ", "250mW  ", "500mW  ", "1000mW "};

uint8_t elrs_tx_pkt_rate[6] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05};
const char* elrs_tx_pkt_rate_labels_868[6] = {"25Hz  ", "50Hz  ", "100Hz ", "100Hz Full", "150Hz", "200Hz"};
const char* elrs_tx_pkt_rate_labels_2400[4] = {"50Hz  ", "100Hz ", "250Hz ", "500Hz "};

uint8_t elrs_telem_ratio[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
const char* elrs_telem_ratio_labels[8] = {"Telemetry OFF", "1:128", "1:64", "1:32", "1:16", "1:8", "1:4", "1:2"};

uint8_t vtx_bands[6] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
const char* vtx_bands_labels[6] = {"BOSCAM A", "BOSCAM B", "BOSCAM C", "BOSCAM E", "RACEBAND", "LOW BAND"};

uint8_t vtx_channels[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
const char* vtx_channels_labels[8] = {"1", "2", "3", "4", "5", "6", "7", "8"};

uint8_t vtx_pitmode[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
const char* vtx_pitmode_labels[8] = {"OFF", "ON", "AUX1", "AUX2", "AUX3", "AUX4", "AUX5", "AUX6"};

