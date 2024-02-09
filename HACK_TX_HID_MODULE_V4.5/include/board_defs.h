#ifndef BOARD_DEFS
#define BOARD_DEFS

#define LED_TYPE_SINGLE 1
#define LED_TYPE_NEOPIXEL 2
#define LED_TYPE_RGB 3

// PiPico
#if BOARD_ID == 1
    #define CRSF_TX 4  // physical 6
    #define CRSF_RX 5  // physical 7

    #define LED_TYPE LED_TYPE_SINGLE
    #define LED_PIN 25
    
    void boardSetup() {
      pinMode(LED_PIN,OUTPUT);
    }

    void led_off() {
      digitalWrite(LED_PIN, LOW);
    }
    
    void led_on() {
      digitalWrite(LED_PIN, HIGH);
    }
#endif
#endif
