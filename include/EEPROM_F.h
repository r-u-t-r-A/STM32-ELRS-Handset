#include <Arduino.h>
#include <Wire.h>
#define I2CEEPROM_ADDR 0x57

// writes a byte of data in memory location addr
void EEPROM_write(unsigned int addr, byte eeprom_data)  {

  Wire.beginTransmission(I2CEEPROM_ADDR);
  // set the pointer position
  //Wire.write((int)(addr >> 8));
  Wire.write((int)(addr & 0xFF));
  Wire.write(eeprom_data);
  Wire.endTransmission();
  delay(10);

}

// reads a byte of data from memory location addr
byte EEPROM_read(unsigned int addr)  {

  byte result;
  Wire.beginTransmission(I2CEEPROM_ADDR);
  // set the pointer position
  //Wire.write((int)(addr >> 8));
  Wire.write((int)(addr & 0xFF));
  Wire.endTransmission();
  Wire.requestFrom(I2CEEPROM_ADDR,1); // get the byte of data
  result = Wire.read();
  return result;
}

void EEPROM_update(unsigned int addr, byte to_write) { //to save write cycles of eeprom
  byte wrtitten = EEPROM_read(addr);
  if (to_write == wrtitten) {
    //do nothing, just skip
  } else {
    EEPROM_write(addr, to_write);
  }

}
