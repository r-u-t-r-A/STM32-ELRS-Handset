#ifndef HANDSET_MENU_H
#define HANDSET_MENU_H

#include <stdint.h>

typedef enum {
    SCREEN_MAIN = 0,
    SCREEN_ELRS = 1,
    SCREEN_TRIM = 2,
    SCREEN_COUNT = 3
} handset_screen_t;

#define MENU_BLINK_MS 500
#define MENU_HOLD_DELAY_MS 1000
#define MENU_HOLD_REPEAT_MS 100

void menu_blink_reset(void);
uint8_t menu_blink_tick(unsigned long now);
uint8_t menu_line_inverted(uint8_t is_selected, uint8_t is_edit_mode);

void menu_hold_repeat_reset(void);
void menu_hold_repeat_arm(uint8_t btn);
int8_t menu_hold_repeat_tick(unsigned long now, uint8_t current_btn);

#define HANDSET_PROTOCOL_ELRS 0
#define HANDSET_PROTOCOL_ESP 1

#define EEPROM_PROTOCOL_ADDR 5
#define EEPROM_MIXER_ADDR 12
#define EEPROM_BUZZER_ADDR 14
#define EEPROM_LEDS_ADDR 15

void handset_main_enter(void);
void handset_main_draw(void);
void handset_main_handle_btn(uint8_t btn);
uint8_t handset_main_edit_mode(void);
void handset_main_edit_step(int8_t delta);

void handset_trim_enter(void);
void handset_trim_draw(void);
void handset_trim_handle_btn(uint8_t btn);
uint8_t handset_trim_edit_mode(void);
void handset_trim_edit_step(int8_t delta);
void handset_trim_update_channel(unsigned long now);

#endif
