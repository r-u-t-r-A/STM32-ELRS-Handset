#include "config.h"
#include <Arduino.h>
#include "crsf.c"

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
  
  buildElrsPacket(crsfCmdPacket, ELRS_POWER_COMMAND, currentPower[tempVal]);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);
  buildElrsPacket(crsfCmdPacket, 0x00, 0x00);
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
 
  buildElrsPacket(crsfCmdPacket, ELRS_PKT_RATE_COMMAND, currentPktRate[tempVal]);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);
  buildElrsPacket(crsfCmdPacket, 0x00, 0x00);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  delay(20);
  
}
