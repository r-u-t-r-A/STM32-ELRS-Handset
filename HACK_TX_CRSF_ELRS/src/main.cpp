//add in speech emulation for voice notifications

#include <Arduino.h>
#include <LiquidCrystal_I2C.h>
#include <HardwareTimer.h>
#include <Wire.h>
#include "config.h"
#include "crsf.c"
#include "mixers.c"
#include "eeprom_f.c"
//#include "CRSF_CMD.h"
#include "sbus.c"
#include "button_handler.c"
#include "lcd.c"
//#include "crsf_telemetry.c"
//#include "crsf_protocol.h"



TIM_TypeDef *Instance_CRSF_TIM = CRSF_TIM_DEF;
HardwareTimer *CRSF_TIM = new HardwareTimer(Instance_CRSF_TIM);

//ADC_TypeDef *ADC_instance = ADC1;
/*
void (*doMixing[2])() { - array of void pointers - instead of switch(...)
  default_mixer,
  elevon_mixer
} ; */

void show_data() {

    if (currentMenuPos == 0) {
      
      char buffer[16];
       
      lcd.setCursor(0, 2);   
      lcd.print("R");  
      sprintf(buffer,"%4d",yaw);      
      lcd.print(buffer);

      lcd.setCursor(5, 2);   
      lcd.print("T");  
      sprintf(buffer,"%4d",throttle); 
      lcd.print(buffer);

      lcd.setCursor(10, 2);  
      lcd.print("A");  
      sprintf(buffer,"%4d",roll);     
      lcd.print(buffer);

      lcd.setCursor(15, 2);  
      lcd.print("E");  
      sprintf(buffer,"%4d",pitch);
      lcd.print(buffer);

      lcd.setCursor(0, 3);
      lcd.print("Bat:");
      String voltage = String((analogRead(battery_in) * ((3.3 / 4096) * bat_volt_div_ratio)), 2);
      //lcd.setCursor(4, 3); 
      lcd.print(voltage); lcd.print("V");

      if (ELRS_RF == true) {

        lcd.setCursor(14, 3); 
        lcd.print("ELRS ");

      } else {

        lcd.setCursor(14, 3); 
        lcd.print("USB");

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

    } else if (currentMenuPos == 18) {
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
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);

}

void RX_wifi_update() {
 
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_EN_RX_WIFI, 4);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  
}

void vtx_band_change() {
  lcd.print(vtx_bands_labels[tempVal]);
  vtx_band = tempVal;
  EEPROM_write(8, vtx_band);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //open vtx administrator
  //delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// power
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// channel
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
}

void vtx_channel_change() {
/*
  lcd.print(tempVal);
  vtx_channel = tempVal - 1;
  EEPROM_write(9, vtx_channel);
  
  buildElrsPacket(crsfCmdPacket, 8, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //open vtx administrator
  //delay(20);
  buildElrsPacket(crsfCmdPacket, 9, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// power
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, 10, vtx_band);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, 11, vtx_channel);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// channel
  //delay(20);

  buildElrsPacket(crsfCmdPacket, 12, vtx_pit);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); // probably pit mode? - set to one cahnges nothing
  //delay(20);
*/
  
  lcd.print(vtx_channels_labels[tempVal]);
  vtx_channel = tempVal;
  EEPROM_write(9, vtx_channel);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //open vtx administrator
  //delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// power
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// channel
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
}

void vtx_power_change() {
/*
  lcd.print(tempVal + 1);
  vtx_power = tempVal;
  EEPROM_write(10, vtx_power);
  
 buildElrsPacket(crsfCmdPacket, 8, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //open vtx administrator
  //delay(20);
  buildElrsPacket(crsfCmdPacket, 9, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// power
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, 10, vtx_band);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, 11, vtx_channel);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// channel
  //delay(20);

  buildElrsPacket(crsfCmdPacket, 12, vtx_pit);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); // probably pit mode? - set to one cahnges nothing
  //delay(20); */

  lcd.print(tempVal);
  vtx_power = tempVal;
  EEPROM_write(10, vtx_power);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //open vtx administrator
  //delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// power
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// channel
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
}

void vtx_pit_mode() {
/*  lcd.print(tempVal);
  vtx_power = tempVal;
  EEPROM_write(13, vtx_power);
  
  buildElrsPacket(crsfCmdPacket, 8, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //open vtx administrator
  //delay(20);
  buildElrsPacket(crsfCmdPacket, 9, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// power
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, 10, vtx_band);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, 11, vtx_channel);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// channel
  //delay(20);

  buildElrsPacket(crsfCmdPacket, 12, vtx_pit);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); // probably pit mode? - set to one cahnges nothing
  //delay(20);


  buildElrsPacket(crsfCmdPacket, 13, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// vtx send */
  lcd.print(vtx_pitmode_labels[tempVal]);
  vtx_pit = tempVal;
  EEPROM_write(13, vtx_pit);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_ADMIN, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //open vtx administrator
  //delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PWR_LVL, vtx_power);
  //buildElrsPacket(crsfCmdPacket, 12, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// power
  //delay(20);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_BAND, vtx_bands[vtx_band]);
 // buildElrsPacket(crsfCmdPacket, 10, 5);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); //band
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_CH, vtx_channels[vtx_channel]);
 // buildElrsPacket(crsfCmdPacket, 11, (tempVal - 1));
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);// channel
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_VTX_PIT_MODE, vtx_pitmode[vtx_pit]);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); // probably pit mode? - set to one cahnges nothing
  //delay(20);

  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_SEND_VTX, 1);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
}

void Trim_T() {
  
  lcd.print(tempVal);
  EEPROM_update(1, tempVal);
  throttle_fine = tempVal;

}

void Trim_Y() {

  lcd.print(tempVal);
  EEPROM_update(2, tempVal);
  yaw_fine = tempVal;

}

void Trim_R() {

  lcd.print(tempVal);
  EEPROM_update(4, tempVal);
  roll_fine = tempVal;
  
}

void Trim_P() {

  lcd.print(tempVal);
  EEPROM_update(3, tempVal);
  pitch_fine = tempVal;
  
}

void USB_DATA_OUT() {
switch(tempVal) {
  case(1):
    lcd.print("SBUS out");
    break;
  case(0):
    lcd.print("debug");
    break;
  }
  usb_data = tempVal;
  EEPROM_update(0, tempVal);

}

void ELRS_Pkt_rate_change() {
/*
  switch (tempVal)  {
  case (3):
    lcd.print("200Hz");
    break;
  case (2):
    lcd.print("100Hz");
    break;
  case (1):
    lcd.print("50Hz");
    break;
  case (0):
    lcd.print("25Hz");
    break;
  }
  
  ELRS_Pkt_rate = tempVal;
  EEPROM_update(6, ELRS_Pkt_rate);
  
  buildElrsPacket(crsfCmdPacket, 1, ELRS_Pkt_rate);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
 // delay(20); */
  ELRS_Pkt_rate = tempVal;
  EEPROM_update(6, ELRS_Pkt_rate);
  lcd.print(elrs_tx_pkt_rate_labels_868[tempVal]);
 
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_PACKET_RATE, elrs_tx_pkt_rate[tempVal]);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
}

void RC_protocol() {

  switch(tempVal) {
  case(1):
    lcd.print("ExpressLRS");
    break;
  case(0):
    lcd.print("SBUS over USB");
    break;
  }

  EEPROM_update(5, tempVal);

}

void ELRS_POWER() {
/*  switch (tempVal)  {
  case (0):
    lcd.print("10mW    ");
    break;
  case (1):
    lcd.print("25mW    ");
    break;
  case (2):
    lcd.print("50mW    ");
    break;
  case (3):
    lcd.print("100mW   ");
    break;
  case (4):
    lcd.print("250mW   ");
    break;
  case (5):
    lcd.print("500mW   ");
    break;
  case (6):
    lcd.print("1000mW");
    break;
  }
  
  ELRS_TX_Power = tempVal;
  EEPROM_update(7, ELRS_TX_Power);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_MAX_TX_POWER, ELRS_TX_POWER);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_TX_POWER_SET, ELRS_TX_POWER);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
 // delay(20); */
  
  ELRS_TX_Power = tempVal;
  EEPROM_update(7, ELRS_TX_Power);

  lcd.print(elrs_tx_pwr_lvl_labels[tempVal]);
  
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_MAX_TX_POWER, elrs_tx_pwr_lvl[ELRS_TX_Power]);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);
  buildElrsPacket(crsfCmdPacket, ELRS_LUA_COMMAND_TX_POWER_SET, elrs_tx_pwr_lvl[ELRS_TX_Power]);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
 
}

void ELRS_telemetry_ratio() {
/*
  switch (tempVal)  {
  case (1):
    lcd.print("1:128");
    break;
  case (2):
    lcd.print("1:64");
    break;
  case (3):
    lcd.print("1:32");
    break;
  case (4):
    lcd.print("1:16");
    break;
  case (5):
    lcd.print("1:8");
    break;
  case (6):
    lcd.print("1:4");
    break;
  case (7):
    lcd.print("1:2");
    break;
  case (0):
    lcd.print("Telemetry off");
    break;
  
  }
  
  ELRS_Tlm_ratio = tempVal;
  EEPROM_update(11, ELRS_Tlm_ratio);
  buildElrsPacket(crsfCmdPacket, 2, ELRS_Tlm_ratio);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE); */

  ELRS_Tlm_ratio = tempVal;
  EEPROM_update(11, ELRS_Tlm_ratio);
  lcd.print(elrs_telem_ratio_labels[ELRS_Tlm_ratio]);
  
  buildElrsPacket(crsfCmdPacket, 2, ELRS_Tlm_ratio);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  
 // delay(20);
}

void select_mixer() {
  lcd.print(mixer_labels[tempVal]);
  mixer_selected = tempVal;
  EEPROM_update(12, tempVal);

}

void elrs_telemetry() {}

void CRSF_SEND() {
  doMixing[mixer_selected]();
  crsfPreparePacket(crsfPacket, rcChannels);
  ELRS_Serial.write(crsfPacket, CRSF_PACKET_SIZE); //Send data over CRSF to tx module

} 

void SBUS_SEND() {
  doMixing[mixer_selected]();
  sbusPreparePacket(sbusPacket, rcChannels, false, false);
  SerialUSB.write(sbusPacket, SBUS_PACKET_LENGTH);
}

void setup()  {

  SerialUSB.begin(115200);
  
  pinMode(Module_power, OUTPUT);
  digitalWrite(Module_power, LOW);
  ELRS_Serial.begin(CRSF_baudrate);
  
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
  
  usb_data = EEPROM_read(0);
  throttle_fine = EEPROM_read(1);
  yaw_fine = EEPROM_read(2);
  pitch_fine = EEPROM_read(3);
  roll_fine = EEPROM_read(4);
  ELRS_RF = EEPROM_read(5);
  ELRS_Pkt_rate = EEPROM_read(6);
  ELRS_TX_Power = EEPROM_read(7);
  vtx_band = EEPROM_read(8);
  vtx_channel = EEPROM_read(9);
  vtx_power = EEPROM_read(10);
  ELRS_Tlm_ratio = EEPROM_read(11);
  mixer_selected = EEPROM_read(12);
  vtx_pit = EEPROM_read(13);
  
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
  menu[6] = {"RC protocol:", 0, 1, ELRS_RF, RC_protocol};
  menu[7] = {"ELRS Power:", 0, 7, ELRS_TX_Power, ELRS_POWER}; //list =  {'10 mW', '25 mW', '50 mW', '100 mW', '250 mW', '500 mW', '1000 mW', '2000 mW'},
  menu[8] = {"ELRS Pkt Rate:", 0, 5, ELRS_Pkt_rate, ELRS_Pkt_rate_change};
  menu[9] = {"Tlm ratio:", 0, 7, ELRS_Tlm_ratio, ELRS_telemetry_ratio};
  menu[10] = {"USB data out:", 0, 1, usb_data, USB_DATA_OUT};
  menu[11] = {"VTX Band:", 0, 6, vtx_band, vtx_band_change};
  menu[12] = {"VTX Channel:", 0, 8, vtx_channel, vtx_channel_change};
  menu[13] = {"VTX Power", 0, 5, vtx_power, vtx_power_change};
  menu[14] = {"VTX Pit", 0, 8, vtx_pit, vtx_pit_mode};
  menu[15] = {"WiFi EN:", 0, 0, 0, wifi_en};
  menu[16] = {"RX WiFi update", 0, 0, 0, RX_wifi_update};
  menu[17] = {"EEPROM Reset", 0, 0, 0, reset_eeprom};
  menu[18] = {"TELEMETRY", 0, 0, 0, elrs_telemetry};

  menuSize = sizeof(menu)/sizeof(STRUCT_MENUPOS);

  analogReadResolution(12);
  
  lcd.begin();
  //showSplashScreen();

  if (ELRS_RF == false) {
    //strat sending sbus over usb serial using timer
    digitalWrite(Module_power, LOW);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(250, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(SBUS_SEND);
    CRSF_TIM->resume();
  } else {
      //Turn on ELRS Module
    digitalWrite(Module_power, HIGH);
    CRSF_TIM->pause();
    CRSF_TIM->setPrescaleFactor(72);
    CRSF_TIM->setOverflow(200, HERTZ_FORMAT); 
    CRSF_TIM->attachInterrupt(CRSF_SEND);
    CRSF_TIM->resume();
  }
  
  showSplashScreen();
  

  for (int i = 0; i < CRSF_MAX_CHANNEL; i++) {

    rcChannels[i] = RC_CHANNEL_MIN;

  } 
  
}

void loop() {
  
  unsigned long currentMillis = millis();

  if ((currentMillis - lcdMillis) > 333) {
    drawMenu();
    show_data();
    lcdMillis = currentMillis;
  }
  serialtelemetryevent();
 /* rx_voltage = batteryVoltage.voltage / 10;
  if (usb_data = 0){
  SerialUSB.print("RSSI:"); SerialUSB.println(LinkStatistics.uplink_RSSI_1);
  SerialUSB.print("bat:"); SerialUSB.print(rx_voltage); SerialUSB.println("V");
  } */

}
