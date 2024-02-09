//#include "crsf_telemetry.c"

LiquidCrystal_I2C lcd(0x27, 20, 4);

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

  if(isPressedSince > 3) autoSwitchTime = 100;
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


void showSplashScreen() {
  //lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print(" HACK_TX DUAL v4.5");
 //SerialUSB.println("HACK_TX debug");
  lcd.setCursor(0, 1);
  lcd.print("    BY ARTUR KUC");
  delay(1000);
  //lcd.noBacklight();
  
}

void hideSplashScreen() {
  lcd.clear();
  delay(250);
}
