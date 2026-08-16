#pragma once
#include <Arduino.h>
#define USBCON
#define PIO_FRAMEWORK_ARDUINO_ENABLE_CDC
/* --- pins --- */
#define battery_in PB1
#define Module_power_2400 PB9
#define joystick_Y PA1
#define joystick_T PA0
#define AUX2 PB5
#define AUX1 PB0
#define BTN_BACK PB14
#define BTN_NEXT PA8
#define BTN_PREV PB15
#define BTN_OK PB13

#define THR_MIN 0
#define THR_MAX 4095
#define YAW_MIN 0
#define THR_MAX 4095

/* --- peripherals --- */
#define ELRS_Serial_2400 Serial1
#define CRSF_baudrate 400000
#define CRSF_TIM_DEF TIM3
#define BOARD_HAS_SSD1315_OLED
#define EEPROM_SIZE 8
/* --- product options --- */
#define pistol_grip
#define SCHOOL
/* Pins are defined above, so the PIN_NONE fallbacks and helpers
 * can now fill in whatever this board lacks. Must come last. */
#include "../../boards_pins.h"