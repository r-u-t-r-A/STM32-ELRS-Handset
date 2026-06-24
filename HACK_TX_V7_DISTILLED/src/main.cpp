
#include <Arduino.h>
#include "display.h"
#include <HardwareTimer.h>
#include <Wire.h>
#include "config.h"
#include "crsf.c"
#include "mixers.c"
#include "eeprom_f.c"
//#include "CRSF_CMD.h"
//#include "button_handler.c"
//#include "lcd.c"
//#include "PPM.c"

//special mode where splash screen is being shown constantly - for taking pictures
//#define showoff

//SFW model names to be used in school
//#define SCHOOL

//debug
//#define debug
int lcd_cycle_number = 0;
int scroll_line_pos = 0;
int sub_menu_pos = 0;
int main_menu_pos = 0;
const char* main_message[34] = {"P","r","e","s","s"," ","O","K"," ","t","o"," ","e","n","t","e","r"," ","m","a","i","n"," ","s","e","t","t","i","n","g","s","!"," "};
const char* ELRS_message[34] = {"P","r","e","s","s"," ","O","K"," ","t","o"," ","e","n","t","e","r"," ","E","L","R","S"," ","s","e","t","t","i","n","g","s","!"," "};
/*
Code is provided as is - no warranty */

Display lcd;

TIM_TypeDef *Instance_CRSF_TIM = CRSF_TIM_DEF;
HardwareTimer *CRSF_TIM = new HardwareTimer(Instance_CRSF_TIM);

//HardwareSerial Serial3(USART3);
HardwareSerial Serial2(USART2);

const char* main_menu_labels[6]     = {"Data:", "Telemetry:", "", "Trims", "VTX admin", "Miscellaneous"}; // index 2 was "NRF24 settings" (disabled)
const char* main_settings_labels[3] = {"RC protocol", "Mixer", "Buzzer"};
const char* ELRS_settings_labels[6] = {"RF Power", "Packet rate", "TLM ratio", "Enable TX WiFi", "Enable RX WiFi", "Bind receiver"};
//const char* NRF_settings_labels[3]  = {"ID", "Channel", "RF Power"};
const char* TRIMS_labels[4]         = {"Yaw", "Throttle", "Roll", "Pitch"};
const char* VTX_admin_labels[4]     = {"Band", "Channel", "Power level", "PIT mode"};
const char* MISC_settings_labels[2] = {"LEDS", "EEPROM reset"};

STRUCT_MENUPOS main_settings[3];

void showSplashScreen() {
  lcd.setCursor(0, 0);
  lcd.print("HACK_TX DUAL v6");
  lcd.setCursor(0, 1);
  lcd.print("  BY ARTUR KUC");
  delay(1000);
}

void hideSplashScreen() {
  lcd.clear();
  delay(250);
}

void handleNextV6() {
 
  if (isInLowerLevel == true && isInSubMenu == false) {
    sub_menu_pos++;
    if (main_menu_pos == 0) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 2);
    } else if (main_menu_pos == 1) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 5);
    //} else if (main_menu_pos == 2) {
    //  sub_menu_pos = constrain(sub_menu_pos, 0, 2);
    } else if (main_menu_pos == 3) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 3);
    } else if (main_menu_pos == 4) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 3);
    } else if (main_menu_pos == 5) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 1);
    }
  } else if (isInLowerLevel == true && isInSubMenu == true) {
    tempVal++;
    if (main_menu_pos == 0) {
      tempVal = constrain(tempVal, main_settings[sub_menu_pos].minVal, main_settings[sub_menu_pos].maxVal);
    } else if (main_menu_pos == 1) {
      tempVal = constrain(tempVal, ELRS_menu[sub_menu_pos].minVal, ELRS_menu[sub_menu_pos].maxVal);
    //} else if (main_menu_pos == 2) {
    //  tempVal = constrain(tempVal, NRF24_menu[sub_menu_pos].minVal, NRF24_menu[sub_menu_pos].maxVal);
    } else if (main_menu_pos == 3) {
      tempVal = constrain(tempVal, TRIM_menu[sub_menu_pos].minVal, TRIM_menu[sub_menu_pos].maxVal);
    } else if (main_menu_pos == 4) {
      tempVal = constrain(tempVal, vtx_menu[sub_menu_pos].minVal, vtx_menu[sub_menu_pos].maxVal);
    } else if (main_menu_pos == 5) {
      tempVal = constrain(tempVal, misc_menu[sub_menu_pos].minVal, misc_menu[sub_menu_pos].maxVal);
    }
  } else {
    main_menu_pos++;
    if (main_menu_pos == 2) main_menu_pos++; // skip disabled NRF24 menu
    main_menu_pos = constrain(main_menu_pos, 0, 5);
  }
}

void handlePrevV6() {

  if (isInLowerLevel == true && isInSubMenu == false) {
    sub_menu_pos = sub_menu_pos - 1;
    if (main_menu_pos == 0) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 2);
    } else if (main_menu_pos == 1) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 5);
    //} else if (main_menu_pos == 2) {
    //  sub_menu_pos = constrain(sub_menu_pos, 0, 2);
    } else if (main_menu_pos == 3) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 3);
    } else if (main_menu_pos == 4) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 3);
    } else if (main_menu_pos == 5) {
      sub_menu_pos = constrain(sub_menu_pos, 0, 1);
    }
  } else if (isInLowerLevel == true && isInSubMenu == true) {
    tempVal = tempVal - 1;
    if (main_menu_pos == 0) {
      tempVal = constrain(tempVal, main_settings[sub_menu_pos].minVal, main_settings[sub_menu_pos].maxVal);
    } else if (main_menu_pos == 1) {
      tempVal = constrain(tempVal, ELRS_menu[sub_menu_pos].minVal, ELRS_menu[sub_menu_pos].maxVal);
    //} else if (main_menu_pos == 2) {
    //  tempVal = constrain(tempVal, NRF24_menu[sub_menu_pos].minVal, NRF24_menu[sub_menu_pos].maxVal);
    } else if (main_menu_pos == 3) {
      tempVal = constrain(tempVal, TRIM_menu[sub_menu_pos].minVal, TRIM_menu[sub_menu_pos].maxVal);
    } else if (main_menu_pos == 4) {
      tempVal = constrain(tempVal, vtx_menu[sub_menu_pos].minVal, vtx_menu[sub_menu_pos].maxVal);
    } else if (main_menu_pos == 5) {
      tempVal = constrain(tempVal, misc_menu[sub_menu_pos].minVal, misc_menu[sub_menu_pos].maxVal);
    } 
    //tempVal = constrain...
  } else {
    main_menu_pos = main_menu_pos - 1;
    if (main_menu_pos == 2) main_menu_pos--; // skip disabled NRF24 menu
    main_menu_pos = constrain(main_menu_pos, 0, 5);
  }
}

void handleBackV6() {
  
  if (isInLowerLevel == true && isInSubMenu == false) {
    isInLowerLevel = false;
  } else if (isInLowerLevel == true && isInSubMenu == true) {
    isInSubMenu = false;
  } else {
    main_menu_pos = 0;
    sub_menu_pos = 0;
    isInSubMenu = false;
    isInLowerLevel = false;
  }
}

void handleOkV6() {
  
  if (isInLowerLevel == true && isInSubMenu == false) {
    isInSubMenu = true;
    
    if (main_menu_pos == 0) {
      tempVal = main_settings[sub_menu_pos].currentVal;
    } else if (main_menu_pos == 1) {
      tempVal = ELRS_menu[sub_menu_pos].currentVal;
    //} else if (main_menu_pos == 2) {
    //  tempVal = NRF24_menu[sub_menu_pos].currentVal;
    } else if (main_menu_pos == 3) {
      tempVal = TRIM_menu[sub_menu_pos].currentVal;
    } else if (main_menu_pos == 4) {
      tempVal = vtx_menu[sub_menu_pos].currentVal;
    } else if (main_menu_pos == 5) {
      tempVal = misc_menu[sub_menu_pos].currentVal;
    }
  } else if (isInLowerLevel == true && isInSubMenu == true) {
    isInSubMenu = false;
    if (main_menu_pos == 0) {
       main_settings[sub_menu_pos].currentVal = tempVal;
    } else if (main_menu_pos == 1) {
      ELRS_menu[sub_menu_pos].currentVal = tempVal;
    //} else if (main_menu_pos == 2) {
    //  NRF24_menu[sub_menu_pos].currentVal = tempVal;
    } else if (main_menu_pos == 3) {
      TRIM_menu[sub_menu_pos].currentVal = tempVal;
    } else if (main_menu_pos == 4) {
      vtx_menu[sub_menu_pos].currentVal = tempVal;
    } else if (main_menu_pos == 5) {
      misc_menu[sub_menu_pos].currentVal = tempVal;
    }
  } else {
    sub_menu_pos = 0;
    isInLowerLevel = true;
  }
 
}

ENUM_BUTTON getButtonV6() {
  if(!digitalRead(BTN_BACK)) return BACK;
  if(!digitalRead(BTN_NEXT)) return NEXT;
  if(!digitalRead(BTN_PREV)) return PREV;
  if(!digitalRead(BTN_OK)) return OK;

  return NONE;
}

void drawMenuV6() {
  static unsigned long lastRead = 0;
  static ENUM_BUTTON lastPressedButton = OK;
  static unsigned int isPressedSince = 0;
  int autoSwitchTime = 500;

  ENUM_BUTTON pressedButton = getButtonV6();

  if(pressedButton == NONE && lastRead != 0) {
    isPressedSince = 0;
    return;
  }
  if(pressedButton != lastPressedButton) {
    isPressedSince = 0;
  }

  if(isPressedSince > 3) autoSwitchTime = 100;
  if(lastRead != 0 && millis() - lastRead < autoSwitchTime && pressedButton == lastPressedButton) return;

  isPressedSince++;
  lastRead = millis();
  lastPressedButton = pressedButton;
  
  switch(pressedButton) {
    case NEXT: handleNextV6(); break;
    case PREV: handlePrevV6(); break;
    case BACK: handleBackV6(); break;
    case OK:   handleOkV6();   break;
  }

  lcd.clear();

  if (isInLowerLevel == true && isInSubMenu == false) {
    switch (main_menu_pos)  {
    case 0:
     lcd.setCursor(0,0); 
     lcd.print("* ");
     lcd.print("Main settings:");
     lcd.setCursor(0, 1);
     lcd.print(main_settings_labels[sub_menu_pos]);
      break;
    case 1:
      lcd.setCursor(0,0); 
      lcd.print("* ");
      lcd.print("ELRS settings:");
      lcd.setCursor(0, 1);
      lcd.print(ELRS_settings_labels[sub_menu_pos]);
      break;
    //case 2: // NRF24 settings menu (disabled)
    //  lcd.setCursor(0,0);
    //  lcd.print("* ");
    //  lcd.print(main_menu_labels[2]);
    //  lcd.print(":");
    //  lcd.setCursor(0, 1);
    //  lcd.print(NRF_settings_labels[sub_menu_pos]);
    //  break;
    case 3:
      lcd.setCursor(0,0); 
      lcd.print("* ");
       lcd.print(main_menu_labels[3]);
     lcd.print(":");
      lcd.setCursor(0, 1);
      lcd.print(TRIMS_labels[sub_menu_pos]);
      break;
    case 4:
      lcd.setCursor(0,0); 
      lcd.print("* ");
       lcd.print(main_menu_labels[4]);
     lcd.print(":");
      lcd.setCursor(0, 1);
      lcd.print(VTX_admin_labels[sub_menu_pos]);
      break;
    case 5:
      lcd.setCursor(0,0); 
      lcd.print("* ");
       lcd.print(main_menu_labels[5]);
     lcd.print(":");
      lcd.setCursor(0, 1);
      lcd.print(MISC_settings_labels[sub_menu_pos]);
      break;
    }
  } else if (isInLowerLevel == true && isInSubMenu == true) {
    //lcd.setCursor(0, 1); 
    if (main_menu_pos == 0) {
     /* if(main_settings[sub_menu_pos].handler != NULL && main_settings[sub_menu_pos].maxVal <= main_settings[sub_menu_pos].minVal) {
      (*(main_settings[sub_menu_pos].handler))();
      return; */
      //}
        main_settings[sub_menu_pos].handler();

    } else if (main_menu_pos == 1) {
      if(ELRS_menu[sub_menu_pos].handler != 0) {
      (*(ELRS_menu[sub_menu_pos].handler))();
      return;
      }
    //} else if (main_menu_pos == 2) {
    //  if(NRF24_menu[sub_menu_pos].handler != 0) {
    //  (*(NRF24_menu[sub_menu_pos].handler))();
    //  return;
    //  }
    } else if (main_menu_pos == 3) {
      if(TRIM_menu[sub_menu_pos].handler != 0) {
      (*(TRIM_menu[sub_menu_pos].handler))();
      return;
      }
    } else if (main_menu_pos == 4) {
      if(vtx_menu[sub_menu_pos].handler != 0) {
      (*(vtx_menu[sub_menu_pos].handler))();
      return;
      }
    } else if (main_menu_pos == 5) {
      if(misc_menu[sub_menu_pos].handler != 0) {
      (*(misc_menu[sub_menu_pos].handler))();
      return;
      }
    } else {

    }
    //to be written

  } else {
    lcd.setCursor(0,0); 
    lcd.print("> ");
    lcd.print(main_menu_labels[main_menu_pos]);
    
  }
  
}


void show_data() {

    if (main_menu_pos == 0 && isInLowerLevel == false) {

    char buffer[16];
       
      lcd.setCursor(0, 2);
      lcd.print("Y");
      sprintf(buffer,"%4d",yaw);
      lcd.print(buffer);

      lcd.setCursor(8, 2);
      lcd.print("T");
      sprintf(buffer,"%4d",throttle);
      lcd.print(buffer);

      lcd.setCursor(0, 3);
      lcd.print("R");
      sprintf(buffer,"%4d",roll);
      lcd.print(buffer);

      lcd.setCursor(8, 3);
      lcd.print("P");
      sprintf(buffer,"%4d",pitch);
      lcd.print(buffer);

      lcd.setCursor(0, 4);
      lcd.print("Bat:");
      String voltage = String((analogRead(battery_in) * ((3.3 / 4096) * bat_volt_div_ratio)), 2);
      lcd.print(voltage); lcd.print("V");
      
      if (lcd_cycle_number > 3) {
        lcd.setCursor(0, 1);
        if (scroll_line_pos > 33) {
          scroll_line_pos = 0;
        } else {
          scroll_line_pos++;
        }

        for (int i = 0; i < 16; i++) {
          int j = i + scroll_line_pos;
          if (j > 33) {
            lcd.print(main_message[j - 34]);
          } else {
            lcd.print(main_message[j-1]);
          }
          
          lcd.setCursor(i, 1);
        }
        lcd_cycle_number = 0;
      } else {
        lcd_cycle_number++;
      }
      
      lcd.setCursor(12, 0);
      lcd.print("M");
      lcd.print(mixer_selected);

      if (protocol_selected == 2) {

        lcd.setCursor(0, 5);
        lcd.print("ELRS 2.4");

      } else if (protocol_selected == 1) {
         lcd.setCursor(0, 5);
        lcd.print("ELRS 868");

      } else if (protocol_selected == 0) {

        lcd.setCursor(0, 5);
        lcd.print("USB_HID");

      } else if (protocol_selected == 3) {

      //  lcd.setCursor(0, 5);
      //  lcd.print("NRF24");
        lcd.setCursor(0, 5);
        lcd.print("UNKNOWN");

      } else {
        lcd.setCursor(0, 5);
        lcd.print("UNKNOWN");
      }

    } else if (main_menu_pos == 1 && isInLowerLevel == false) {

      if (lcd_cycle_number > 3) {
        lcd.setCursor(0, 1);
        if (scroll_line_pos > 33) {
          scroll_line_pos = 0;
        } else {
          scroll_line_pos++;
        }

        for (int i = 0; i < 16; i++) {
          int j = i + scroll_line_pos;
          if (j > 33) {
            lcd.print(ELRS_message[j - 34]);
          } else {
            lcd.print(ELRS_message[j-1]);
          }
          
          lcd.setCursor(i, 1);
        }
        lcd_cycle_number = 0;
      } else {
        lcd_cycle_number++;
      }
      lcd.setCursor(0, 2);
      lcd.print("RSSI:");
      lcd.print(LinkStatistics.uplink_RSSI_1);
      lcd.setCursor(0, 3);
      lcd.print("LQ:");
      lcd.print(LinkStatistics.uplink_Link_quality);
      lcd.setCursor(0, 4);
      lcd.print("Bat:");
      rx_voltage = batteryVoltage.voltage / 10.0;
      String rx_bat = String(rx_voltage, 1);
      lcd.print(rx_bat);
      lcd.print("V");

    } else if (main_menu_pos == 3 && isInSubMenu == true && sub_menu_pos == 2) {

      lcd.setCursor(0, 2);
      lcd.print("Roll =");
      char buffer[16]; 
      sprintf(buffer,"%4d",roll); 
      //lcd.setCursor(6, 2); 
      lcd.print(buffer);

    } else if (main_menu_pos == 3 && isInSubMenu == true && sub_menu_pos == 3) {

      lcd.setCursor(0, 2);
      lcd.print("Pitch =");
      char buffer[16]; 
      sprintf(buffer,"%4d",pitch); 
      //lcd.setCursor(7, 2); 
      lcd.print(buffer);

    } else if (main_menu_pos == 3 && isInSubMenu == true && sub_menu_pos == 0) {

      lcd.setCursor(0, 2);
      lcd.print("Yaw =");
      char buffer[16]; 
      sprintf(buffer,"%4d",yaw); 
      //lcd.setCursor(5, 2); 
      lcd.print(buffer);

    } else if (main_menu_pos == 3 && isInSubMenu == true && sub_menu_pos == 1) {

      lcd.setCursor(0, 2);
      lcd.print("Throttle =");
      char buffer[16]; 
      sprintf(buffer,"%4d",throttle); 
      //lcd.setCursor(11, 2); 
      lcd.print(buffer);

    }
}

void wifi_en() {
  lcd.setCursor(0, 0);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_ENABLE_WIFI, 4);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
    //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  lcd.print("TX WIFI!");

}

void RX_wifi_update() {
  lcd.setCursor(0, 0);
 
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_EN_RX_WIFI, 4);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  lcd.print("RX WIFI!");
 
  
}

void vtx_band_change() {
  lcd.setCursor(0, 0);
  lcd.print(vtx_menu[0].label);
  lcd.setCursor(0, 1);
  lcd.print(vtx_bands_labels[tempVal]);
  vtx_band = tempVal;
  EEPROM_write(EEPROM_VTX_B_ADDR, vtx_band);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //open vtx administrator
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
  if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // channel
  //delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
    //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // power
  //delay(20);
  

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void vtx_channel_change() {
  lcd.setCursor(0, 0);
  lcd.print(vtx_menu[1].label);
  lcd.setCursor(0, 1);
  lcd.print(vtx_channels_labels[tempVal]);
  vtx_channel = tempVal;
  EEPROM_write(EEPROM_VTX_CH_ADDR, vtx_channel);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //open vtx administrator
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // channel
  //delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // power
  //delay(20);
  

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
  //  hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void vtx_power_change() {
  lcd.setCursor(0, 0);
  lcd.print(vtx_menu[2].label);
  lcd.setCursor(0, 1);
  lcd.print(tempVal);
  vtx_power = tempVal;
  EEPROM_write(EEPROM_VTX_P_ADDR, vtx_power);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
 // } else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //open vtx administrator
  //delay(20);
  
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // channel
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // power
  //delay(20);
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
    //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
  //  hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void vtx_pit_mode() {
  lcd.setCursor(0, 0);
  lcd.print(vtx_menu[3].label);
  lcd.setCursor(0, 1);
  lcd.print(vtx_pitmode_labels[tempVal]);
  vtx_pit = tempVal;
  EEPROM_write(EEPROM_VTX_PIT_ADDR, vtx_pit);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
  if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //open vtx administrator
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
 if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
 if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // channel

    buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
 if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
    //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // power
  //delay(20);
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
 if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
  if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 4) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void ELRS_Pkt_rate_change() {
  lcd.setCursor(0, 0);
  lcd.print(ELRS_menu[1].label);
  lcd.setCursor(0, 1);
  ELRS_Pkt_rate = tempVal;
  EEPROM_update(EEPROM_ELRS_PKT_R_ADDR, ELRS_Pkt_rate);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_PACKET_RATE, elrs_tx_pkt_rate[tempVal]);
   if (protocol_selected == 1) {
    lcd.print(elrs_tx_pkt_rate_labels_868[tempVal]);
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    lcd.print(elrs_tx_pkt_rate_labels_2400[tempVal]);
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
 // } else if (protocol_selected == 0 || protocol_selected == 3) {
  //  lcd.print(elrs_tx_pkt_rate_labels_default[tempVal]);
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void ELRS_POWER() {
  lcd.setCursor(0, 0);
  lcd.print(ELRS_menu[0].label);
  lcd.setCursor(0,1);
  ELRS_TX_Power = tempVal;
  EEPROM_update(EEPROM_ELRS_TX_P_ADDR, ELRS_TX_Power);

  lcd.print(elrs_tx_pwr_lvl_labels[tempVal]);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_MAX_TX_POWER, elrs_tx_pwr_lvl[ELRS_TX_Power]);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 0 || protocol_selected == 3) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
  delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_TX_POWER_SET, elrs_tx_pwr_lvl[ELRS_TX_Power]);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
 // } else if (protocol_selected == 0 || protocol_selected == 3) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
}

void ELRS_telemetry_ratio() {
  lcd.setCursor(0, 0);
  lcd.print(ELRS_menu[2].label);
  lcd.setCursor(0, 1);
  ELRS_Tlm_ratio = tempVal;
  EEPROM_update(EEPROM_ELRS_TLM_R_ADDR, ELRS_Tlm_ratio);
  lcd.print(elrs_telem_ratio_labels[ELRS_Tlm_ratio]);
  
  buildElrsPacket(crsfCmdPacket, 2, ELRS_Tlm_ratio);

  if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //} else if (protocol_selected == 0 || protocol_selected == 3) {
  //  hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }

}

void bind_rx() {
  lcd.setCursor(0, 0);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_BIND, 4);
  if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
 // } else if (protocol_selected == 0 || protocol_selected == 3) {
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  lcd.print("BIND MODE!");

}

/*
void RF24_model_select() {
  lcd.print("NRF24 ID:");
  lcd.setCursor(0, 1);
  rf24_model_id = tempVal;
  EEPROM_update(HID_COMMAND_MODEL_ID, rf24_model_id);
  lcd.print(rf24_model_id);
  
 // buildElrsPacket(crsfCmdPacket, HID_COMMAND_MODEL_ID, rf24_model_id);
 // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

}

void RF24_channel_select() {
  lcd.print("NRF24 channel:");
  lcd.setCursor(0, 1);
  rf24_ch = tempVal;
  EEPROM_update(HID_COMMAND_TX_CHANNEL, rf24_ch);
  lcd.print(rf24_ch);
  
  buildElrsPacket(crsfCmdPacket, HID_COMMAND_TX_CHANNEL, rf24_ch);
  //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

}

void RF24_power_select() {
  lcd.print("NRF24 power:");
  lcd.setCursor(0, 1);
  rf24_pwr_lvl = tempVal;
  EEPROM_update(HID_COMMAND_TX_PWR, rf24_pwr_lvl);
  //lcd.print(rf24_pwr_lvl);
  
  switch (rf24_pwr_lvl) {
  case 0:
    lcd.print("MIN");
    break;
   case 1:
    lcd.print("LOW");
    break;
   case 2:
    lcd.print("HIGH");
    break;
   case 3:
    lcd.print("MAX");
    break;
  }
  
  buildElrsPacket(crsfCmdPacket, HID_COMMAND_TX_PWR, rf24_pwr_lvl);
  //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

}
*/

void Trim_T() {
  lcd.setCursor(0, 0);
  lcd.print(TRIM_menu[1].label);
  lcd.setCursor(0, 1);
  throttle_fine = tempVal;
  EEPROM_update((32 + mixer_selected*4), throttle_fine);
  lcd.print(throttle_fine);
  #ifdef debug
  SerialUSB.print("trim t:"); SerialUSB.println(throttle_fine);
  #endif

}

void Trim_Y() {
  lcd.setCursor(0, 0);
  lcd.print(TRIM_menu[0].label);
  lcd.setCursor(0, 1);
  yaw_fine = tempVal;
  EEPROM_update((33 + mixer_selected*4), yaw_fine);
  lcd.print(yaw_fine);
  #ifdef debug
  SerialUSB.print("trim y:"); SerialUSB.println(yaw_fine);
  #endif

}

void Trim_P() {
  lcd.setCursor(0, 0);
  lcd.print(TRIM_menu[3].label);
  lcd.setCursor(0, 1);
  pitch_fine = tempVal;
  EEPROM_update((34 + mixer_selected*4), pitch_fine);
  lcd.print(pitch_fine);
  #ifdef debug
  SerialUSB.print("trim p:"); SerialUSB.println(pitch_fine);
  #endif
  
}

void Trim_R() {
  lcd.setCursor(0, 0);
  lcd.print(TRIM_menu[2].label);
  lcd.setCursor(0, 1);
  roll_fine = tempVal;
  EEPROM_update((35 + mixer_selected*4), roll_fine);
  lcd.print(roll_fine);
  #ifdef debug
  SerialUSB.print("trim r:"); SerialUSB.println(roll_fine);
  #endif
  
}

void BUZZER_CFG() {
    lcd.setCursor(0, 0);
  lcd.print(main_settings[2].label);
  lcd.setCursor(0, 1);
  switch(tempVal) {
    case(1):
      lcd.print("ON");
      break;
    case(0):
      lcd.print("OFF");
      break;
    }
    EEPROM_update(EEPROM_BUZZER_ADDR, tempVal);
    use_buzzer = tempVal;
  

}

void RC_protocol() {
  lcd.setCursor(0, 0);
  lcd.print(main_settings[0].label);
  lcd.setCursor(0, 1);
  switch(tempVal) {
  case(5):
    lcd.print(" ");
    break;
  case(4):
    lcd.print(" ");
    break;
  //case(3):
  //  lcd.print("RF24");
  //  break;
  case(2):
    lcd.print("ExpressLRS 2.4");
    break;
  case(1):
    lcd.print("ExpressLRS 868");
    break;
  case(0):
    lcd.print("SIM");
    break;
  }

  EEPROM_update(EEPROM_PROTOCOL_ADDR, tempVal);
  protocol_selected = tempVal;

}

void select_mixer() {
    lcd.setCursor(0, 0);
  lcd.print(main_settings[1].label);
  lcd.setCursor(0, 1);
  lcd.print(mixer_labels[tempVal]);
  
  mixer_selected = tempVal;

  #ifdef debug
  SerialUSB.print("mixer selected:"); SerialUSB.println(mixer_selected);
  #endif
  
  EEPROM_update(EEPROM_MIXER_ADDR, tempVal);

  throttle_fine = EEPROM_read((32 + mixer_selected*4));
  yaw_fine = EEPROM_read((33 + mixer_selected*4));
  pitch_fine = EEPROM_read((34 + mixer_selected*4));
  roll_fine = EEPROM_read((35 + mixer_selected*4));
/*
  menu[1] = {"Roll Trim", 0, 255, roll_fine, Trim_R};
  menu[2] = {"Pitch Trim", 0, 255, pitch_fine, Trim_P};
  menu[3] = {"Yaw Trim", 0, 255, yaw_fine, Trim_Y};
  menu[4] = {"Throttle Trim", 0, 255, throttle_fine, Trim_T};
  */
  #ifdef debug
  SerialUSB.print("thr_fine:"); SerialUSB.println(throttle_fine);
  SerialUSB.print("yaw_fine:"); SerialUSB.println(yaw_fine);
  SerialUSB.print("rll_fine:"); SerialUSB.println(roll_fine);
  SerialUSB.print("pit_fine:"); SerialUSB.println(pitch_fine);
  #endif
  //mixer_on_boot = mixer_selected;
}

void elrs_telemetry() {}

void CRSF_SEND_868() {
  doMixing[mixer_selected]();
  crsfPreparePacket(crsfPacket, rcChannels);
  ELRS_Serial_868.write(crsfPacket, CRSF_PACKET_SIZE); //Send data over CRSF to tx module

} 

void CRSF_SEND_2400() {
  doMixing[mixer_selected]();
  crsfPreparePacket(crsfPacket, rcChannels);
  ELRS_Serial_2400.write(crsfPacket, CRSF_PACKET_SIZE); //Send data over CRSF to tx module

} 

void HID_SEND_CRSF() {
  doMixing[mixer_selected]();
  crsfPreparePacket(crsfPacket, rcChannels);
  //hid_serial.write(crsfPacket, CRSF_PACKET_SIZE); //Send data over CRSF to tx module

} 

void void_null() {
  //do nothing
}

void setup()  {

  //pinMode(reset_pin, OUTPUT);
 // digitalWrite(reset_pin, HIGH);
  pinMode(Module_power_868, OUTPUT);
  digitalWrite(Module_power_868, LOW);
  pinMode(Module_power_2400, OUTPUT);
  digitalWrite(Module_power_2400, LOW);
  //pinMode(PF_4, INPUT);
 // pinMode(buzzer, OUTPUT);
 // digitalWrite(buzzer, LOW);
  //setup two invidual serial ports for tx modules + one for HID module
  ELRS_Serial_868.begin(CRSF_baudrate); //UART1
  ELRS_Serial_2400.begin(CRSF_baudrate);  //UART2
  //hid_serial.begin(400000);  //UART3

  //debug serial over usb
  #ifdef debug
  SerialUSB.begin(115200);
  SerialUSB.println("starting debug");
  #endif
  pinMode(joystick_Y, INPUT);  //joystick 1.1
  pinMode(joystick_T, INPUT);  //joystick 1.2
  pinMode(joystick_P, INPUT);  //joystick 2.1
  pinMode(joystick_R, INPUT);  //joystick 2.2

  pinMode(battery_in, INPUT);  //battery voltage divider

  pinMode(AUX1, INPUT_PULLDOWN); //AUX1
  pinMode(AUX2, INPUT_PULLDOWN);  //AUX2
  pinMode(AUX3, INPUT_PULLDOWN);  //AUX3
  pinMode(AUX4, INPUT_PULLDOWN);  //AUX4

  //pinMode(POT1, INPUT);
  //pinMode(POT2, INPUT);
  
  Wire.begin();
  Wire.setClock(100000);
  //showSplashScreen();
  
  usb_data = EEPROM_read(EEPROM_USB_DAT_ADDR);   //load settings from external eeprom
 /* throttle_fine = EEPROM_read(1);
  yaw_fine = EEPROM_read(2);
  pitch_fine = EEPROM_read(3);
  roll_fine = EEPROM_read(4); */
  protocol_selected = EEPROM_read(EEPROM_PROTOCOL_ADDR);
  ELRS_Pkt_rate = EEPROM_read(EEPROM_ELRS_PKT_R_ADDR);
  ELRS_TX_Power = EEPROM_read(EEPROM_ELRS_TX_P_ADDR);
  vtx_band = EEPROM_read(EEPROM_VTX_B_ADDR);
  vtx_channel = EEPROM_read(EEPROM_VTX_CH_ADDR);
  vtx_power = EEPROM_read(EEPROM_VTX_P_ADDR);
  ELRS_Tlm_ratio = EEPROM_read(EEPROM_ELRS_TLM_R_ADDR);
  mixer_selected = EEPROM_read(EEPROM_MIXER_ADDR);
  vtx_pit = EEPROM_read(EEPROM_VTX_PIT_ADDR);
  use_buzzer = EEPROM_read(EEPROM_BUZZER_ADDR);

  throttle_fine = EEPROM_read((32 + mixer_selected*4));
  yaw_fine = EEPROM_read((33 + mixer_selected*4));
  pitch_fine = EEPROM_read((34 + mixer_selected*4));
  roll_fine = EEPROM_read((35 + mixer_selected*4));

  //rf24_model_id = EEPROM_read(HID_COMMAND_MODEL_ID);
  //rf24_ch = EEPROM_read(HID_COMMAND_TX_CHANNEL);
  //rf24_pwr_lvl = EEPROM_read(HID_COMMAND_TX_PWR);

  //mixer_on_boot = mixer_selected; //save values read on boot to check if user changed them during use
  protocol_on_boot = protocol_selected;
//  protocol_selected = protocol_selected;
  
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);

  //menu[menu_number] = {label, value_low, value_high, default_value, function}
  //main_settings[0] = {"RC protocol:", 0, 3, protocol_selected, RC_protocol};
  main_settings[0] = {"RC protocol:", 0, 2, protocol_selected, RC_protocol}; // max 2: no RF24 (was 3)
  main_settings[1] = {"Mixer:", 0, (number_of_mixers - 1), mixer_selected, select_mixer};
  main_settings[2] = {"Buzzer", 0, 1, use_buzzer, BUZZER_CFG};

  ELRS_menu[0] = {"ELRS Power:", 0, 6, ELRS_TX_Power, ELRS_POWER}; //list =  {'10 mW', '25 mW', '50 mW', '100 mW', '250 mW', '500 mW', '1000 mW', '2000 mW'},
  ELRS_menu[1] = {"ELRS PKT Rate:", 0, 8, ELRS_Pkt_rate, ELRS_Pkt_rate_change};
  ELRS_menu[2] = {"TlM ratio:", 0, 7, ELRS_Tlm_ratio, ELRS_telemetry_ratio};
  ELRS_menu[3] = {"Enable TX WiFi", 0, 0, 0, wifi_en};
  ELRS_menu[4] = {"Enable RX WiFi", 0, 0, 0, RX_wifi_update};
  ELRS_menu[5] = {"Bind RX", 0, 0, 0, bind_rx};
  
  TRIM_menu[0] = {"Yaw Trim:", 0, 255, yaw_fine, Trim_Y};
  TRIM_menu[1] = {"Throttle Trim:", 0, 255, throttle_fine, Trim_T};
  TRIM_menu[2] = {"Roll Trim:", 0, 255, roll_fine, Trim_R};
  TRIM_menu[3] = {"Pitch Trim:", 0, 255, pitch_fine, Trim_P};
  
  vtx_menu[0] = {"VTX Band:", 0, 6, vtx_band, vtx_band_change};
  vtx_menu[1] = {"VTX Channel:", 0, 7, vtx_channel, vtx_channel_change};
  vtx_menu[2] = {"VTX Power:", 0, 5, vtx_power, vtx_power_change};
  vtx_menu[3] = {"VTX Pit:", 0, 7, vtx_pit, vtx_pit_mode};
  
  //NRF24_menu[0] = {"RF24 Model ID:", 0, 20, rf24_model_id, RF24_model_select};
  //NRF24_menu[1] = {"RF24 Channel:", 0, 125, rf24_ch, RF24_channel_select};
  //NRF24_menu[2] = {"RF24 Power:", 0, 3, rf24_pwr_lvl, RF24_power_select};

  misc_menu[0] = {"LEDS", 0, 0, 0, 0};
  misc_menu[1] = {"EEPROM Reset!", 0, 0, 0, reset_eeprom};
 
  analogReadResolution(12); //set ADC to 12-bit res
  
  lcd.begin();

  showSplashScreen();
  if (use_buzzer == true) {
    buildElrsPacket(crsfCmdPacket, HID_COMMAND_BEEP, 1);
    //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

    //digitalWrite(buzzer, HIGH);
   // delay(100);
   // digitalWrite(buzzer, LOW);
  }
  #ifdef debug
  SerialUSB.print("mixer selected boot:"); SerialUSB.println(mixer_selected);
  #endif
  
  #ifdef debug
  SerialUSB.print("thr_fine boot:"); SerialUSB.println(throttle_fine);
  SerialUSB.print("yaw_fine boot:"); SerialUSB.println(yaw_fine);
  SerialUSB.print("rll_fine boot:"); SerialUSB.println(roll_fine);
  SerialUSB.print("pit_fine boot:"); SerialUSB.println(pitch_fine);
  #endif

  if (protocol_selected == 0) { 
   
    digitalWrite(Module_power_868, LOW); //send crsf data to pi pico working as hid interface 
    digitalWrite(Module_power_2400, LOW);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(250, HERTZ_FORMAT); 
    //CRSF_TIM->attachInterrupt(HID_SEND_CRSF);
    CRSF_TIM->resume();

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_PROTOCOL, 0);
    //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  

  } else if (protocol_selected == 1) { // Expresslrs 868MHz
      //Turn on ELRS Module
    digitalWrite(Module_power_868, HIGH);
    digitalWrite(Module_power_2400, LOW);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(250, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(CRSF_SEND_868);
    CRSF_TIM->resume();

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_PROTOCOL, 0);
    //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

  } else if (protocol_selected == 2) { // ExpressLRS 2.4GHz
      //Turn on ELRS Module
    digitalWrite(Module_power_2400, HIGH);
    digitalWrite(Module_power_868, LOW);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(250, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(CRSF_SEND_2400);
    CRSF_TIM->resume();

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_PROTOCOL, 0);
    //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

  //} else if (protocol_selected == 3) { 
  //
  //  digitalWrite(Module_power_868, LOW); //send crsf data to pi pico working as rf24 rc tx module
  //  digitalWrite(Module_power_2400, LOW);
  //  CRSF_TIM->pause();
  //  CRSF_TIM->setPrescaleFactor(72);
  //  CRSF_TIM->setOverflow(250, HERTZ_FORMAT); 
  //  //CRSF_TIM->attachInterrupt(HID_SEND_CRSF);
  //  CRSF_TIM->resume();
  //
  //  buildElrsPacket(crsfCmdPacket, HID_COMMAND_PROTOCOL, 1);
  //  //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //
  //  buildElrsPacket(crsfCmdPacket, HID_COMMAND_MODEL_ID, rf24_model_id);
  //  //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //
  //  buildElrsPacket(crsfCmdPacket, HID_COMMAND_TX_CHANNEL, rf24_ch);
  //  //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //
  //  buildElrsPacket(crsfCmdPacket, HID_COMMAND_TX_PWR, rf24_pwr_lvl);
  //  //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  
  }
  
  for (int i = 0; i < CRSF_MAX_CHANNEL; i++) { 

    if (i == 1) {
      rcChannels[i] = RC_CHANNEL_MIN; // min value for throttle channel
    } else {
      rcChannels[i] = RC_CHANNEL_MID; // mid for rest of channels
    }

  } 
  
  hideSplashScreen(); //finish boot sequance 
}

void loop() {
 // Serial1.swap();
  unsigned long currentMillis = millis();

  if ((currentMillis - lcdMillis) > 100) { //dont update lcd every time, only every third of a second
    
    #ifndef showoff
    drawMenuV6();
    show_data();
    #endif
    
    #ifdef showoff
    showSplashScreen();
    #endif
    lcdMillis = currentMillis;
     #ifdef debug
  
  SerialUSB.print("yaw:");
  SerialUSB.print(analogRead(joystick_Y));
  SerialUSB.print(" thr:");
  SerialUSB.print(analogRead(joystick_T));
  SerialUSB.print(" roll:");
  SerialUSB.print(analogRead(joystick_R));
  SerialUSB.print(" pitch:");
  SerialUSB.print(analogRead(joystick_P));
  SerialUSB.print(" AUX1:");
  SerialUSB.print(digitalRead(AUX1));
  SerialUSB.print(" AUX3:");
  SerialUSB.print(digitalRead(AUX3));
  SerialUSB.print(" VBAT:");
  String voltage = String((analogRead(battery_in) * ((3.3 / 4096) * bat_volt_div_ratio)), 2);
  SerialUSB.print(voltage);
  SerialUSB.println("V");
  //delay(10);
  #endif
  }
  
  if (protocol_selected == 1) {
    serialtelemetryevent_868();
  } else if (protocol_selected == 2) {
    serialtelemetryevent_2400();
  }

 // if (use_buzzer == true && armed != digitalRead(AUX1)) {
   // armed = digitalRead(AUX1);
   // buildElrsPacket(crsfCmdPacket, HID_COMMAND_BEEP, 1);
   // hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

    //digitalWrite(buzzer, HIGH);
   // delay(100);
   // digitalWrite(buzzer, LOW);
 // }
  //if (hid_serial.available()) {
   // hid_serial.readBytes((char*)&HID_tele_struct, HID_tele_size);
  //}
/*
  if (currentMenuPos != 5 && mixer_selected != mixer_on_boot) { //auto reset for modes requiring reboot to apply
   digitalWrite(reset_pin, LOW);
  } */

  if (isInSubMenu == false && sub_menu_pos != 0 && protocol_selected != protocol_on_boot) {
    digitalWrite(reset_pin, LOW);
    //buildElrsPacket(crsfCmdPacket, HID_COMMAND_RESET, 1);
    //hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }  

 /* if (currentMenuPos != 5 && mixer_selected != mixer_on_boot) {
    
    throttle_fine = EEPROM_read((32 + mixer_selected*4));
    yaw_fine = EEPROM_read(33 + mixer_selected);
    pitch_fine = EEPROM_read((34 + mixer_selected*4));
    roll_fine = EEPROM_read((35 + mixer_selected*4));
    mixer_on_boot = mixer_selected; 
  }  */

  #ifdef debug
  char received = SerialUSB.read();
  if (received == 'r') {
    //digitalWrite(reset_pin, LOW);
  }
  #endif
  
}
