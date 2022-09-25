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


void reset_eeprom() {

  EEPROM_write(0, 50); //nrf24 channel
  EEPROM_write(1, 127); //trim 1
  EEPROM_write(2, 127); //trim 2
  EEPROM_write(3, 127); //trim 3
  EEPROM_write(4, 127); //trim 4
  EEPROM_write(5, 0); //elrs RF
  EEPROM_write(7, 0); //elrs output power

}
