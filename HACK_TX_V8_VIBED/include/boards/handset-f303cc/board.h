#pragma once
#include <Arduino.h>
#define USBCON
#define PIO_FRAMEWORK_ARDUINO_ENABLE_CDC
/* --- pins --- */
#define battery_in        PB1
#define bat_volt_div_ratio 5.848
#define Module_power_ESP  PB12
#define Module_power_2400 PB8
#define joystick_Y        PA0
#define joystick_T        PA1
#define joystick_P        PA6
#define joystick_R        PA5
#define AUX1              PB3
#define AUX2              PB0
#define AUX3              PB9
#define AUX4              PB5
#define BTN_PREV          PB13
#define BTN_OK            PB15
#define BTN_BACK          PA8
#define BTN_NEXT          PB14
#define Trainer_pin       PB10
#define reset_pin         PB4

/* --- peripherals --- */
#define BOARD_HAS_ESP_SERIAL
#define ESP_Serial        Serial1
#define ELRS_Serial_2400  Serial2
#define CRSF_baudrate     400000
#define CRSF_TIM_DEF      TIM2
#define EEPROM_SIZE 1
#define OLED_SCL          PB6
#define OLED_SDA          PB7
/* --- product options --- */
#define SCHOOL
#define handset
/* Pins are defined above, so the PIN_NONE fallbacks and helpers
 * can now fill in whatever this board lacks. Must come last. */
#include "../../boards_pins.h"