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

STRUCT_MENUPOS menu[16];

int currentMenuPos = 0;
int menuSize;
bool isInLowerLevel = false;
int tempVal;

uint8_t vtx_power;
uint8_t vtx_channel;
uint8_t vtx_band;

