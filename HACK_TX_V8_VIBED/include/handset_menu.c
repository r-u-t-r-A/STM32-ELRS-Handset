#include "handset_menu.h"
#include "oled_text.h"
#include <Arduino.h>
#include <U8x8lib.h>
#include <string.h>

#ifdef BOARD_HAS_SSD1315_OLED
    extern U8X8_SSD1315_128X64_NONAME_HW_I2C oled;
#else
    extern U8X8_SH1106_128X64_NONAME_HW_I2C oled;
#endif

extern int throttle_fine;
extern int yaw_fine;
extern int pitch_fine;
extern int roll_fine;
extern int throttle;
extern int yaw;
extern int pitch;
extern int roll;
extern int mixer_selected;
extern int control_protocol;
extern bool use_buzzer;
extern bool use_leds;

extern const char *mixer_labels[];

#define HANDSET_MIXER_COUNT number_of_mixers
#define HANDSET_PROTOCOL_COUNT 3

static const char *protocol_labels[HANDSET_PROTOCOL_COUNT] = {
    "ELRS",
    "ESP",
    "SIM",
};

void EEPROM_update(unsigned int addr, byte to_write);
byte EEPROM_read(unsigned int addr);

enum {
    MENU_ACT_BACK = 0,
    MENU_ACT_NEXT = 1,
    MENU_ACT_PREV = 2,
    MENU_ACT_OK = 3
};

static unsigned long menu_blink_millis = 0;
static uint8_t menu_blink_on = 1;

#define MENU_HOLD_BTN_NONE 0xFF

static struct {
    uint8_t held_btn;
    unsigned long hold_started_ms;
    unsigned long last_repeat_ms;
    uint8_t repeat_active;
} hold_repeat = {MENU_HOLD_BTN_NONE, 0, 0, 0};

void menu_hold_repeat_reset(void) {
    hold_repeat.held_btn = MENU_HOLD_BTN_NONE;
    hold_repeat.repeat_active = 0;
}

void menu_hold_repeat_arm(uint8_t btn) {
    if (btn == MENU_ACT_NEXT || btn == MENU_ACT_PREV) {
        hold_repeat.held_btn = btn;
        hold_repeat.hold_started_ms = millis();
        hold_repeat.last_repeat_ms = hold_repeat.hold_started_ms;
        hold_repeat.repeat_active = 0;
    }
}

int8_t menu_hold_repeat_tick(unsigned long now, uint8_t current_btn) {
    if (hold_repeat.held_btn == MENU_HOLD_BTN_NONE) {
        return 0;
    }
    if (current_btn != hold_repeat.held_btn) {
        menu_hold_repeat_reset();
        return 0;
    }

    if (!hold_repeat.repeat_active) {
        if ((now - hold_repeat.hold_started_ms) < MENU_HOLD_DELAY_MS) {
            return 0;
        }
        hold_repeat.repeat_active = 1;
        hold_repeat.last_repeat_ms = now;
        return (hold_repeat.held_btn == MENU_ACT_NEXT) ? 1 : -1;
    }

    if ((now - hold_repeat.last_repeat_ms) < MENU_HOLD_REPEAT_MS) {
        return 0;
    }
    hold_repeat.last_repeat_ms = now;
    return (hold_repeat.held_btn == MENU_ACT_NEXT) ? 1 : -1;
}

typedef struct {
    const char *label;
    int *value;
    int *channel_value;
    int min;
    int max;
    uint8_t eeprom_slot;
} handset_item_t;

typedef struct {
    uint8_t line_index;
    uint8_t edit_mode;
} static_menu_state_t;

static static_menu_state_t trim_state;
static handset_item_t trim_items[4];

static static_menu_state_t main_state;

static const char *main_item_labels[] = {
    "Protocol",
    "Mixer",
    "Buzzer",
    "LEDs",
};

#define MAIN_ITEM_COUNT 3

static void main_reload_trims(void) {
    throttle_fine = EEPROM_read((unsigned int)(32 + mixer_selected * 4));
    yaw_fine = EEPROM_read((unsigned int)(33 + mixer_selected * 4));
    pitch_fine = EEPROM_read((unsigned int)(34 + mixer_selected * 4));
    roll_fine = EEPROM_read((unsigned int)(35 + mixer_selected * 4));

}

static void main_format_value(uint8_t index, char *out, uint16_t out_size) {
    if (!out || out_size < 2) {
        return;
    }
    out[0] = 0;

    switch (index) {
    case 0:
        if (control_protocol >= 0 && control_protocol < HANDSET_PROTOCOL_COUNT) {
            snprintf(out, out_size, "%s", protocol_labels[control_protocol]);
        } else {
            snprintf(out, out_size, "%d", control_protocol);
        }
        break;
    case 1:
        if (mixer_selected >= 0 && mixer_selected < HANDSET_MIXER_COUNT) {
            snprintf(out, out_size, "%s", mixer_labels[mixer_selected]);
        } else {
            snprintf(out, out_size, "%d", mixer_selected);
        }
        break;
    case 2:
        snprintf(out, out_size, "%s", use_buzzer ? "ON" : "OFF");
        break;
    case 3:
        snprintf(out, out_size, "%s", use_leds ? "ON" : "OFF");
        break;
    default:
        break;
    }
}

void handset_main_enter(void) {
    main_state.line_index = 0;
    main_state.edit_mode = 0;
    menu_blink_reset();
}

uint8_t handset_main_edit_mode(void) {
    return main_state.edit_mode;
}

void handset_main_draw(void) {
    if (main_state.line_index >= MAIN_ITEM_COUNT) {
        main_state.line_index = MAIN_ITEM_COUNT - 1;
    }

    for (uint8_t i = 0; i < MAIN_ITEM_COUNT; i++) {
        char value[20];
        char line[28];
        main_format_value(i, value, sizeof(value));
        snprintf(line, sizeof(line), "%s %s", main_item_labels[i], value);

        if (menu_line_inverted(i == main_state.line_index, main_state.edit_mode)) {
            oled.setInverseFont(1);
        }

        oled.setCursor(0, i);
        oled.print(chop_chars(line, OLED_ROW_CHARS));
        oled.setInverseFont(0);
    }
}

static void main_redraw(void) {
    oled.clearDisplay();
    handset_main_draw();
}

static void main_adjust(int8_t delta) {
    switch (main_state.line_index) {
    case 0:
        control_protocol += delta;
        if (control_protocol < 0) {
            control_protocol = 0;
        }
        if (control_protocol >= HANDSET_PROTOCOL_COUNT) {
            control_protocol = HANDSET_PROTOCOL_COUNT - 1;
        }
        break;
    case 1:
        mixer_selected += delta;
        if (mixer_selected < 0) {
            mixer_selected = 0;
        }
        if (mixer_selected >= HANDSET_MIXER_COUNT) {
            mixer_selected = HANDSET_MIXER_COUNT - 1;
        }
        break;
    case 2:
        use_buzzer = (bool)((int)use_buzzer + delta);
        if ((int)use_buzzer < 0) {
            use_buzzer = false;
        }
        if ((int)use_buzzer > 1) {
            use_buzzer = true;
        }
        break;
    case 3:
        use_leds = (bool)((int)use_leds + delta);
        if ((int)use_leds < 0) {
            use_leds = false;
        }
        if ((int)use_leds > 1) {
            use_leds = true;
        }
        break;
    default:
        break;
    }
}

static void main_save_item(uint8_t index) {
    switch (index) {
    case 0:
        EEPROM_update(EEPROM_PROTOCOL_ADDR, (byte)control_protocol);
        break;
    case 1:
        EEPROM_update(EEPROM_MIXER_ADDR, (byte)mixer_selected);
        main_reload_trims();
        break;
    case 2:
        EEPROM_update(EEPROM_BUZZER_ADDR, (byte)use_buzzer);
        break;
    case 3:
        EEPROM_update(EEPROM_LEDS_ADDR, (byte)use_leds);
        break;
    default:
        break;
    }
}

void handset_main_edit_step(int8_t delta) {
    if (!main_state.edit_mode || delta == 0) {
        return;
    }
    main_adjust(delta);
    main_redraw();
}

void handset_main_handle_btn(uint8_t btn) {
    if (main_state.edit_mode) {
        if (btn == MENU_ACT_NEXT) {
            main_adjust(1);
            menu_hold_repeat_arm(btn);
            main_redraw();
        } else if (btn == MENU_ACT_PREV) {
            main_adjust(-1);
            menu_hold_repeat_arm(btn);
            main_redraw();
        } else if (btn == MENU_ACT_OK) {
            main_save_item(main_state.line_index);
            main_state.edit_mode = 0;
            menu_hold_repeat_reset();
            menu_blink_reset();
            main_redraw();
        } else if (btn == MENU_ACT_BACK) {
            main_state.edit_mode = 0;
            menu_hold_repeat_reset();
            menu_blink_reset();
            main_redraw();
        }
        return;
    }

    if (btn == MENU_ACT_NEXT) {
        if (main_state.line_index + 1 < MAIN_ITEM_COUNT) {
            main_state.line_index++;
            main_redraw();
        }
    } else if (btn == MENU_ACT_PREV) {
        if (main_state.line_index > 0) {
            main_state.line_index--;
            main_redraw();
        }
    } else if (btn == MENU_ACT_OK) {
        main_state.edit_mode = 1;
        menu_blink_reset();
        main_redraw();
    }
}

void menu_blink_reset(void) {
    menu_blink_millis = millis();
    menu_blink_on = 1;
}

uint8_t menu_blink_tick(unsigned long now) {
    if ((now - menu_blink_millis) >= MENU_BLINK_MS) {
        menu_blink_millis = now;
        menu_blink_on = (uint8_t)!menu_blink_on;
        return 1;
    }
    return 0;
}

uint8_t menu_line_inverted(uint8_t is_selected, uint8_t is_edit_mode) {
    if (!is_selected) {
        return 0;
    }
    if (!is_edit_mode) {
        return 1;
    }
    return menu_blink_on;
}

static unsigned int trim_eeprom_addr(const handset_item_t *item) {
    return (unsigned int)(32 + mixer_selected * 4 + item->eeprom_slot);
}

static void trim_refresh_items(void) {
    trim_items[0].label = "Yaw trim";
    trim_items[0].value = &yaw_fine;
    trim_items[0].channel_value = &yaw;
    trim_items[0].min = 0;
    trim_items[0].max = 255;
    trim_items[0].eeprom_slot = 1;

    trim_items[1].label = "Thr trim";
    trim_items[1].value = &throttle_fine;
    trim_items[1].channel_value = &throttle;
    trim_items[1].min = 0;
    trim_items[1].max = 255;
    trim_items[1].eeprom_slot = 0;

    trim_items[2].label = "Roll trim";
    trim_items[2].value = &roll_fine;
    trim_items[2].channel_value = &roll;
    trim_items[2].min = 0;
    trim_items[2].max = 255;
    trim_items[2].eeprom_slot = 3;

    trim_items[3].label = "Pitch trim";
    trim_items[3].value = &pitch_fine;
    trim_items[3].channel_value = &pitch;
    trim_items[3].min = 0;
    trim_items[3].max = 255;
    trim_items[3].eeprom_slot = 2;
}

void handset_trim_enter(void) {
    trim_refresh_items();
    trim_state.line_index = 0;
    trim_state.edit_mode = 0;
    menu_blink_reset();
}

uint8_t handset_trim_edit_mode(void) {
    return trim_state.edit_mode;
}

static void trim_draw_channel_row(void) {
    if (!trim_state.edit_mode) {
        return;
    }

    handset_item_t *item = &trim_items[trim_state.line_index];
    char ch_line[20];
    snprintf(ch_line, sizeof(ch_line), "%4d", *item->channel_value);

    oled.setCursor(0, 4);
    oled.print(chop_chars("                ", OLED_ROW_CHARS));
    oled.setCursor(0, 4);
    oled.print(chop_chars(ch_line, OLED_ROW_CHARS));
}

void handset_trim_draw(void) {
    const uint8_t item_count = 4;

    if (trim_state.line_index >= item_count) {
        trim_state.line_index = item_count - 1;
    }

    for (uint8_t i = 0; i < item_count; i++) {
        handset_item_t *item = &trim_items[i];
        char line[24];
        snprintf(line, sizeof(line), "%s %3d", item->label, *item->value);

        if (menu_line_inverted(i == trim_state.line_index, trim_state.edit_mode)) {
            oled.setInverseFont(1);
        }

        oled.setCursor(0, i);
        oled.print(chop_chars(line, OLED_ROW_CHARS));
        oled.setInverseFont(0);
    }

    if (trim_state.edit_mode) {
        trim_draw_channel_row();
    } else {
        oled.setCursor(0, 4);
        oled.print(chop_chars("                ", OLED_ROW_CHARS));
    }
}

void handset_trim_update_channel(unsigned long now) {
    static unsigned long last_ms = 0;

    if (!trim_state.edit_mode) {
        return;
    }
    if ((now - last_ms) < 100) {
        return;
    }
    last_ms = now;
    trim_draw_channel_row();
}

static void trim_redraw(void) {
    oled.clearDisplay();
    handset_trim_draw();
}

static void trim_adjust(int8_t delta) {
    handset_item_t *item = &trim_items[trim_state.line_index];
    int next = *item->value + delta;
    if (next < item->min) {
        next = item->min;
    }
    if (next > item->max) {
        next = item->max;
    }
    *item->value = next;
}

void handset_trim_edit_step(int8_t delta) {
    if (!trim_state.edit_mode || delta == 0) {
        return;
    }
    trim_adjust(delta);
    trim_redraw();
}

void handset_trim_handle_btn(uint8_t btn) {
    const uint8_t item_count = 4;

    if (trim_state.edit_mode) {
        if (btn == MENU_ACT_NEXT) {
            trim_adjust(1);
            menu_hold_repeat_arm(btn);
            trim_redraw();
        } else if (btn == MENU_ACT_PREV) {
            trim_adjust(-1);
            menu_hold_repeat_arm(btn);
            trim_redraw();
        } else if (btn == MENU_ACT_OK) {
            handset_item_t *item = &trim_items[trim_state.line_index];
            EEPROM_update(trim_eeprom_addr(item), (byte)*item->value);
            trim_state.edit_mode = 0;
            menu_hold_repeat_reset();
            menu_blink_reset();
            trim_redraw();
        } else if (btn == MENU_ACT_BACK) {
            trim_state.edit_mode = 0;
            menu_hold_repeat_reset();
            menu_blink_reset();
            trim_redraw();
        }
        return;
    }

    if (btn == MENU_ACT_NEXT) {
        if (trim_state.line_index + 1 < item_count) {
            trim_state.line_index++;
            trim_redraw();
        }
    } else if (btn == MENU_ACT_PREV) {
        if (trim_state.line_index > 0) {
            trim_state.line_index--;
            trim_redraw();
        }
    } else if (btn == MENU_ACT_OK) {
        trim_state.edit_mode = 1;
        menu_blink_reset();
        trim_redraw();
    }
}
