#pragma once
/*
 * Board pin abstraction: any pin absent on this target is PIN_NONE.
 * Rule: shared code never touches a raw pin macro; it goes through these helpers.
 */

#include <Arduino.h>
#include <stdint.h>
#include "rc/rc_limits.h"
/* ------------------------------------------------------------------ */
/* 1. Sentinel                                                         */
/* ------------------------------------------------------------------ */

#define PIN_NONE (-1)

/* PinName-aware validity test. Covers -1, NC (0xFFFFFFFF) and any
 * negative value coming from a board header. */
#define PIN_VALID(p) (((int32_t)(p)) >= 0)

/* ------------------------------------------------------------------ */
/* 2. Defaults: every pin symbol the shared code may reference.        */
/*    An env that does not define one gets PIN_NONE automatically,     */
/*    so shared code always compiles on every target.                  */
/* ------------------------------------------------------------------ */

/* power rails */
#ifndef Module_power_2400
  #define Module_power_2400 PIN_NONE
#endif
#ifndef Module_power_ESP
  #define Module_power_ESP PIN_NONE
#endif

/* sticks / axes */
#ifndef joystick_T
  #define joystick_T PIN_NONE
#endif
#ifndef joystick_Y
  #define joystick_Y PIN_NONE
#endif
#ifndef joystick_P
  #define joystick_P PIN_NONE
#endif
#ifndef joystick_R
  #define joystick_R PIN_NONE
#endif

/* aux switches / analog aux */
#ifndef AUX1
  #define AUX1 PIN_NONE
#endif
#ifndef AUX2
  #define AUX2 PIN_NONE
#endif
#ifndef AUX3
  #define AUX3 PIN_NONE
#endif
#ifndef AUX4
  #define AUX4 PIN_NONE
#endif
#ifndef AUX5
  #define AUX5 PIN_NONE
#endif
#ifndef AUX6
  #define AUX6 PIN_NONE
#endif

/* pots */
#ifndef POT1
  #define POT1 PIN_NONE
#endif
#ifndef POT2
  #define POT2 PIN_NONE
#endif

/* buttons (all four are mandatory for the menu, but keep uniform) */
#ifndef BTN_PREV
  #define BTN_PREV PIN_NONE
#endif
#ifndef BTN_NEXT
  #define BTN_NEXT PIN_NONE
#endif
#ifndef BTN_OK
  #define BTN_OK PIN_NONE
#endif
#ifndef BTN_BACK
  #define BTN_BACK PIN_NONE
#endif

/* misc peripherals */
#ifndef battery_in
  #define battery_in PIN_NONE
#endif
#ifndef Trainer_pin
  #define Trainer_pin PIN_NONE
#endif
#ifndef reset_pin
  #define reset_pin PIN_NONE
#endif
#ifndef buzzer
  #define buzzer PIN_NONE
#endif
#ifndef led_pin
  #define led_pin PIN_NONE
#endif

/* ------------------------------------------------------------------ */
/* 3. Capability macros derived from the pin table                     */
/*    Use these for #if, menu limits and mixer availability.           */
/* ------------------------------------------------------------------ */

#define BOARD_HAS_PITCH_AXIS   PIN_VALID(joystick_P)
#define BOARD_HAS_ROLL_AXIS    PIN_VALID(joystick_R)
#define BOARD_HAS_4AXIS        (BOARD_HAS_PITCH_AXIS && BOARD_HAS_ROLL_AXIS)
#define BOARD_HAS_BATTERY      PIN_VALID(battery_in)
#define BOARD_HAS_BUZZER       PIN_VALID(buzzer)
#define BOARD_HAS_LEDS         PIN_VALID(led_pin)
#define BOARD_HAS_TRAINER      PIN_VALID(Trainer_pin)
#define BOARD_HAS_ESP_MODULE    PIN_VALID(Module_power_ESP)
#define BOARD_HAS_RESET_PIN    PIN_VALID(reset_pin)

#define BOARD_AUX_COUNT ( \
    (PIN_VALID(AUX1) ? 1 : 0) + (PIN_VALID(AUX2) ? 1 : 0) + \
    (PIN_VALID(AUX3) ? 1 : 0) + (PIN_VALID(AUX4) ? 1 : 0) + \
    (PIN_VALID(AUX5) ? 1 : 0) + (PIN_VALID(AUX6) ? 1 : 0))

/* ------------------------------------------------------------------ */
/* 4. Channel defaults for absent hardware                             */
/* ------------------------------------------------------------------ */

//#define RC_CHANNEL_MID ((RC_CHANNEL_MIN + RC_CHANNEL_MAX) / 2)

/* logical defaults, chosen so a missing input is never "armed"       */
#define PIN_DEF_PULLUP_IDLE   1   /* button not pressed */
#define PIN_DEF_PULLDOWN_IDLE 0   /* switch off / disarmed */

/* ------------------------------------------------------------------ */
/* 5. Init helpers (statements)                                        */
/* ------------------------------------------------------------------ */

#define PIN_MODE(p, mode) \
    do { if (PIN_VALID(p)) pinMode((uint32_t)(p), (mode)); } while (0)

#define PIN_MODE_INPUT(p)          PIN_MODE((p), INPUT)
#define PIN_MODE_PULLUP(p)         PIN_MODE((p), INPUT_PULLUP)
#define PIN_MODE_PULLDOWN(p)       PIN_MODE((p), INPUT_PULLDOWN)
#define PIN_MODE_OUTPUT(p)         PIN_MODE((p), OUTPUT)

#define PIN_WRITE(p, val) \
    do { if (PIN_VALID(p)) digitalWrite((uint32_t)(p), (val)); } while (0)

#define PIN_PWM_WRITE(p, val) \
    do { if (PIN_VALID(p)) analogWrite((uint32_t)(p), (val)); } while (0)

#define PIN_ATTACH_IRQ(p, fn, mode) \
    do { if (PIN_VALID(p)) attachInterrupt(digitalPinToInterrupt((uint32_t)(p)), (fn), (mode)); } while (0)

#define PIN_DETACH_IRQ(p) \
    do { if (PIN_VALID(p)) detachInterrupt(digitalPinToInterrupt((uint32_t)(p))); } while (0)

#define PIN_TONE(p, freq, dur) \
    do { if (PIN_VALID(p)) tone((uint32_t)(p), (freq), (dur)); } while (0)

#define PIN_NO_TONE(p) \
    do { if (PIN_VALID(p)) noTone((uint32_t)(p)); } while (0)

/* ------------------------------------------------------------------ */
/* 6. Read helpers (expressions, single evaluation of p)               */
/* ------------------------------------------------------------------ */

static inline int pin_read_digital(int32_t p, int def_val) {
    return PIN_VALID(p) ? digitalRead((uint32_t)p) : def_val;
}

static inline int32_t pin_read_analog(int32_t p, int32_t def_val) {
    return PIN_VALID(p) ? (int32_t)analogRead((uint32_t)p) : def_val;
}

/* buttons are INPUT_PULLUP -> idle HIGH, pressed LOW */
static inline int pin_read_button(int32_t p) {
    return PIN_VALID(p) ? (digitalRead((uint32_t)p) == LOW) : 0;
}

/* switches are INPUT_PULLDOWN -> idle LOW */
static inline int pin_read_switch(int32_t p) {
    return pin_read_digital(p, PIN_DEF_PULLDOWN_IDLE);
}

/* ------------------------------------------------------------------ */
/* 7. Channel producers: the important part                            */
/*    Absent hardware yields a deliberate value, never a 0-read        */
/*    mapped through a calibration range.                              */
/* ------------------------------------------------------------------ */

/* digital switch -> channel, LOW=min HIGH=max, absent = min (safe/off) */
static inline int32_t ch_from_switch(int32_t p) {
    return PIN_VALID(p)
        ? (int32_t)map(digitalRead((uint32_t)p), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX)
        : RC_CHANNEL_MIN;
}

/* inverted digital (pullup buttons used as sim channels) */
static inline int32_t ch_from_button(int32_t p) {
    return PIN_VALID(p)
        ? (int32_t)map(digitalRead((uint32_t)p), 1, 0, RC_CHANNEL_MIN, RC_CHANNEL_MAX)
        : RC_CHANNEL_MIN;
}

/* analog aux / pot over full ADC span, absent = min */
static inline int32_t ch_from_analog_full(int32_t p, int32_t adc_lo, int32_t adc_hi) {
    return PIN_VALID(p)
        ? (int32_t)map(analogRead((uint32_t)p), adc_lo, adc_hi, RC_CHANNEL_MIN, RC_CHANNEL_MAX)
        : RC_CHANNEL_MIN;
}

/* calibrated stick axis, absent = CENTER (never an extreme) */
static inline int32_t ch_from_axis(int32_t p, int32_t adc_lo, int32_t adc_hi) {
    return PIN_VALID(p)
        ? (int32_t)map(analogRead((uint32_t)p), adc_lo, adc_hi, RC_CHANNEL_MIN, RC_CHANNEL_MAX)
        : RC_CHANNEL_MID;
}

/* throttle-like axis, absent = MIN (fail-safe low) */
static inline int32_t ch_from_throttle(int32_t p, int32_t adc_lo, int32_t adc_hi) {
    return PIN_VALID(p)
        ? (int32_t)map(analogRead((uint32_t)p), adc_lo, adc_hi, RC_CHANNEL_MIN, RC_CHANNEL_MAX)
        : RC_CHANNEL_MIN;
}

/* trim application stays the same; helper keeps the clamp in one place */
static inline int32_t ch_apply_trim(int32_t ch, int fine) {
    return constrain(ch + (fine - 127) * 2, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
}

/* ------------------------------------------------------------------ */
/* 9. Battery                                                          */
/* ------------------------------------------------------------------ */

/* returns -1.0f when the board has no divider, so the UI can show N/A */
static inline float board_battery_volts(float div_ratio) {
    if (!PIN_VALID(battery_in)) {
        return -1.0f;
    }
    return (float)analogRead((uint32_t)battery_in) * ((3.3f / 4096.0f) * div_ratio);
}