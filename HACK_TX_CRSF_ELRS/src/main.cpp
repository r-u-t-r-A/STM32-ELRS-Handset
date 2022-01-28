//add in speech emulation for voice notifications

#include <Arduino.h>
#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include <LiquidCrystal_I2C.h>
//#include <LiquidCrystal.h>

//#include <HardwareTimer.h>
#include "config.h"
#include "crsf.c"
#include "sbus.c"
#include "EEPROM_F.h"
//#include "button_handler.c"
/*
TIM_TypeDef *Instance = TIM4;
HardwareTimer *PPM_TIM = new HardwareTimer(Instance);
*/
LiquidCrystal_I2C lcd(0x27, 20, 4);

//RF24 radio(PA4, PB0); //Set CE and CSN pins
RF24 radio(PB10, PA4);

uint8_t read_power;
uint8_t read_Pkt_rate;

void show_data() {

  unsigned long currentMillis = millis();

  if ((currentMillis - lcdMillis) > 333) {
    
    if (currentMenuPos == 0) {
       
      lcd.setCursor(0, 2);
      lcd.print("R");
      lcd.setCursor(5, 2);
      lcd.print("T");
      lcd.setCursor(10, 2);
      lcd.print("A");
      lcd.setCursor(15, 2);
      lcd.print("E");
  
      char buffer[16]; sprintf(buffer,"%4dV",yaw); lcd.setCursor(1, 2); lcd.print(buffer);
      sprintf(buffer,"%4d",throttle); lcd.setCursor(6, 2); lcd.print(buffer);
      sprintf(buffer,"%4d",roll); lcd.setCursor(11, 2); lcd.print(buffer);
      sprintf(buffer,"%4d",pitch); lcd.setCursor(16, 2); lcd.print(buffer);
  
      lcd.setCursor(0, 3);
      lcd.print("Bat:");
      String voltage = String((analogRead(battery_in) * ((3.3 / 4096) * 2.751)), 2);
      lcd.setCursor(4, 3); lcd.print(voltage);

      if (ELRS_RF == true) {

        lcd.setCursor(14, 3); lcd.print("ELRS ");

      } else {

        lcd.setCursor(14, 3); lcd.print("nRF24");

      }
     
    } else if (currentMenuPos == 1 && isInLowerLevel == true) {

      lcd.setCursor(0, 2);
      lcd.print("Roll =");
      char buffer[16]; sprintf(buffer,"%4d",roll); lcd.setCursor(6, 2); lcd.print(buffer);

    } else if (currentMenuPos == 2 && isInLowerLevel == true) {

      lcd.setCursor(0, 2);
      lcd.print("Pitch =");
      char buffer[16]; sprintf(buffer,"%4d",pitch); lcd.setCursor(7, 2); lcd.print(buffer);

    } else if (currentMenuPos == 3 && isInLowerLevel == true) {

      lcd.setCursor(0, 2);
      lcd.print("Yaw =");
      char buffer[16]; sprintf(buffer,"%4d",yaw); lcd.setCursor(5, 2); lcd.print(buffer);

    } else if (currentMenuPos == 4 && isInLowerLevel == true) {

      lcd.setCursor(0, 2);
      lcd.print("Throttle =");
      char buffer[16]; sprintf(buffer,"%4d",throttle); lcd.setCursor(11, 2); lcd.print(buffer);

    }

    lcdMillis = currentMillis;
  }  

}

void handleNext() {
  if(isInLowerLevel) {
    tempVal++;
    if(tempVal > menu[currentMenuPos].maxVal) tempVal = menu[currentMenuPos].maxVal;
  } else {
    currentMenuPos = (currentMenuPos + 1) % menuSize;
  }
}

void handlePrev() {
  if(isInLowerLevel) {
    tempVal--;
    if(tempVal < menu[currentMenuPos].minVal) tempVal = menu[currentMenuPos].minVal;
  } else {
    currentMenuPos--;
    if(currentMenuPos < 0) currentMenuPos = menuSize - 1;
  }
}

void handleBack() {
  if(isInLowerLevel) {
    isInLowerLevel = false;
  }
}

void handleOk() {
  if(menu[currentMenuPos].handler != NULL && menu[currentMenuPos].maxVal <= menu[currentMenuPos].minVal) {
    (*(menu[currentMenuPos].handler))();
    return;
  }
  if(isInLowerLevel) {
    menu[currentMenuPos].currentVal = tempVal;
    isInLowerLevel = false;
  } else {
    tempVal = menu[currentMenuPos].currentVal;
    isInLowerLevel = true;
  }
}


ENUM_BUTTON getButton() {
  if(!digitalRead(BTN_BACK)) return BACK;
  if(!digitalRead(BTN_NEXT)) return NEXT;
  if(!digitalRead(BTN_PREV)) return PREV;
  if(!digitalRead(BTN_OK)) return OK;

  return NONE;
}


void drawMenu() {
  static unsigned long lastRead = 0;
  static ENUM_BUTTON lastPressedButton = OK;
  static unsigned int isPressedSince = 0;
  int autoSwitchTime = 500;

  ENUM_BUTTON pressedButton = getButton();

  if(pressedButton == NONE && lastRead != 0) {
    isPressedSince = 0;
    return;
  }
  if(pressedButton != lastPressedButton) {
    isPressedSince = 0;
  }

  if(isPressedSince > 3) autoSwitchTime = 70;
  if(lastRead != 0 && millis() - lastRead < autoSwitchTime && pressedButton == lastPressedButton) return;

  isPressedSince++;
  lastRead = millis();
  lastPressedButton = pressedButton;
  
  switch(pressedButton) {
    case NEXT: handleNext(); break;
    case PREV: handlePrev(); break;
    case BACK: handleBack(); break;
    case OK: handleOk(); break;
  }

  
  lcd.clear();
  
  if(isInLowerLevel) {
    lcd.setCursor(0, 0);
    lcd.print(menu[currentMenuPos].label);
    lcd.setCursor(0, 1);
    lcd.print(F("> "));

    if(menu[currentMenuPos].handler != NULL) {
      (*(menu[currentMenuPos].handler))();
    } else {
      lcd.print(tempVal);
    }
  } else {
    lcd.setCursor(0, 0);
    lcd.print(F("Main Menu:"));
    lcd.setCursor(0, 1);
    lcd.print(F("> "));

    lcd.print(menu[currentMenuPos].label);
   
  }
}

/* Funkcje-uchwyty użytkownika */
void vtx_channel_change() {
  lcd.print(tempVal);
  buildElrsPacket(crsfCmdPacket, 10, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);

  buildElrsPacket(crsfCmdPacket, 13, 0);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);
  
}

void reset_eeprom() {
   EEPROM_write(0, 50);
    EEPROM_write(1, 127);
    EEPROM_write(2, 127);
    EEPROM_write(3, 127);
    EEPROM_write(4, 127);
    EEPROM_write(5, 0);
    EEPROM_write(7, 0);
}

void ELRS_Enable() {
  lcd.print(tempVal);
  EEPROM_update(5, tempVal);
}

void ELRS_POWER() {
  lcd.print(tempVal);
 // currentPower = tempVal;
  EEPROM_update(7, tempVal);
  
  buildElrsPacket(crsfCmdPacket, 6, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);
  buildElrsPacket(crsfCmdPacket, 5, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);
 
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

void CH_NRF24() {

  lcd.print(tempVal);
  EEPROM_update(0, tempVal);
}

void ELRS_Pkt_rate() {

  lcd.print(tempVal);
  EEPROM_update(6, tempVal);
  buildElrsPacket(crsfCmdPacket, 1, tempVal);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);
  
  
}

void radio_begin() {

  radio.begin();
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS); 
  radio.setChannel(ch);
  radio.setPALevel(RF24_PA_MIN);
  radio.openWritingPipe(radio_pipe);
  //radio.openReadingPipe(1, radio_pipe);

}

void convert_the_data_new() {
 
  throttle = map(analogRead(PA2), 3620, 860, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = map(analogRead(PA3), 714, 3680, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = map(analogRead(PA1), 740, 3560, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = map(analogRead(PA0), 3460, 920, RC_CHANNEL_MIN, RC_CHANNEL_MAX);  

  throttle = throttle + (throttle_fine - 127) * 4;
  yaw = yaw + (yaw_fine - 127) * 4;
  pitch = pitch + (pitch_fine - 127) * 4;
  roll = roll + (roll_fine - 127) * 4;  

  throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  if (ELRS_RF == true /* || SBUS_over_Serial == true */) {

    rcChannels[0]=yaw;
    rcChannels[1]=throttle;
    rcChannels[2]=roll;
    rcChannels[3]=pitch;
    rcChannels[4]=map(digitalRead(PA15), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    rcChannels[5]=map(digitalRead(PB3), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    rcChannels[6]=map(digitalRead(PB4), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    rcChannels[7]=map(digitalRead(PB5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  } else {
    
    tx.txCH2 = map(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX, 0, 255);
    tx.txCH1 = map(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX, 0, 255);
    tx.txCH4 = map(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX, 0, 255);
    tx.txCH3 = map(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX, 0, 255);

    tx.txCH5 = digitalRead(PA15);
    tx.txCH6 = digitalRead(PB3);  

    tx.txCH7 = digitalRead(PB4);
    tx.txCH8 = digitalRead(PB5);

  #ifdef DEBUG
    SerialUSB.println(tx.txCH1);
    SerialUSB.println(tx.txCH2);
    SerialUSB.println(tx.txCH3);
    SerialUSB.println(tx.txCH4);
    SerialUSB.println(tx.txCH5);
    SerialUSB.println(tx.txCH6);
    SerialUSB.println(tx.txCH7);
    SerialUSB.println(tx.txCH8);
  
    SerialUSB.println("READ:");
    SerialUSB.println(analogRead(PA1));
    SerialUSB.println(analogRead(PA0));
    SerialUSB.println(analogRead(PA3));
    SerialUSB.println(analogRead(PA2));
  #endif
  }
}

void transmit() {

  SerialUSB.println("Trying to transmit");
  radio.stopListening();
  delay(1);
  radio.write(&tx, sizeof(tx));
  SerialUSB.println("done");

}

void showSplashScreen() {
  //lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("HACK_RC_TX ELRS");
  //SerialUSB.println("HACK_TX debug");
  lcd.setCursor(0, 1);
  lcd.print("BY rutrA");
  delay(2000);
  //lcd.noBacklight();
  lcd.clear();
  delay(1000);
}

void setup()  {

  SerialUSB.begin(115200);
  SPI.setClockDivider(SPI_CLOCK_DIV64);
  pinMode(Module_power, OUTPUT);
  digitalWrite(Module_power, LOW);
  ELRS_Serial.begin(CRSF_baudrate);
  
  pinMode(PA3, INPUT);  //joystick 1.1
  pinMode(PA2, INPUT);  //joystick 1.2
  pinMode(PA1, INPUT);  //joystick 2.1
  pinMode(PA0, INPUT);  //joystick 2.2

  pinMode(PB0, INPUT);  //battery voltage divider

  pinMode(PB1, INPUT);  //Trim buttons

  pinMode(PA15, INPUT); //AUX1
  pinMode(PB3, INPUT);  //AUX2
  pinMode(PB4, INPUT);  //AUX3
  pinMode(PB5, INPUT);  //AUX4
  

  Wire.begin();

  ch = EEPROM_read(0);
  throttle_fine = EEPROM_read(1);
  yaw_fine = EEPROM_read(2);
  pitch_fine = EEPROM_read(3);
  roll_fine = EEPROM_read(4);
  read_Pkt_rate = EEPROM_read(6);
  read_power = EEPROM_read(7);
  ELRS_RF = EEPROM_read(5);

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
  menu[5] = {"ELRS EN:", 0, 1, ELRS_RF, ELRS_Enable};
  menu[6] = {"ELRS Power:", 0, 7, read_power, ELRS_POWER}; //list =  {'10 mW', '25 mW', '50 mW', '100 mW', '250 mW', '500 mW', '1000 mW', '2000 mW'},
  menu[7] = {"ELRS Pkt Rate:", 0, 3, read_Pkt_rate, ELRS_Pkt_rate};
  menu[8] = {"NRF24 CH:", 0, 125, ch, CH_NRF24};
  menu[9] = {"EEPROM Reset", 0, 0, 0, reset_eeprom};
  menu[10] = {"VTX Channel:", 1, 8, 8, vtx_channel_change};
  menuSize = sizeof(menu)/sizeof(STRUCT_MENUPOS);

  analogReadResolution(12);
  //lcd.begin(20, 4);
  
  lcd.begin();


  if (ELRS_RF == false) {
    digitalWrite(Module_power, LOW);
    radio_begin();

  } else {
      //Turn on ELRS Module
      digitalWrite(Module_power, HIGH);
    /*  // Setup ELRS Module
      lcd.setCursor(0, 0);
      lcd.print("Setting up ELRS");
      lcd.setCursor(16, 0);
      lcd.blink();
    
      delay(10000);
      buildElrsPacket(crsfCmdPacket,ELRS_PKT_RATE_COMMAND, 0x05);
      ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
      delay(20);
      buildElrsPacket(crsfCmdPacket,ELRS_POWER_COMMAND, 0x05);
      ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
      delay(20);
      
      lcd.blink_off();
      lcd.setCursor(0, 1);
      lcd.print("Done");
      delay(500);
      lcd.clear(); */

    }


    SerialUSB.println("showing splashscreen");
    showSplashScreen();
    SerialUSB.println("done");

    if (lcd_en == true) { 

    } else {

      lcd.noBacklight();
      lcd.clear();

    }
  
   
  for (uint8_t i = 0; i < CRSF_MAX_CHANNEL; i++) {

    rcChannels[i] = RC_CHANNEL_MIN;

  } 
}

void loop() {
  
  uint32_t currentMicros = micros();

  drawMenu();
  show_data();
  
  convert_the_data_new();

  if (ELRS_RF == false)  {
  
    transmit(); //transmit data over NRF24

  } else if (ELRS_RF == true) {

    if (currentMicros > crsfTime) {

      crsfPreparePacket(crsfPacket, rcChannels);
      ELRS_Serial.write(crsfPacket, CRSF_PACKET_SIZE); //Send data over CRSF to tx module
      crsfTime = currentMicros + CRSF_TIME_BETWEEN_FRAMES_US;

    } 

  } /* else if (SBUS_over_Serial == true) {

    if (currentMicros > sbusTime) {

      sbusPreparePacket(sbusPacket, rcChannels);
      SerialUSB.write(sbusPacket, SBUS_PACKET_LENGTH); //Send data over SBUS to vJoySerialFeeder software running on PC
      sbusTime = currentMicros + SBUS_UPDATE_RATE;
    }

  } */

//  if (SBUS_over_Serial == false) {

    SerialUSB.print("Took ");
    SerialUSB.print((millis() - previousMillis));
    SerialUSB.println("ms"); //Print time needed for one loop of code
    previousMillis = millis();
 
 // }
  
}
