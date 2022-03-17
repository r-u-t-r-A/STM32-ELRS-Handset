#include <Arduino.h>
#include "CRSF_CMD.h"
#include "crsf.h"
#include "crsf.c"

void wifi_en() {
 
  buildElrsPacket(crsfCmdPacket, 15, 4);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
  //delay(20);
 
}

void RX_wifi_update() {
  buildElrsPacket(crsfCmdPacket, 16, 4);
  ELRS_Serial.write(crsfCmdPacket, CRSF_CMD_PACKET_SIZE);
 // delay(20);
}
