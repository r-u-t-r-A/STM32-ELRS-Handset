//Inputs outputs
#define battery_in PB0                  //pin for analog in from the battery divider
#define buttons_analog_in PB1           //Analog in from all the push buttons
#define ELRS_Serial Serial1
#define CRSF_baudrate 115200
#define Module_power PB12

bool ELRS_RF = false;

const uint64_t radio_pipe = 0xE8E8F0F0E1LL;
int ch = 50;

unsigned long previousMillis = 0;
unsigned long lcdMillis = 0;
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

// The sizeof this struct should not exceed 32 bytes
struct TX_data {

  byte txCH1;
  byte txCH2;
  byte txCH3;
  byte txCH4;
  byte txCH5;
  byte txCH6;
  byte txCH7;
  byte txCH8;

};

//Create a variable with the structure above and name it tx
TX_data tx;

uint32_t crsfTime = 0;

#define BTN_BACK  PB13
#define BTN_NEXT  PB14
#define BTN_PREV  PB15
#define BTN_OK    PA8

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

STRUCT_MENUPOS menu[11];

int currentMenuPos = 0;
int menuSize;
bool isInLowerLevel = false;
int tempVal;

static uint8_t currentPower[8] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};//  list =  {'10 mW', '25 mW', '50 mW', '100 mW', '250 mW', '500 mW', '1000 mW', '2000 mW'},
  //                                            0        1         2        3        4   
static uint8_t currentPktRate[4] = {0x06, 0x05, 0x04, 0x02}; //  '25Hz(-123dbm)', '50Hz(-120dbm)', '100Hz(-117dbm)', '200Hz(-112dbm)'
  //                                           0              1               3                 5      

#define SBUS_MIN_OFFSET 173
#define SBUS_MID_OFFSET 992
#define SBUS_MAX_OFFSET 1811
#define SBUS_CHANNEL_NUMBER 16
#define SBUS_PACKET_LENGTH 25
#define SBUS_FRAME_HEADER 0x0f
#define SBUS_FRAME_FOOTER 0x00
#define SBUS_FRAME_FOOTER_V2 0x04
#define SBUS_STATE_FAILSAFE 0x08
#define SBUS_STATE_SIGNALLOSS 0x04
#define SBUS_UPDATE_RATE 15 //ms

#define SBUS


