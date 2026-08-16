
/* Define before crsf.c — it includes crsf_debug.c and crsf_dynamic.c */
//#define debug

#include <Arduino.h>
#include <U8x8lib.h>
//#include <LiquidCrystal_I2C.h>
#include <HardwareTimer.h>
#include <Wire.h>
//#include "config.h"
#include "rc/crsf.c"
#include MIXERS_FILE   /* defines doMixing, mixer_labels */
#include "eeprom_f.c"
//#include <U8g2lib.h>
#include <U8x8lib.h>
#include "rc/crsf_dynamic.h"
#include "rc/crsf_debug.h"
#include "oled_text.h"
#include "oled_text.c"
#include "handset_menu.h"
#include "handset_menu.c"

//U8G2_SSD1315_128X64_NONAME_1_HW_I2C oled(U8G2_R0);
#ifdef BOARD_HAS_SSD1315_OLED
    U8X8_SSD1315_128X64_NONAME_HW_I2C oled(U8X8_PIN_NONE, PB6, PB7);
#else
    U8X8_SH1106_128X64_NONAME_HW_I2C oled(U8X8_PIN_NONE, PB6, PB7);
#endif

//#include "CRSF_CMD.h"
//#include "button_handler.c"
//#include " oled.c"
//#include "PPM.c"

//special mode where splash screen is being shown constantly - for taking pictures
//#define showoff

//SFW model names to be used in school
//#define SCHOOL

//debug
//#define debug
int  oled_cycle_number = 0;
int scroll_line_pos = 0;
int main_menu_pos = 0;
const char* main_message[34] = {"P","r","e","s","s"," ","O","K"," ","t","o"," ","e","n","t","e","r"," ","m","a","i","n"," ","s","e","t","t","i","n","g","s","!"," "};
const char* ELRS_message[34] = {"P","r","e","s","s"," ","O","K"," ","t","o"," ","e","n","t","e","r"," ","E","L","R","S"," ","s","e","t","t","i","n","g","s","!"," "};
/*
Code is provided as is - no warranty */

//Inputs outputs

HardwareSerial Serial2(USART2);

bool armed = false;
bool use_buzzer = false;
bool use_leds = false;

int control_protocol = 0;

bool usb_data = 0;

int mixer_selected;
unsigned long previousMillis = 0;
unsigned long oledMillis = 0;

//#define I2CEEPROM_ADDR 0x57
//const uint8_t I2CEEPROM_ADDR = 0x50;

uint8_t ELRS_TX_Power;
uint8_t ELRS_Pkt_rate;
uint8_t ELRS_Tlm_ratio;

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

bool oled_en = true;
bool command_to_send = false;

bool menu_initialized = false;
bool param_discovery_started = false;
bool elrs_param_load_requested = false;
bool elrs_menu_pending = false;
unsigned long elrs_protocol_error_until = 0;

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

ENUM_BUTTON getButtonV6();

// Dynamic parameter manager is defined in crsf.c
extern dynamic_param_manager_t param_manager;
dynamic_menu_state_t menu_state = {0};

void draw_dynamic_menu(void);

static const char *rx_wifi_block_reason(void) {
    if (LinkStatistics.uplink_Link_quality < 5) {
        return "No link";
    }
    if (digitalRead(AUX1)) {
        return "Disarm!";
    }
    return NULL;
}

// Define max capacity for your handset
#define MAX_PARAMETERS 100
//int currentMenuPos = 0;
//int menuSize;
bool isInLowerLevel = false;
int tempVal;

uint8_t vtx_power;
uint8_t vtx_channel;
uint8_t vtx_band;
uint8_t vtx_pit;

#define bat_volt_div_ratio 5.848

float rx_voltage = 0.0;

#define ELRS_LUA_COMMAND_REQUEST 0
#define ELRS_LUA_COMMAND_PACKET_RATE 1
#define ELRS_LUA_COMMAND_TELEM_RATIO 2
#define ELRS_LUA_COMMAND_SWITCH_MODE 3
#define ELRS_LUA_COMMAND_MODEL_MATCH 4
#define ELRS_LUA_COMMAND_TX_POWER_SET 5
#define ELRS_LUA_COMMAND_MAX_TX_POWER 6
#define ELRS_LUA_COMMAND_DYNAMIC_POWER 7
#define ELRS_LUA_COMMAND_VTX_ADMIN 8
#define ELRS_LUA_COMMAND_VTX_BAND 9
#define ELRS_LUA_COMMAND_VTX_CH 10 //ok
#define ELRS_LUA_COMMAND_VTX_PWR_LVL 11 //ok
#define ELRS_LUA_COMMAND_VTX_PIT_MODE 12
#define ELRS_LUA_COMMAND_SEND_VTX 13
//#define ELRS_LUA_COMMAND_WIFI_CONN 14
#define ELRS_LUA_COMMAND_ENABLE_WIFI 15 //ok
#define ELRS_LUA_COMMAND_EN_RX_WIFI 16  //ok
#define ELRS_LUA_COMMAND_BIND 17

/*
//#define HID_COMMAND_PROTOCOL 20//
#define HID_COMMAND_MODEL_ID 21
#define HID_COMMAND_TX_CHANNEL 22
#define HID_COMMAND_TX_PWR 23
#define HID_COMMAND_RESET 24
#define HID_COMMAND_BEEP 25
*/

#define EEPROM_USB_DAT_ADDR 0
#define EEPROM_PROTOCOL_ADDR 5
#define EEPROM_ELRS_PKT_R_ADDR 6
#define EEPROM_ELRS_TX_P_ADDR 7
#define EEPROM_VTX_B_ADDR 8
#define EEPROM_VTX_CH_ADDR 9
#define EEPROM_VTX_P_ADDR 10
#define EEPROM_ELRS_TLM_R_ADDR 11
#define EEPROM_MIXER_ADDR 12
#define EEPROM_VTX_PIT_ADDR 13
#define EEPROM_BUZZER_ADDR 14
#define EEPROM_LEDS_ADDR 15

TIM_TypeDef *Instance_CRSF_TIM = CRSF_TIM_DEF;
HardwareTimer *CRSF_TIM = new HardwareTimer(Instance_CRSF_TIM);

//HardwareSerial Serial3(USART3);
//HardwareSerial Serial2(USART2);

//#pragma once
//#include "PPM.c"

void handle_menu_navigation(ENUM_BUTTON btn) {
    dynamic_field_t *visible_fields[20];
    uint8_t field_count = dynamic_get_visible_fields(
        &param_manager,
        menu_state.current_folder,
        visible_fields,
        20
    );

    if (field_count == 0) {
        return;
    }

    if (btn == NEXT) {
        if (menu_state.line_index + 1 < field_count) {
            menu_state.line_index++;
            if (menu_state.line_index >= menu_state.page_offset + 8) {
                menu_state.page_offset = menu_state.line_index - 7;
            }
            oled.clearDisplay();
            draw_dynamic_menu();
        }
    } else if (btn == PREV) {
        if (menu_state.line_index > 0) {
            menu_state.line_index--;
            if (menu_state.line_index < menu_state.page_offset) {
                menu_state.page_offset = menu_state.line_index;
            }
            oled.clearDisplay();
            draw_dynamic_menu();
        }
    } else if (btn == OK) {
        menu_state.selected_field = dynamic_get_field_in_folder(
            &param_manager,
            menu_state.current_folder,
            menu_state.line_index
        );

        dynamic_field_t *field = menu_state.selected_field;
        if (!field) {
            return;
        }

        if (field->type == CRSF_FOLDER) {
            menu_state.current_folder = field->id;
            menu_state.line_index = 0;
            menu_state.page_offset = 0;
            oled.clearDisplay();
            draw_dynamic_menu();
        } else if (field->type == CRSF_COMMAND) {
            if (field->value == 2) {
                return;
            }
            if (field->value == 1 && dynamic_command_popup_active()) {
                return;
            }
            uint8_t cmd_byte = dynamic_param_command_tx_byte(field);
            if (cmd_byte != 0) {
                if (strncmp(field->name, "Enable Rx WiFi", sizeof(field->name)) == 0) {
                    const char *block = rx_wifi_block_reason();
                    if (block) {
                        dynamic_command_popup_show_notice(block, 3000);
                        oled.clearDisplay();
                        draw_dynamic_menu();
                        return;
                    }
                }
                dynamic_param_send_value(ADDR_MODULE, field);
                oled.clearDisplay();
                draw_dynamic_menu();
            }
        } else if (!field->grey && field->type <= CRSF_TEXT_SELECTION) {
            menu_state.edit_mode = 1;
            menu_state.edit_value = field->value;
            menu_blink_reset();
            oled.clearDisplay();
            draw_dynamic_menu();
        }
    } else if (btn == BACK) {
        if (dynamic_command_popup_active()) {
            dynamic_field_t *cmd_field = dynamic_get_field_by_id(
                &param_manager, dynamic_command_popup_field_id());
            if (cmd_field && cmd_field->value == 2) {
                dynamic_param_send_command(ADDR_MODULE, cmd_field->id, CRSF_CMD_STATUS_CANCEL);
                dynamic_command_popup_clear();
                oled.clearDisplay();
                draw_dynamic_menu();
                return;
            }
        }
        if (menu_state.current_folder != DYNAMIC_ROOT_FOLDER) {
            dynamic_field_t *folder = dynamic_get_field_by_id(&param_manager, menu_state.current_folder);
            menu_state.current_folder = folder ? folder->parent : DYNAMIC_ROOT_FOLDER;
            menu_state.line_index = 0;
            menu_state.page_offset = 0;
            oled.clearDisplay();
            draw_dynamic_menu();
        } else {
            isInLowerLevel = false;
            main_menu_pos = SCREEN_MAIN;
            oled.clearDisplay();
        }
    }
}

// In edit mode, handle increment/decrement:
static void elrs_menu_enter(void) {
    isInLowerLevel = true;
    menu_state.current_folder = DYNAMIC_ROOT_FOLDER;
    menu_state.line_index = 0;
    menu_state.page_offset = 0;
    menu_state.edit_mode = 0;
    menu_blink_reset();
    oled.clearDisplay();
    draw_dynamic_menu();
}

static void elrs_menu_start_load(void) {
    dynamic_param_reset_for_reload(&param_manager);
    dynamic_command_popup_clear();
    menu_state.current_folder = DYNAMIC_ROOT_FOLDER;
    menu_state.line_index = 0;
    menu_state.page_offset = 0;
    menu_state.edit_mode = 0;
    menu_state.selected_field = NULL;
    elrs_param_load_requested = true;
    elrs_menu_pending = true;
    param_discovery_started = true;
    CRSF_broadcast_ping();
    oled.clearDisplay();
}

void handle_value_edit(ENUM_BUTTON btn) {
    if (!menu_state.selected_field) return;

    if (btn == NEXT) {
        dynamic_field_increment(menu_state.selected_field, 1);
        menu_hold_repeat_arm((uint8_t)btn);
        oled.clearDisplay();
        draw_dynamic_menu();
    } else if (btn == PREV) {
        dynamic_field_increment(menu_state.selected_field, -1);
        menu_hold_repeat_arm((uint8_t)btn);
        oled.clearDisplay();
        draw_dynamic_menu();
    } else if (btn == OK) {
        dynamic_param_send_value(ADDR_MODULE, menu_state.selected_field);
        menu_state.edit_mode = 0;
        menu_hold_repeat_reset();
        menu_blink_reset();
        oled.clearDisplay();
        draw_dynamic_menu();
    } else if (btn == BACK) {
        menu_state.edit_mode = 0;
        menu_hold_repeat_reset();
        menu_blink_reset();
        oled.clearDisplay();
        draw_dynamic_menu();
    }
}

static void ui_edit_hold_repeat_tick(unsigned long now) {
    ENUM_BUTTON btn = getButtonV6();
    int8_t delta = 0;

    if (main_menu_pos == SCREEN_ELRS && isInLowerLevel && menu_initialized &&
        menu_state.edit_mode && menu_state.selected_field) {
        if (btn == NEXT || btn == PREV) {
            delta = menu_hold_repeat_tick(now, (uint8_t)btn);
            if (delta != 0) {
                dynamic_field_increment(menu_state.selected_field, delta);
                oled.clearDisplay();
                draw_dynamic_menu();
            }
        } else if (btn == NONE) {
            menu_hold_repeat_reset();
        }
        return;
    }

    if (main_menu_pos == SCREEN_TRIM && isInLowerLevel && handset_trim_edit_mode()) {
        if (btn == NEXT || btn == PREV) {
            delta = menu_hold_repeat_tick(now, (uint8_t)btn);
            if (delta != 0) {
                handset_trim_edit_step(delta);
            }
        } else if (btn == NONE) {
            menu_hold_repeat_reset();
        }
        return;
    }

    if (main_menu_pos == SCREEN_MAIN && isInLowerLevel && handset_main_edit_mode()) {
        if (btn == NEXT || btn == PREV) {
            delta = menu_hold_repeat_tick(now, (uint8_t)btn);
            if (delta != 0) {
                handset_main_edit_step(delta);
            }
        } else if (btn == NONE) {
            menu_hold_repeat_reset();
        }
        return;
    }

    menu_hold_repeat_reset();
}

void process_ui_buttons() {
    static ENUM_BUTTON last_btn = NONE;
    ENUM_BUTTON btn = getButtonV6();
    if (btn == NONE) {
        last_btn = NONE;
        return;
    }
    if (btn == last_btn) {
        return;
    }
    last_btn = btn;

    if (main_menu_pos == SCREEN_ELRS && isInLowerLevel && menu_initialized) {
        if (menu_state.edit_mode) {
            handle_value_edit(btn);
        } else {
            handle_menu_navigation(btn);
        }
        return;
    }

    if (main_menu_pos == SCREEN_TRIM && isInLowerLevel) {
        if (btn == BACK && !handset_trim_edit_mode()) {
            isInLowerLevel = false;
            main_menu_pos = SCREEN_MAIN;
            oled.clearDisplay();
        } else {
            handset_trim_handle_btn((uint8_t)btn);
        }
        return;
    }

    if (main_menu_pos == SCREEN_MAIN && isInLowerLevel) {
        if (btn == BACK && !handset_main_edit_mode()) {
            isInLowerLevel = false;
            oled.clearDisplay();
        } else {
            handset_main_handle_btn((uint8_t)btn);
        }
        return;
    }

    if (btn == NEXT) {
        main_menu_pos++;
        if (main_menu_pos >= SCREEN_COUNT) {
            main_menu_pos = 0;
        }
        oled.clearDisplay();
    } else if (btn == PREV) {
        if (main_menu_pos > 0) {
            main_menu_pos--;
        }
        oled.clearDisplay();
    } else if (btn == OK && main_menu_pos == SCREEN_MAIN) {
        isInLowerLevel = true;
        handset_main_enter();
        oled.clearDisplay();
        handset_main_draw();
    } else if (btn == OK && main_menu_pos == SCREEN_ELRS) {
        if (control_protocol != HANDSET_PROTOCOL_ELRS) {
            elrs_protocol_error_until = millis() + 3000;
            oled.clearDisplay();
        } else {
            elrs_menu_start_load();
        }
    } else if (btn == OK && main_menu_pos == SCREEN_TRIM) {
        isInLowerLevel = true;
        handset_trim_enter();
        oled.clearDisplay();
        handset_trim_draw();
    } else if (btn == BACK && isInLowerLevel) {
        isInLowerLevel = false;
        main_menu_pos = SCREEN_MAIN;
        oled.clearDisplay();
    } else if (btn == BACK && !isInLowerLevel && main_menu_pos != SCREEN_MAIN) {
        main_menu_pos = SCREEN_MAIN;
        oled.clearDisplay();
    }
}

void handle_buttons(ENUM_BUTTON btn) {
    (void)btn;
}

void handle_edit_mode(ENUM_BUTTON btn) {
    (void)btn;
}


// writes a byte of data in memory location addr
//#define debug
/*void EEPROM_write(unsigned int addr, byte eeprom_data)  {
  #ifdef debug
  SerialUSB.print("wr addr"); SerialUSB.println(addr);
  #endif
 Wire.beginTransmission(I2CEEPROM_ADDR);
    Wire.write(addr >> 8);
    Wire.write(addr & 0xFF);
    Wire.write(eeprom_data);
    Wire.endTransmission();
    delay(10);

}

// reads a byte of data from memory location addr
byte EEPROM_read(unsigned int addr)  {
  Wire.beginTransmission(I2CEEPROM_ADDR);
    Wire.write(addr >> 8);
    Wire.write(addr & 0xFF);
    Wire.endTransmission();
    Wire.requestFrom(I2CEEPROM_ADDR, (byte)1);
    byte data = 0;
    if (Wire.available())
    {
        data = Wire.read();
    }
    return data;
}

void EEPROM_update(unsigned int addr, byte to_write) { //to save write cycles of eeprom
  byte wrtitten = EEPROM_read(addr);
  if (to_write == wrtitten) {
    //do nothing, just skip
  } else {
    EEPROM_write(addr, to_write);
  }

}


void reset_eeprom() {

  EEPROM_write(0, 0); //nrf24 channel
  EEPROM_write(1, 127); //trim 1
  EEPROM_write(2, 127); //trim 2
  EEPROM_write(3, 127); //trim 3
  EEPROM_write(4, 127); //trim 4
  EEPROM_write(5, 0); //elrs RF
  EEPROM_write(6, 0); //elrs output power
  EEPROM_write(7, 0); //elrs RF
  EEPROM_write(8, 0); //elrs output power
  EEPROM_write(9, 0); //elrs RF
  EEPROM_write(10, 0); //elrs output power
  EEPROM_write(11, 0); //elrs RF
  EEPROM_write(12, 0); //elrs output power
  EEPROM_write(13, 0); 
}*/


void showSplashScreen() {
  oled.setCursor(0, 0);
  oled.print(chop_chars("HACK TX v8", OLED_ROW_CHARS));
  oled.setCursor(0, 1);
  oled.print(chop_chars("BY ARTUR KUC", OLED_ROW_CHARS));
}

ENUM_BUTTON getButtonV6() {
  if(!digitalRead(BTN_BACK)) return BACK;
  if(!digitalRead(BTN_NEXT)) return NEXT;
  if(!digitalRead(BTN_PREV)) return PREV;
  if(!digitalRead(BTN_OK)) return OK;

  return NONE;
}

void draw_dynamic_menu() {
    dynamic_field_t *visible_fields[20];
    uint8_t field_count = dynamic_get_visible_fields(
        &param_manager,
        menu_state.current_folder,
        visible_fields,
        20
    );

    if (field_count == 0) {
        oled.setCursor(0, 0);
        oled.print(chop_chars("No parameters", OLED_ROW_CHARS));
        return;
    }

    if (menu_state.line_index >= field_count) {
        menu_state.line_index = field_count - 1;
    }

    uint8_t display_lines = 8;
    uint8_t start_idx = menu_state.page_offset;
    const uint8_t has_scroll_up = (start_idx > 0) ? 1 : 0;
    const uint8_t has_scroll_down = ((start_idx + display_lines) < field_count) ? 1 : 0;
    const char *cmd_status = NULL;

    for (uint8_t i = 0; i < display_lines && (start_idx + i) < field_count; i++) {
        uint8_t field_idx = start_idx + i;
        dynamic_field_t *field = visible_fields[field_idx];
        char value_str[32];
        char line[36];
        dynamic_field_display_value(field, value_str, sizeof(value_str));

        uint8_t max_chars = OLED_ROW_CHARS;
        if ((i == 0 && has_scroll_up) || (i == 7 && has_scroll_down)) {
            max_chars = OLED_ROW_CHARS - 1;
        }

        if (menu_line_inverted(field_idx == menu_state.line_index, menu_state.edit_mode)) {
            oled.setInverseFont(1);
        }

        if (strncmp(field->name, "Packet Rate", sizeof(field->name)) == 0) {
            const uint8_t left_chars = (max_chars > OLED_PKT_STATS_CHARS)
                ? (uint8_t)(max_chars - OLED_PKT_STATS_CHARS) : 0;
           /* char stats[8];
            uint16_t good = elrs_info.good_pkts;
            if (good > 999) {
                good = 999;
            }
            snprintf(stats, sizeof(stats), "%03u/%03u", elrs_info.bad_pkts, good);
          */
            oled.setCursor(0, i);
            oled.print(chop_chars(value_str, left_chars));
        } else if (field->type == CRSF_COMMAND) {
            snprintf(line, sizeof(line), "[%s]", field->name);
            oled.setCursor(0, i);
            oled.print(chop_chars(line, max_chars));
            if (field->value == 2 && field->options_blob[0]) {
                cmd_status = field->options_blob;
            } else if (field->value == 3) {
                cmd_status = "OK=confirm";
            }
        } else {
            if (dynamic_field_compact_display(field)) {
                strncpy(line, value_str, sizeof(line) - 1);
            } else if (field->type == CRSF_FOLDER) {
                snprintf(line, sizeof(line), ">%s", field->name);
            } else if (field->type == CRSF_TEXT_SELECTION) {
                snprintf(line, sizeof(line), "%s %s", field->name, value_str);
            } else {
                strncpy(line, field->name, sizeof(line) - 1);
            }
            line[sizeof(line) - 1] = 0;

            oled.setCursor(0, i);
            oled.print(chop_chars(line, max_chars));
        }

        oled.setInverseFont(0);
    }

    if (start_idx > 0) {
        oled.setCursor(127, 0);
        oled.print("^");
    }
    if ((start_idx + display_lines) < field_count) {
        oled.setCursor(127, 7);
        oled.print("v");
    }
    if (!cmd_status) {
        cmd_status = dynamic_command_popup_banner();
    }
    if (cmd_status) {
        uint8_t status_row = (field_count <= 6 && !has_scroll_down) ? 7 : 6;
        oled.setCursor(0, status_row);
        oled.print(chop_chars(cmd_status, OLED_ROW_CHARS - (status_row == 7 && has_scroll_down)));
    }
}

void show_data() {

    if (main_menu_pos == SCREEN_MAIN && isInLowerLevel == false) {
      oled.setCursor(0, 0);
      oled.print("Main:");
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "Y%4d T%4d", yaw, throttle);
      oled.setCursor(0, 2);
      oled.print(chop_chars(buffer, OLED_ROW_CHARS));
      snprintf(buffer, sizeof(buffer), "R%4d P%4d", roll, pitch);
      oled.setCursor(0, 3);
      oled.print(chop_chars(buffer, OLED_ROW_CHARS));

      String voltage = String((analogRead(battery_in) * ((3.3 / 4096) * bat_volt_div_ratio)), 2);
      snprintf(buffer, sizeof(buffer), "Bat:%sV", voltage.c_str());
      oled.setCursor(0, 4);
      oled.print(chop_chars(buffer, OLED_ROW_CHARS));
      
      if ( oled_cycle_number > 3) {
         oled.setCursor(0, 1);
        if (scroll_line_pos > 33) {
          scroll_line_pos = 0;
        } else {
          scroll_line_pos++;
        }

        for (int i = 0; i<20; i++) {
          int j = i + scroll_line_pos;
          if (j > 33) {
             oled.print(main_message[j - 34]);
          } else {
             oled.print(main_message[j-1]);
          }
          
           oled.setCursor(i, 1);
        }
         oled_cycle_number = 0;
      } else {
         oled_cycle_number++;
      }
      
      snprintf(buffer, sizeof(buffer), "M%d", mixer_selected);
      oled.setCursor(10, 0);
      oled.print(chop_chars(buffer, OLED_ROW_CHARS - 1));

      //snprintf(buffer, sizeof(buffer), "B:%s L:%s",
            //   use_buzzer ? "ON" : "OFF", use_leds ? "ON" : "OFF");
     // oled.setCursor(0, 4);
     // oled.print(chop_chars(buffer, OLED_ROW_CHARS));
      //oled.setCursor(0, 5);
     // oled.print(chop_chars("OK settings", OLED_ROW_CHARS));


    } else if (main_menu_pos == SCREEN_ELRS && isInLowerLevel == false) {
      if ((int32_t)(elrs_protocol_error_until - millis()) > 0) {
        oled.setCursor(0, 0);
        oled.print(chop_chars("Error:", OLED_ROW_CHARS));
        oled.setCursor(0, 2);
        oled.print(chop_chars("Set protocol", OLED_ROW_CHARS));
        oled.setCursor(0, 3);
        oled.print(chop_chars("to ELRS", OLED_ROW_CHARS));
        return;
      }

      if (elrs_menu_pending && !param_manager.params_loaded) {
        oled.setCursor(0, 0);
        oled.print(chop_chars("ELRS:", OLED_ROW_CHARS));
        oled.setCursor(0, 2);
        oled.print(chop_chars("Loading params", OLED_ROW_CHARS));
        char load_line[16];
        snprintf(load_line, sizeof(load_line), "%u/%u",
                 param_manager.fields_loaded, param_manager.fields_count);
        oled.setCursor(0, 3);
        oled.print(chop_chars(load_line, OLED_ROW_CHARS));
        return;
      }

      oled.setCursor(0, 0);
      oled.print("ELRS:");
      if ( oled_cycle_number > 3) {
         oled.setCursor(0, 1);
        if (scroll_line_pos > 33) {
          scroll_line_pos = 0;
        } else {
          scroll_line_pos++;
        }

        for (int i = 0; i<20; i++) {
          int j = i + scroll_line_pos;
          if (j > 33) {
             oled.print(ELRS_message[j - 34]);
          } else {
             oled.print(ELRS_message[j-1]);
          }
          
           oled.setCursor(i, 1);
        }
         oled_cycle_number = 0;
      } else {
         oled_cycle_number++;
      }

      char buffer[32];
      snprintf(buffer, sizeof(buffer), "RSSI:%d", LinkStatistics.uplink_RSSI_1);
      oled.setCursor(0, 2);
      oled.print(chop_chars(buffer, OLED_ROW_CHARS));

      rx_voltage = batteryVoltage.voltage / 10.0;
      String rx_bat = String(rx_voltage, 1);
      snprintf(buffer, sizeof(buffer), "Bat:%sV", rx_bat.c_str());
      oled.setCursor(0, 4);
      oled.print(chop_chars(buffer, OLED_ROW_CHARS));

      snprintf(buffer, sizeof(buffer), "LQ:%d", LinkStatistics.uplink_Link_quality);
      oled.setCursor(0, 3);
      oled.print(chop_chars(buffer, OLED_ROW_CHARS));

    } else if (main_menu_pos == SCREEN_TRIM && isInLowerLevel == false) {
      oled.setCursor(0, 0);
      oled.print("Trim:");
      char buffer[32];
      snprintf(buffer, sizeof(buffer), "Y%3d T%3d", yaw_fine, throttle_fine);
      oled.setCursor(0, 2);
      oled.print(chop_chars(buffer, OLED_ROW_CHARS));
      snprintf(buffer, sizeof(buffer), "R%3d P%3d", roll_fine, pitch_fine);
      oled.setCursor(0, 3);
      oled.print(chop_chars(buffer, OLED_ROW_CHARS));
      oled.setCursor(0, 1);
      oled.print(chop_chars("OK to edit", OLED_ROW_CHARS));

    }
}


uint8_t find_parameter_by_name(const char* name) {
  dynamic_field_t *field = dynamic_find_field_by_name(&param_manager, name);
  return field ? field->id : 0xFF;
}

void send_param_by_name(const char* param_name, int32_t value) {
  dynamic_field_t *field = dynamic_find_field_by_name(&param_manager, param_name);
  if (field) {
    if (field->max != field->min || field->max != 0) {
      value = constrain(value, field->min, field->max);
    }
    field->value = value;
    dynamic_param_send_value(ADDR_MODULE, field);
  }
}

void CRSF_SEND_2400() {
  doMixing[mixer_selected]();
  crsfPreparePacket(crsfPacket, rcChannels);
  ELRS_Serial_2400.write(crsfPacket, CRSF_PACKET_SIZE);
}
#ifdef BOARD_HAS_ESP_MODULE 
void CRSF_SEND_ESP() {
  doMixing[mixer_selected]();
  crsfPreparePacket(crsfPacket, rcChannels);
  ESP_Serial.write(crsfPacket, CRSF_PACKET_SIZE);
}
#endif
void crsf_rc_timer_pause(void) {
  CRSF_TIM->pause();
}

void crsf_rc_timer_resume(void) {
  CRSF_TIM->resume();
} 

void void_null() {
  //do nothing
}

void setup()  {

  PIN_MODE_OUTPUT(Module_power_2400);
  PIN_MODE_OUTPUT(Module_power_ESP);
  PIN_WRITE(Module_power_2400, LOW);
  PIN_WRITE(Module_power_ESP, LOW);
  
  //ELRS_Serial_2400.setTx(PA9);
  //ELRS_Serial_2400.setRx(PA10);
  ELRS_Serial_2400.begin(CRSF_baudrate);  //UART1
#ifdef BOARD_HAS_ESP_MODULE
  ESP_Serial.begin(CRSF_baudrate);
#endif
  //debug serial over usb
  #ifdef debug
  SerialUSB.begin(115200);
  //delay(10000);
  SerialUSB.println("starting debug");
  #endif
  PIN_MODE_INPUT(joystick_Y);  
  PIN_MODE_INPUT(joystick_T);  
  PIN_MODE_INPUT(joystick_R);  
  PIN_MODE_INPUT(joystick_P);  
  
  PIN_MODE_INPUT(battery_in);  //battery voltage divider

  PIN_MODE_PULLDOWN(AUX1); //AUX1
  PIN_MODE_INPUT(AUX2);  //AUX2
  PIN_MODE_PULLDOWN(AUX3); //AUX3
  PIN_MODE_PULLDOWN(AUX4);  //AUX4
 
  Wire.setSCL(PB6);
  Wire.setSDA(PB7);
 
  Wire.begin();
  Wire.setClock(400000);
  delay(100);
  oled.begin();
   delay(100);
 
  ELRS_Pkt_rate = EEPROM_read(EEPROM_ELRS_PKT_R_ADDR);
  ELRS_TX_Power = EEPROM_read(EEPROM_ELRS_TX_P_ADDR);
  vtx_band = EEPROM_read(EEPROM_VTX_B_ADDR);
  vtx_channel = EEPROM_read(EEPROM_VTX_CH_ADDR);
  vtx_power = EEPROM_read(EEPROM_VTX_P_ADDR);
  ELRS_Tlm_ratio = EEPROM_read(EEPROM_ELRS_TLM_R_ADDR);
  
  vtx_pit = EEPROM_read(EEPROM_VTX_PIT_ADDR);
   
  mixer_selected = EEPROM_read(EEPROM_MIXER_ADDR);
  mixer_selected = constrain(mixer_selected, 0, 4);
  control_protocol = EEPROM_read(EEPROM_PROTOCOL_ADDR);
  control_protocol = constrain(control_protocol, 0, 2);
  use_buzzer = EEPROM_read(EEPROM_BUZZER_ADDR);
  use_buzzer = (use_buzzer != 0);
  use_leds = EEPROM_read(EEPROM_LEDS_ADDR);
  use_leds = (use_leds != 0);

  throttle_fine = EEPROM_read((32 + mixer_selected*4));
  yaw_fine = EEPROM_read((33 + mixer_selected*4));
  pitch_fine = EEPROM_read((34 + mixer_selected*4));
  roll_fine = EEPROM_read((35 + mixer_selected*4));

  PIN_MODE_PULLUP(BTN_NEXT);
  PIN_MODE_PULLUP(BTN_PREV);
  PIN_MODE_PULLUP(BTN_BACK);
  PIN_MODE_PULLUP(BTN_OK);
  int protocol_selected = 0;
 
  analogReadResolution(12); //set ADC to 12-bit res
  
  oled.setFont(u8x8_font_5x7_f);

  showSplashScreen();
  
  #ifdef debug
  SerialUSB.print("mixer selected boot:"); SerialUSB.println(mixer_selected);
  #endif
  
  #ifdef debug
  SerialUSB.print("thr_fine boot:"); SerialUSB.println(throttle_fine);
  SerialUSB.print("yaw_fine boot:"); SerialUSB.println(yaw_fine);
  SerialUSB.print("rll_fine boot:"); SerialUSB.println(roll_fine);
  SerialUSB.print("pit_fine boot:"); SerialUSB.println(pitch_fine);
  #endif
  
  if (control_protocol == HANDSET_PROTOCOL_ELRS) {
    PIN_WRITE(Module_power_2400, HIGH);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(250, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(CRSF_SEND_2400);
    CRSF_TIM->resume();

   
  } else if (control_protocol == HANDSET_PROTOCOL_ESP) {
    #ifdef BOARD_HAS_ESP_MODULE 
    PIN_WRITE(Module_power_ESP, HIGH);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(250, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(CRSF_SEND_ESP);
    CRSF_TIM->resume();
     #endif
  }
 
 /* for (int i = 0; i < CRSF_MAX_CHANNEL; i++) {

    if (i == 1) {
      rcChannels[i] = RC_CHANNEL_MIN;
    } else {
      rcChannels[i] = RC_CHANNEL_MID;
    }

  }*/
  crsf_param_boot_ready_at = millis() + CRSF_PARAM_BOOT_DELAY_MS;
  
  CRSF_broadcast_ping();
  dynamic_param_init(&param_manager, MAX_PARAMETERS);

#ifdef debug
  SerialUSB.println("CRSF param USB debug ON (115200)");
  SerialUSB.println("Status every 2s while loading. Press 'd' after load for dump.");
#endif

  menu_state.current_folder = DYNAMIC_ROOT_FOLDER;
  menu_state.line_index = 0;
  menu_state.page_offset = 0;
  menu_state.edit_mode = 0;
  //hideSplashScreen(); //finish boot sequance
}

void loop() {
  unsigned long currentMillis = millis();

  serialtelemetryevent_2400();

  if (elrs_param_load_requested && param_manager.fields_count > 0 &&
      crsf_param_discovery_allowed()) {
    param_discovery_started = true;
    CRSF_discover_parameters(ADDR_MODULE);
  }

  if (elrs_menu_pending && param_manager.params_loaded) {
    elrs_menu_pending = false;
    elrs_menu_enter();
  }

  if ((currentMillis - oledMillis) > 100) {
    if (!crsf_param_discovery_allowed()) {
      int32_t secs_left = (int32_t)(crsf_param_boot_ready_at - currentMillis);
      if (secs_left < 0) {
        secs_left = 0;
      }
      //oled.clear();
      oled.setCursor(0, 3);
      oled.print(chop_chars("BOOTING", OLED_ROW_CHARS));
      char boot_line[16];
      snprintf(boot_line, sizeof(boot_line), "%lus", (unsigned long)(secs_left / 1000));
      oled.setCursor(0, 4);
      oled.print(chop_chars(boot_line, OLED_ROW_CHARS));
    } else if (!menu_initialized) {
      menu_initialized = true;
      oled.clearDisplay();
    }

    if (menu_initialized && isInLowerLevel) {
      uint8_t blink_redraw = 0;
      uint8_t menu_redraw = 0;
      if (main_menu_pos == SCREEN_ELRS && menu_state.edit_mode) {
        blink_redraw = menu_blink_tick(currentMillis);
      } else if (main_menu_pos == SCREEN_TRIM && handset_trim_edit_mode()) {
        blink_redraw = menu_blink_tick(currentMillis);
        handset_trim_update_channel(currentMillis);
      } else if (main_menu_pos == SCREEN_MAIN && handset_main_edit_mode()) {
        blink_redraw = menu_blink_tick(currentMillis);
      }
      if (main_menu_pos == SCREEN_ELRS) {
        if (dynamic_command_popup_tick(currentMillis, ADDR_MODULE)) {
          menu_redraw = 1;
        }
        if (dynamic_menu_take_dirty() || dynamic_command_popup_banner_take_edge()) {
          menu_redraw = 1;
        }
      }
      if (blink_redraw || menu_redraw) {
        oled.clearDisplay();
        if (main_menu_pos == SCREEN_ELRS) {
          draw_dynamic_menu();
        } else if (main_menu_pos == SCREEN_TRIM) {
          handset_trim_draw();
        } else if (main_menu_pos == SCREEN_MAIN) {
          handset_main_draw();
        }
      }
    } else if (menu_initialized) {
      show_data();
    }


    oledMillis = currentMillis;
  }

  process_ui_buttons();
  ui_edit_hold_repeat_tick(currentMillis);

 // if (use_buzzer == true && armed != digitalRead(AUX1)) {
   // armed = digitalRead(AUX1);
  //  buildElrsPacket(crsfCmdPacket, HID_COMMAND_BEEP, 1);
//  }

  #ifdef debug
  static unsigned long dbgMillis = 0;
  if (!param_manager.params_loaded && param_manager.fields_count > 0 &&
      (currentMillis - dbgMillis) > 2000) {
    crsf_dbg_print_param_status();
    dbgMillis = currentMillis;
  }

  if (SerialUSB.available()) {
    char received = SerialUSB.read();
    if (received == 'd') {
      crsf_dbg_print_param_status();
    }
  }

  /*if (param_manager.params_loaded) {
    SerialUSB.print("yaw:");
    SerialUSB.print(analogRead(joystick_Y));
    SerialUSB.print(" thr:");
    SerialUSB.print(analogRead(joystick_T));
    SerialUSB.print(" AUX1:");
    SerialUSB.print(digitalRead(AUX1));
    SerialUSB.print(" VBAT:");
    String voltage = String((analogRead(battery_in) * ((3.3 / 4096) * bat_volt_div_ratio)), 2);
    SerialUSB.print(voltage);
    SerialUSB.println("V");
  }*/
  #endif
  delay(50);
}
