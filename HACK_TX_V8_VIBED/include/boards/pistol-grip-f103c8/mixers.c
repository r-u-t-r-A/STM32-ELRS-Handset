#pragma once
//#include "PPM.c"
#include <Arduino.h>
/* number_of_mixers is derived from doMixing[]/mixer_labels[] at the bottom
 * of this file -- add or remove a mixer there and the count follows. */

extern int throttle;
extern int yaw;
extern int throttle_fine;
extern int yaw_fine;
extern int rcChannels[CRSF_MAX_CHANNEL];

const char* mixer_labels[] = {"RTAE1234", "AETR1234", "AETR_lim","CALIB"};

void RTAE1234() { //default mixer
  throttle = ch_from_throttle(joystick_T, 0, 4095);
  yaw = ch_from_axis(joystick_Y, 4095, 0);

  throttle = ch_apply_trim(throttle, throttle_fine);
  yaw = ch_apply_trim(yaw, yaw_fine);

  rcChannels[0]=yaw;
  rcChannels[1]=throttle;
  rcChannels[4]=ch_from_switch(AUX1);
  rcChannels[5]=ch_from_axis(AUX2, 0, 4095);
  
}

void calib() { //default mixer

  throttle = pin_read_analog(joystick_T, 0);
  yaw = pin_read_analog(joystick_Y, 0);

}
void AETR1234() { //default mixer 
 throttle = ch_from_throttle(joystick_T, 0, 4095);
  yaw = ch_from_axis(joystick_Y, 4095, 0);
  throttle = ch_apply_trim(throttle, throttle_fine);
  yaw = ch_apply_trim(yaw, yaw_fine);
  rcChannels[2] = throttle;
  rcChannels[3] = yaw;

  rcChannels[4]=ch_from_switch(AUX1);

  rcChannels[5]=ch_from_axis(AUX2, 0, 4095);
  rcChannels[10]=ch_from_button(BTN_BACK); //for sim
  rcChannels[11]=ch_from_button(BTN_OK);

}

void AETR_lim() { //default mixer

  int aux2_val = ch_from_axis(AUX2, 0, 4095);
  int raw_throttle = pin_read_analog(joystick_T, 0);
 // raw_throttle = ch_apply_trim(raw_throttle, throttle_fine);
  if (aux2_val < 800) {
    throttle = map(raw_throttle, 0, 4095, 582, 1402);
  } else if (aux2_val > 800 && aux2_val < 1180) {
    throttle = map(raw_throttle, 0, 4095, 378, 1606);
  } else {
    throttle = map(raw_throttle, 0, 4095, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  }
  yaw = ch_from_axis(joystick_Y, 4095, 0);
  yaw = ch_apply_trim(yaw, yaw_fine);
  throttle = ch_apply_trim(throttle, throttle_fine);
  rcChannels[3]=yaw;
  rcChannels[2]=throttle;

  rcChannels[4]=ch_from_switch(AUX1);

  rcChannels[5]=aux2_val;
  rcChannels[12]=ch_from_button(BTN_BACK); //for sim
  rcChannels[13]=ch_from_button(BTN_OK);

}
void (*doMixing[])() = {
  RTAE1234,
  AETR1234,
  AETR_lim,
  calib
};

#define number_of_mixers (sizeof(doMixing) / sizeof(doMixing[0]))
static_assert(sizeof(mixer_labels) / sizeof(mixer_labels[0]) == number_of_mixers,
              "mixer_labels and doMixing must have the same number of entries");