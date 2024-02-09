
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareTimer.h>
#include <Wire.h>
#include "config.h"
#include "crsf.c"
#include "mixers.c"
#include "eeprom_f.c"
//#include "CRSF_CMD.h"
#include "button_handler.c"
#include "lcd.c"
//#include "PPM.c"

//special mode where splash screen is being shown constantly - for taking pictures
//#define showoff

//SFW model names to be used in school
//#define SCHOOL

//debug
//#define debug

#ifdef debug

#define USBCON
#define PIO_FRAMEWORK_ARDUINO_ENABLE_CDC

#endif

TIM_TypeDef *Instance_CRSF_TIM = CRSF_TIM_DEF;
HardwareTimer *CRSF_TIM = new HardwareTimer(Instance_CRSF_TIM);

HardwareSerial Serial3(USART3);
HardwareSerial Serial2(USART2);


void show_data() {

    if (currentMenuPos == 0) {
      
      char buffer[16];
       
      lcd.setCursor(0, 2);   
      lcd.print("Y");  
      sprintf(buffer,"%4d",yaw);      
      lcd.print(buffer);

      lcd.setCursor(5, 2);   
      lcd.print("T");  
      sprintf(buffer,"%4d",throttle); 
      lcd.print(buffer);

      lcd.setCursor(10, 2);  
      lcd.print("R");  
      sprintf(buffer,"%4d",roll);     
      lcd.print(buffer);

      lcd.setCursor(15, 2);  
      lcd.print("P");  
      sprintf(buffer,"%4d",pitch);
      lcd.print(buffer);

      lcd.setCursor(0, 3);
      lcd.print("Bat:");
      String voltage = String((analogRead(battery_in) * ((3.3 / 4096) * bat_volt_div_ratio)), 2);
      //lcd.setCursor(4, 3); 
      lcd.print(voltage); lcd.print("V");

      lcd.setCursor(16, 0);
      lcd.print("M");
      lcd.print(mixer_selected);

      if (protocol_selected == 2) {

        lcd.setCursor(11, 3); 
        lcd.print("ELRS 2.4");

      } else if (protocol_selected == 1) {
         lcd.setCursor(11, 3); 
        lcd.print("ELRS 868");

      } else if (protocol_selected == 0) {

        lcd.setCursor(11, 3); 
        lcd.print("USB_HID");

      } else if (protocol_selected == 3) {

        lcd.setCursor(11, 3); 
        lcd.print("NRF24");

      } else {
        lcd.setCursor(11, 3); 
        lcd.print("UNKNOWN");
      }
     
    } else if (currentMenuPos == 1 && isInLowerLevel == true) {

      lcd.setCursor(0, 2);
      lcd.print("Roll =");
      char buffer[16]; 
      sprintf(buffer,"%4d",roll); 
      //lcd.setCursor(6, 2); 
      lcd.print(buffer);

    } else if (currentMenuPos == 2 && isInLowerLevel == true) {

      lcd.setCursor(0, 2);
      lcd.print("Pitch =");
      char buffer[16]; 
      sprintf(buffer,"%4d",pitch); 
      //lcd.setCursor(7, 2); 
      lcd.print(buffer);

    } else if (currentMenuPos == 3 && isInLowerLevel == true) {

      lcd.setCursor(0, 2);
      lcd.print("Yaw =");
      char buffer[16]; 
      sprintf(buffer,"%4d",yaw); 
      //lcd.setCursor(5, 2); 
      lcd.print(buffer);

    } else if (currentMenuPos == 4 && isInLowerLevel == true) {

      lcd.setCursor(0, 2);
      lcd.print("Throttle =");
      char buffer[16]; 
      sprintf(buffer,"%4d",throttle); 
      //lcd.setCursor(11, 2); 
      lcd.print(buffer);

    } else if (currentMenuPos == 22) { //telemtry screen
      lcd.setCursor(0, 2);
      lcd.print("RSSI:");
      lcd.print(LinkStatistics.uplink_RSSI_1);
      lcd.setCursor(0, 3);
      lcd.print("Bat:");
      rx_voltage = batteryVoltage.voltage / 10.0;
      String rx_bat = String(rx_voltage, 1);
      lcd.print(rx_bat);
      lcd.print("V");
      lcd.setCursor(10, 2);
      lcd.print("LQ:");
      lcd.print(LinkStatistics.uplink_Link_quality);
      lcd.print(" ");
    
    }
}

void wifi_en() {
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_ENABLE_WIFI, 4);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 

}

void RX_wifi_update() {
 
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_EN_RX_WIFI, 4);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
  
}

void vtx_band_change() {
  lcd.print(vtx_bands_labels[tempVal]);
  vtx_band = tempVal;
  EEPROM_write(8, vtx_band);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
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
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // channel
  //delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // power
  //delay(20);
  

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void vtx_channel_change() {

  lcd.print(vtx_channels_labels[tempVal]);
  vtx_channel = tempVal;
  EEPROM_write(9, vtx_channel);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //open vtx administrator
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // channel
  //delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // power
  //delay(20);
  

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void vtx_power_change() {

  lcd.print(tempVal);
  vtx_power = tempVal;
  EEPROM_write(10, vtx_power);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //open vtx administrator
  //delay(20);
  
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // channel
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // power
  //delay(20);
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void vtx_pit_mode() {

  lcd.print(vtx_pitmode_labels[tempVal]);
  vtx_pit = tempVal;
  EEPROM_write(13, vtx_pit);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
  if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //open vtx administrator
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
 if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
 if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // channel

    buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
 if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 // power
  //delay(20);
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
 if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
  // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
  if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 4) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void ELRS_Pkt_rate_change() {

  ELRS_Pkt_rate = tempVal;
  EEPROM_update(6, ELRS_Pkt_rate);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_PACKET_RATE, elrs_tx_pkt_rate[tempVal]);
   if (protocol_selected == 1) {
    lcd.print(elrs_tx_pkt_rate_labels_868[tempVal]);
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    lcd.print(elrs_tx_pkt_rate_labels_2400[tempVal]);
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 0 || protocol_selected == 3) {
    lcd.print(elrs_tx_pkt_rate_labels_default[tempVal]);
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
}

void ELRS_POWER() {

  ELRS_TX_Power = tempVal;
  EEPROM_update(7, ELRS_TX_Power);

  lcd.print(elrs_tx_pwr_lvl_labels[tempVal]);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_MAX_TX_POWER, elrs_tx_pwr_lvl[ELRS_TX_Power]);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 0 || protocol_selected == 3) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
 
  delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_TX_POWER_SET, elrs_tx_pwr_lvl[ELRS_TX_Power]);
   if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 0 || protocol_selected == 3) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }
}

void ELRS_telemetry_ratio() {

  ELRS_Tlm_ratio = tempVal;
  EEPROM_update(11, ELRS_Tlm_ratio);
  lcd.print(elrs_telem_ratio_labels[ELRS_Tlm_ratio]);
  
  buildElrsPacket(crsfCmdPacket, 2, ELRS_Tlm_ratio);

  if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 0 || protocol_selected == 3) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }

}

void bind_rx() {

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_BIND, 4);
  if (protocol_selected == 1) {
    ELRS_Serial_868.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 2) {
    ELRS_Serial_2400.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  } else if (protocol_selected == 0 || protocol_selected == 3) {
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  }

}

void RF24_model_select() {

  rf24_model_id = tempVal;
  EEPROM_update(HID_COMMAND_MODEL_ID, rf24_model_id);
  lcd.print(rf24_model_id);
  
  buildElrsPacket(crsfCmdPacket, HID_COMMAND_MODEL_ID, rf24_model_id);
  hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

}

void RF24_channel_select() {

  rf24_ch = tempVal;
  EEPROM_update(HID_COMMAND_TX_CHANNEL, rf24_ch);
  lcd.print(rf24_ch);
  
  buildElrsPacket(crsfCmdPacket, HID_COMMAND_TX_CHANNEL, rf24_ch);
  hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

}

void RF24_power_select() {

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
  hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

}

void Trim_T() {
  
  throttle_fine = tempVal;
  EEPROM_update((32 + mixer_selected*4), throttle_fine);
  lcd.print(throttle_fine);
  #ifdef debug
  SerialUSB.print("trim t:"); SerialUSB.println(throttle_fine);
  #endif

}

void Trim_Y() {

  yaw_fine = tempVal;
  EEPROM_update((33 + mixer_selected*4), yaw_fine);
  lcd.print(yaw_fine);
  #ifdef debug
  SerialUSB.print("trim y:"); SerialUSB.println(yaw_fine);
  #endif

}

void Trim_P() {

  pitch_fine = tempVal;
  EEPROM_update((34 + mixer_selected*4), pitch_fine);
  lcd.print(pitch_fine);
  #ifdef debug
  SerialUSB.print("trim p:"); SerialUSB.println(pitch_fine);
  #endif
  
}

void Trim_R() {

  
  roll_fine = tempVal;
  EEPROM_update((35 + mixer_selected*4), roll_fine);
  lcd.print(roll_fine);
  #ifdef debug
  SerialUSB.print("trim r:"); SerialUSB.println(roll_fine);
  #endif
  
}


void USB_DATA_OUT() {
switch(tempVal) {
  case(1):
    lcd.print("useless");
    break;
  case(0):
    lcd.print("debug");
    break;
  }
  usb_data = tempVal;
  EEPROM_update(0, tempVal);

}

void RC_protocol() {

  switch(tempVal) {
  case(5):
    lcd.print(" ");
    break;
  case(4):
    lcd.print(" ");
    break;
  case(3):
    lcd.print("RF24");
    break;
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

  EEPROM_update(5, tempVal);
  protocol_selected = tempVal;

}

void select_mixer() {
  
  lcd.print(mixer_labels[tempVal]);
  
  mixer_selected = tempVal;

  #ifdef debug
  SerialUSB.print("mixer selected:"); SerialUSB.println(mixer_selected);
  #endif
  
  EEPROM_update(12, tempVal);

  throttle_fine = EEPROM_read((32 + mixer_selected*4));
  yaw_fine = EEPROM_read((33 + mixer_selected*4));
  pitch_fine = EEPROM_read((34 + mixer_selected*4));
  roll_fine = EEPROM_read((35 + mixer_selected*4));

  menu[1] = {"Roll Trim", 0, 255, roll_fine, Trim_R};
  menu[2] = {"Pitch Trim", 0, 255, pitch_fine, Trim_P};
  menu[3] = {"Yaw Trim", 0, 255, yaw_fine, Trim_Y};
  menu[4] = {"Throttle Trim", 0, 255, throttle_fine, Trim_T};
  
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
  hid_serial.write(crsfPacket, CRSF_PACKET_SIZE); //Send data over CRSF to tx module

} 

void void_null() {
  //do nothing
}

void setup()  {

  pinMode(reset_pin, OUTPUT);
  digitalWrite(reset_pin, HIGH);
  pinMode(Module_power_868, OUTPUT);
  digitalWrite(Module_power_868, LOW);
  pinMode(Module_power_2400, OUTPUT);
  digitalWrite(Module_power_2400, LOW);
  //setup two invidual serial ports for tx modules + one for HID module
  ELRS_Serial_868.begin(CRSF_baudrate); //UART1
  ELRS_Serial_2400.begin(CRSF_baudrate);  //UART2
  hid_serial.begin(CRSF_baudrate);  //UART3

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

  pinMode(AUX1, INPUT); //AUX1
  pinMode(AUX2, INPUT);  //AUX2
  pinMode(AUX3, INPUT);  //AUX3
  pinMode(AUX4, INPUT);  //AUX4
  
  Wire.begin();
  Wire.setClock(100000);
  //showSplashScreen();
  
  usb_data = EEPROM_read(0);   //load settings from external eeprom
 /* throttle_fine = EEPROM_read(1);
  yaw_fine = EEPROM_read(2);
  pitch_fine = EEPROM_read(3);
  roll_fine = EEPROM_read(4); */
  protocol_selected = EEPROM_read(5);
  ELRS_Pkt_rate = EEPROM_read(6);
  ELRS_TX_Power = EEPROM_read(7);
  vtx_band = EEPROM_read(8);
  vtx_channel = EEPROM_read(9);
  vtx_power = EEPROM_read(10);
  ELRS_Tlm_ratio = EEPROM_read(11);
  mixer_selected = EEPROM_read(12);
  vtx_pit = EEPROM_read(13);

  throttle_fine = EEPROM_read((32 + mixer_selected*4));
  yaw_fine = EEPROM_read((33 + mixer_selected*4));
  pitch_fine = EEPROM_read((34 + mixer_selected*4));
  roll_fine = EEPROM_read((35 + mixer_selected*4));

  rf24_model_id = EEPROM_read(HID_COMMAND_MODEL_ID);
  rf24_ch = EEPROM_read(HID_COMMAND_TX_CHANNEL);
  rf24_pwr_lvl = EEPROM_read(HID_COMMAND_TX_PWR);

  mixer_on_boot = mixer_selected; //save values read on boot to check if user changed them during use
  protocol_on_boot = protocol_selected;
//  protocol_selected = protocol_selected;
  
  pinMode(BTN_NEXT, INPUT_PULLUP);
  pinMode(BTN_PREV, INPUT_PULLUP);
  pinMode(BTN_BACK, INPUT_PULLUP);
  pinMode(BTN_OK, INPUT_PULLUP);

  //menu[menu_number] = {label, value_low, value_high, default_value, function}
  menu[0] = {"Data:", 0, 0, 0, show_data};
  menu[1] = {"Roll Trim", 0, 255, roll_fine, Trim_R};
  menu[2] = {"Pitch Trim", 0, 255, pitch_fine, Trim_P};
  menu[3] = {"Yaw Trim", 0, 255, yaw_fine, Trim_Y};
  menu[4] = {"Throttle Trim", 0, 255, throttle_fine, Trim_T};
  menu[5] = {"Mixer:", 0, (number_of_mixers - 1), mixer_selected, select_mixer};
  menu[6] = {"RC protocol:", 0, 3, protocol_selected, RC_protocol};
  menu[7] = {"ELRS Power:", 0, 7, ELRS_TX_Power, ELRS_POWER}; //list =  {'10 mW', '25 mW', '50 mW', '100 mW', '250 mW', '500 mW', '1000 mW', '2000 mW'},
  menu[8] = {"ELRS Pkt Rate:", 0, 8, ELRS_Pkt_rate, ELRS_Pkt_rate_change};
  menu[9] = {"Tlm ratio:", 0, 7, ELRS_Tlm_ratio, ELRS_telemetry_ratio};
  menu[10] = {"USB data out:", 0, 1, usb_data, USB_DATA_OUT};
  menu[11] = {"VTX Band:", 0, 6, vtx_band, vtx_band_change};
  menu[12] = {"VTX Channel:", 0, 7, vtx_channel, vtx_channel_change};
  menu[13] = {"VTX Power", 0, 5, vtx_power, vtx_power_change};
  menu[14] = {"VTX Pit", 0, 7, vtx_pit, vtx_pit_mode};
  menu[15] = {"WiFi EN:", 0, 0, 0, wifi_en};
  menu[16] = {"RX WiFi update", 0, 0, 0, RX_wifi_update};
  menu[17] = {"Bind RX", 0, 0, 0, bind_rx};

  menu[18] = {"RF24 Model ID", 0, 20, rf24_model_id, RF24_model_select};
  menu[19] = {"RF24 Channel", 0, 125, rf24_ch, RF24_channel_select};
  menu[20] = {"RF24 Power", 0, 3, rf24_pwr_lvl, RF24_power_select};

  menu[21] = {"EEPROM Reset", 0, 0, 0, reset_eeprom};
  menu[22] = {"TELEMETRY", 0, 0, 0, elrs_telemetry};

  menuSize = sizeof(menu)/sizeof(STRUCT_MENUPOS);

  analogReadResolution(12); //set ADC to 12-bit res
  
  lcd.begin();
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

  if (protocol_selected == 0) { 
   
    digitalWrite(Module_power_868, LOW); //send crsf data to pi pico working as hid interface 
    digitalWrite(Module_power_2400, LOW);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(250, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(HID_SEND_CRSF);
    CRSF_TIM->resume();

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_PROTOCOL, 0);
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  

  } else if (protocol_selected == 1) { // Expresslrs 868MHz
      //Turn on ELRS Module
    digitalWrite(Module_power_868, HIGH);
    digitalWrite(Module_power_2400, LOW);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(500, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(CRSF_SEND_868);
    CRSF_TIM->resume();

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_PROTOCOL, 0);
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

  } else if (protocol_selected == 2) { // ExpressLRS 2.4GHz
      //Turn on ELRS Module
    digitalWrite(Module_power_2400, HIGH);
    digitalWrite(Module_power_868, LOW);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(500, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(CRSF_SEND_2400);
    CRSF_TIM->resume();

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_PROTOCOL, 0);
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

  } else if (protocol_selected == 3) { 
  
    digitalWrite(Module_power_868, LOW); //send crsf data to pi pico working as rf24 rc tx module
    digitalWrite(Module_power_2400, LOW);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(250, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(HID_SEND_CRSF);
    CRSF_TIM->resume();

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_PROTOCOL, 1);
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_MODEL_ID, rf24_model_id);
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_TX_CHANNEL, rf24_ch);
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

    buildElrsPacket(crsfCmdPacket, HID_COMMAND_TX_PWR, rf24_pwr_lvl);
    hid_serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  
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
    drawMenu();
    show_data();
    #endif
    
    #ifdef showoff
    showSplashScreen();
    #endif
    lcdMillis = currentMillis;
  }
  
  if (protocol_selected == 1) {
    serialtelemetryevent_868();
  } else if (protocol_selected == 2) {
    serialtelemetryevent_2400();
  }
/*
  if (currentMenuPos != 5 && mixer_selected != mixer_on_boot) { //auto reset for modes requiring reboot to apply
   digitalWrite(reset_pin, LOW);
  } */

  if (currentMenuPos != 6 && protocol_selected != protocol_on_boot) {
    digitalWrite(reset_pin, LOW);
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
    digitalWrite(reset_pin, LOW);
  }
  #endif
}
