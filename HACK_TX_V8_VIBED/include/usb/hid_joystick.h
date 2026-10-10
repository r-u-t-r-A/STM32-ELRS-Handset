/*
 * USB HID joystick for PC simulators (protocol "SIM").
 *
 * Runs on the STM32duino USB device stack, which is compiled in by
 * -D PIO_FRAMEWORK_ARDUINO_ENABLE_HID (platformio.ini). The core's own
 * mouse/keyboard class is never registered; this file registers its own
 * joystick class instead. Nothing touches the USB pins until
 * hid_joystick_begin() is called, so with ELRS/ESP the handset never
 * enumerates.
 *
 * Report: 8 axes (CH1-CH8, 0..2047) + 12 buttons (CH5-CH16 above mid).
 */

#ifndef HID_JOYSTICK_H
#define HID_JOYSTICK_H

#include <stdint.h>

#define HID_JOY_AXIS_COUNT   8
#define HID_JOY_AXIS_MAX     2047
#define HID_JOY_BUTTON_FIRST 4   /* rcChannels index of button 1 (CH5) */
#define HID_JOY_BUTTON_COUNT 12

/* Bring up USB and start enumeration. Call once, from setup(). */
void hid_joystick_begin(void);

/* Pack rcChannels[] into a report and queue it. Skips the frame if the
 * host has not configured the device yet or the previous report is still
 * in flight. Safe to call from the CRSF timer ISR. */
void hid_joystick_send(const int *channels);

/* 1 once the host has configured the device. */
uint8_t hid_joystick_connected(void);

#endif
