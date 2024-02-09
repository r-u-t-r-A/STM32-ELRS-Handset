#include <Arduino.h>
#include <CrsfSerial.h>  // https://github.com/CapnBry/CRServoF/
#include <PicoGamepad.h> // https://gitlab.com/realrobots/PicoGamepad/-/tree/master

#include "calibration.h"

#include <RF24.h>
#include <SPI.h> 

#define CRSF_TX 4  // physical 6
#define CRSF_RX 5  // physical 7

#define LED_TYPE LED_TYPE_SINGLE
#define LED_PIN 25

#define LED_TYPE_SINGLE 1
#define LED_TYPE_NEOPIXEL 2
#define LED_TYPE_RGB 3

#define CE_PIN 20
#define CSN_PIN 17

int selected_protocol = 0;

RF24 radio(CE_PIN, CSN_PIN);

//const uint64_t My_radio_pipeIn = 0xE8E8F0F0E1LL;  

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

// This struct defines data, which are embedded inside the ACK payload
struct ackPayload {
  float vcc; // vehicle vcc voltage
  float batteryVoltage; // vehicle battery voltage
  boolean batteryOk; // the vehicle battery voltage is OK!
  byte channel = 1; // the channel number
};
ackPayload payload;

// Did the receiver acknowledge the sent data?
boolean transmissionState;


const uint64_t pipeOut[] PROGMEM = {
  0xE8E8F0F0E1LL, 0xE9E8F0F0B1LL, 0xE9E8F0F0B2LL, 0xE9E8F0F0B3LL, 0xE9E8F0F0B4LL, 0xE9E8F0F0B5LL,
  0xE9E8F0F0B6LL, 0xE9E8F0F0B7LL, 0xE9E8F0F0B8LL, 0xE9E8F0F0B9LL, 0xE9E8F0F0B0LL,
  0xE9E8F0F0C1LL, 0xE9E8F0F0C2LL, 0xE9E8F0F0C3LL, 0xE9E8F0F0C4LL, 0xE9E8F0F0C5LL,
  0xE9E8F0F0C6LL, 0xE9E8F0F0C7LL, 0xE9E8F0F0C8LL, 0xE9E8F0F0C9LL, 0xE9E8F0F0C0LL
};
const int maxVehicleNumber = (sizeof(pipeOut) / (sizeof(uint64_t)));


void boardSetup() {
      pinMode(LED_PIN,OUTPUT);
    }

    void led_off() {
      digitalWrite(LED_PIN, LOW);
    }
    
    void led_on() {
      digitalWrite(LED_PIN, HIGH);
    }

// Blink routine variables and state tracking

#define BLINK_TIME 60000                 // blink routine window (in ms)
#define BLINK_DELAY 500                  // delay in between led state change (in ms)

bool led_state = false;                  // track led on/off state
unsigned long ms_curr = 0;               // current time
unsigned long ms_last_link_changed = 0;  // last time crsf link changed
unsigned long ms_last_led_changed = 0;   // last time led changed state in blink routine

UART Serial2(CRSF_TX, CRSF_RX, NC, NC);

CrsfSerial crsf(Serial2, CRSF_BAUDRATE); // pass any HardwareSerial port
int channel_data = 0;
int map_data = 0;

PicoGamepad gamepad;

btn_config *c;

/***
 * This callback is called whenever new channel values are available.
 * Use crsf.getChannel(x) to get us channel values (1-16).
 ***/
void packetChannels_gamepad()
{
    // Manually expanding instead of looping so I can change params as needed
    
    // X - Channel 1 - A
    channel_data = crsf.getChannel(1);
    map_data = map(channel_data, \
      CHANNEL_1_LOW_EP,          \
      CHANNEL_1_HIGH_EP,         \
      JOYSTICK_LOW,              \
      JOYSTICK_HIGH);
    gamepad.SetX(map_data);
    
    // Y - Channel 2 - E
    channel_data = crsf.getChannel(2);
    map_data = map(channel_data, \
      CHANNEL_2_LOW_EP,          \
      CHANNEL_2_HIGH_EP,         \
      JOYSTICK_LOW,              \
      JOYSTICK_HIGH);
    gamepad.SetY(map_data);
    
    // Rx - Channel 3 - T
    channel_data = crsf.getChannel(3);
    map_data = map(channel_data, \
      CHANNEL_3_LOW_EP,          \
      CHANNEL_3_HIGH_EP,         \
      JOYSTICK_LOW,              \
      JOYSTICK_HIGH);
    gamepad.SetRx(map_data);
    
    // Ry - Channel 4 - R
    channel_data = crsf.getChannel(4);
    map_data = map(channel_data, \
      CHANNEL_4_LOW_EP,          \
      CHANNEL_4_HIGH_EP,         \
      JOYSTICK_LOW,              \
      JOYSTICK_HIGH);
    gamepad.SetRy(map_data);

    // Z - Channel 5
    channel_data = crsf.getChannel(5);
    map_data = map(channel_data, \
      CHANNEL_5_LOW_EP,          \
      CHANNEL_5_HIGH_EP,         \
      JOYSTICK_LOW,              \
      JOYSTICK_HIGH);
    gamepad.SetZ(map_data);

    // Rz - Channel 6
    channel_data = crsf.getChannel(6);
    map_data = map(channel_data, \
      CHANNEL_6_LOW_EP,          \
      CHANNEL_6_HIGH_EP,         \
      JOYSTICK_LOW,              \
      JOYSTICK_HIGH);
    gamepad.SetRz(map_data);
    
    // Rx - Channel 7
    channel_data = crsf.getChannel(7);
    map_data = map(channel_data, \
      CHANNEL_7_LOW_EP,          \
      CHANNEL_7_HIGH_EP,         \
      JOYSTICK_LOW,              \
      JOYSTICK_HIGH);
    gamepad.SetThrottle(map_data);

    // Rx - Channel 8
    channel_data = crsf.getChannel(8);
    map_data = map(channel_data, \
      CHANNEL_8_LOW_EP,          \
      CHANNEL_8_HIGH_EP,         \
      JOYSTICK_LOW,              \
      JOYSTICK_HIGH);
    gamepad.SetS0(map_data);

    // Ry - unused
    // gamepad.SetRy(map_data);
    // Rz - unused
    // gamepad.SetRz(map_data);
    // S0 - unused
    // gamepad.SetS0(map_data);

    // Multi-position switches can be set up in calibrations.h
    // The button will report HIGH when the channel is withing
    // a lower / upper bound (inclusive) constraint.
    // Default is HIGH (1510, 2011) else LOW

    for(uint8_t i = 0; i < NUM_BUTTONS; i++){
      c = &btn_map[i];
      channel_data = crsf.getChannel(c->channel);
      // bounds check inclusive
      if(channel_data >= c->lower_bound && channel_data <= c->upper_bound) {
        map_data = c->invert ? LOW : HIGH;
      }
      else {
        map_data = c->invert ? HIGH : LOW;
      }
      gamepad.SetButton(c->id, map_data);

      #ifdef BTN_PRINT
      Serial.print("b: "); Serial.print(c->id));
      Serial.print(" c: "); Serial.print(channel_data); 
      Serial.print(" m: "); Serial.println(map_data);
      #endif
    }
    // TODO what to do with Channel 13,14,15,16 (NA,NA,LQ,RSSI)

    // Set hat direction, 4 hats available. direction is clockwise 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW 8=CENTER 
    // gamepad.SetHat(0, 8);

    gamepad.send_update();

}

void packetChannels_nrf24()
{    
  tx.txCH1 = map(crsf.getChannel(1), 989, 2011, 0, 255);
  tx.txCH2 = map(crsf.getChannel(2), 989, 2011, 0, 255);
  tx.txCH3 = map(crsf.getChannel(3), 989, 2011, 0, 255);
  tx.txCH4 = map(crsf.getChannel(4), 989, 2011, 0, 255);
  tx.txCH5 = map(crsf.getChannel(5), 989, 2011, 0, 1);
  tx.txCH6 = map(crsf.getChannel(6), 989, 2011, 0, 1);
  tx.txCH7 = map(crsf.getChannel(7), 989, 2011, 0, 1);
  tx.txCH8 = map(crsf.getChannel(8), 989, 2011, 0, 1);
  radio.write(&tx, sizeof(TX_data));
}

void crsfLinkUp() {
  ms_last_link_changed = millis();
  ms_last_led_changed = ms_last_link_changed;
  led_state = true;
  led_on();
}

void crsfLinkDown() {
  ms_last_link_changed = millis();
  ms_last_led_changed = ms_last_link_changed;
  led_state = false;
  led_off();
}

void led_loop() {
  ms_curr = millis();
  // link is down
  if(!crsf.isLinkUp()) {
    // within the blink routine window (BLINK_TIME)
    if(ms_curr < (ms_last_link_changed + BLINK_TIME)) {
      // handle led toggle delay
      if(ms_curr > (ms_last_led_changed + BLINK_DELAY)) {
        ms_last_led_changed = ms_curr;
        led_state ? led_on() : led_off();
        led_state = !led_state;  // toggle led state
      }
    }
    else
    {
      // ensure the led is off if the blink routine expired and link is down
      led_off();
    }
  }
}

void Config() {
  //if (crsf.getCommandType() == 1) {
  if (crsf.getCommandType() == 20) {

    selected_protocol = crsf.getCommandValue();
    selected_protocol = constrain(selected_protocol, 0, 1);
    return;

  } else if (crsf.getCommandType() == 21) {

    int selected_pipe = crsf.getCommandValue();
    selected_pipe = constrain(selected_pipe, 0, 20);
    radio.openWritingPipe(pipeOut[selected_pipe]);
    return;

  } else if (crsf.getCommandType() == 22) {

    int rf_ch = crsf.getCommandValue();
    rf_ch = constrain(rf_ch, 0, 125);
    radio.setChannel(rf_ch);
    return;

  } else if (crsf.getCommandType() == 23) {
    
    int rf_pa_level = crsf.getCommandValue();
    rf_pa_level = constrain(rf_pa_level, 0, 3);
    radio.setPALevel(rf_pa_level);
    
  } else {

  }
}

void setup()
{
  Serial.begin(115200);
  boardSetup();
  crsfLinkDown();

  gamepad.send_update();

    // Attach the channels callback
  crsf.onPacketChannels = &packetChannels_gamepad;
  crsf.onLinkUp = &crsfLinkUp;
  crsf.onLinkDown = &crsfLinkDown;
  crsf.onCommandPacket = &Config;
   // crsf.onOobData = &crsfOobData;
  crsf.begin();
   // serialEcho = true;

  radio.begin();
  //radio.setAutoAck(pipeOut[0], true);
  radio.setAutoAck(false);
  //radio.enableAckPayload();
  radio.setChannel(125);
  radio.setPALevel(RF24_PA_MIN);
  radio.setDataRate(RF24_250KBPS);
  ////radio.enableDynamicPayloads();
  radio.setPayloadSize(sizeof(TX_data));
 // radio.setRetries(5, 5);
  //radio.openReadingPipe(1, My_radio_pipe);
  radio.openWritingPipe(pipeOut[0]);
  radio.powerDown();

  tx.txCH1 = 127;
  tx.txCH2 = 0;
  tx.txCH3 = 127;
  tx.txCH4 = 127;
  tx.txCH5 = 0;
  tx.txCH6 = 0;
  tx.txCH7 = 0;
  tx.txCH8 = 0;
}

void loop()
{
    // Must call CrsfSerial.loop() in loop() to process data
  crsf.loop();
    //checkSerialIn();
    
  led_loop();

  if (selected_protocol == 1 && crsf.onPacketChannels != &packetChannels_nrf24) {
    radio.powerUp();
    crsf.onPacketChannels = &packetChannels_nrf24;
    
  } else if (selected_protocol == 0 && crsf.onPacketChannels != &packetChannels_gamepad) {
    crsf.onPacketChannels = &packetChannels_gamepad;
    radio.powerDown();
  }
   
} 