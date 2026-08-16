#pragma once
//#include "PPM.c"
#include <Arduino.h>
/* number_of_mixers is derived from doMixing[]/mixer_labels[] at the bottom
 * of this file -- add or remove a mixer there and the count follows. */

extern int throttle;
extern int yaw;
extern int throttle_fine;
extern int yaw_fine;
extern int roll;
extern int pitch;
extern int roll_fine;
extern int pitch_fine;
extern int rcChannels[CRSF_MAX_CHANNEL];

const char* mixer_labels[] = {"RTAE1234", "AETR1234", "servo skid", "drift fuckup" ,"CALIB"};

void RTAE1234() { //default mixer

  throttle = ch_from_throttle(joystick_T, 650, 3505);
  yaw = ch_from_axis(joystick_Y, 3777, 550);
  pitch = ch_from_axis(joystick_P, 3705, 365);
  roll = ch_from_axis(joystick_R, 580, 3545);

  throttle = throttle + (throttle_fine - 127) * 2;
  yaw = yaw + (yaw_fine - 127) * 2;
  pitch = pitch + (pitch_fine - 127) * 2;
  roll = roll + (roll_fine - 127) * 2;

  throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[0]=yaw;
  rcChannels[1]=throttle;
  rcChannels[2]=roll;
  rcChannels[3]=pitch;
  rcChannels[4]=ch_from_switch(AUX1);

  rcChannels[5]=ch_from_switch(AUX2);
  rcChannels[6]=ch_from_switch(AUX3);
  rcChannels[7]=ch_from_switch(AUX4);
  //rcChannels[8]=map(digitalRead(AUX5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  //rcChannels[9]=map(digitalRead(AUX6), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  //rcChannels[10]=map(analogRead(POT1), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX); //for sim
  //rcChannels[11]=map(analogRead(POT2), 4096, 0, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[10]=ch_from_button(BTN_BACK); //for sim
  rcChannels[11]=ch_from_button(BTN_OK);
}

/*
void RTAE1234() { //default mixer 
 
  throttle = map(analogRead(joystick_T), 3620, 860, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = map(analogRead(joystick_Y), 714, 3680, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = map(analogRead(joystick_P), 740, 3560, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = map(analogRead(joystick_R), 3460, 920, RC_CHANNEL_MIN, RC_CHANNEL_MAX);  

  throttle = throttle + (throttle_fine - 127) * 2;
  yaw = yaw + (yaw_fine - 127) * 2;
  pitch = pitch + (pitch_fine - 127) * 2;
  roll = roll + (roll_fine - 127) * 2;  

  throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[0]=yaw;
  rcChannels[1]=throttle;
  rcChannels[2]=roll;
  rcChannels[3]=pitch;
  rcChannels[4]=map(digitalRead(AUX1), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[5]=map(analogRead(AUX2), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[6]=map(analogRead(AUX3), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[7]=map(digitalRead(AUX4), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[8]=map(digitalRead(AUX5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[9]=map(digitalRead(AUX6), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  
}*/

void calib() { //default mixer

  throttle = pin_read_analog(joystick_T, 0);
  yaw = pin_read_analog(joystick_Y, 0);
  pitch = pin_read_analog(joystick_P, 0);
  roll = pin_read_analog(joystick_R, 0);

}
void AETR1234() { //default mixer 
 /*
  throttle = map(analogRead(joystick_T), 3620, 860, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = map(analogRead(joystick_Y), 714, 3680, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = map(analogRead(joystick_P), 740, 3560, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = map(analogRead(joystick_R), 3460, 920, RC_CHANNEL_MIN, RC_CHANNEL_MAX);  
*/
  throttle = ch_from_throttle(joystick_T, 650, 3505);
  yaw = ch_from_axis(joystick_Y, 3777, 550);
  pitch = ch_from_axis(joystick_P, 3705, 365);
  roll = ch_from_axis(joystick_R, 580, 3545);

  throttle = throttle + (throttle_fine - 127) * 2;
  yaw = yaw + (yaw_fine - 127) * 2;
  pitch = pitch + (pitch_fine - 127) * 2;
  roll = roll + (roll_fine - 127) * 2;

  throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[0]=roll;
  rcChannels[1]=pitch;
  rcChannels[2]=throttle;
  rcChannels[3]=yaw;
  rcChannels[4]=ch_from_switch(AUX1);

  rcChannels[5]=ch_from_switch(AUX2);
  rcChannels[6]=ch_from_switch(AUX3);
  rcChannels[7]=ch_from_switch(AUX4);
  //rcChannels[8]=map(digitalRead(AUX5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  //rcChannels[9]=map(digitalRead(AUX6), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  //rcChannels[10]=map(analogRead(POT1), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX); //for sim
  //rcChannels[11]=map(analogRead(POT2), 4096, 0, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[10]=ch_from_button(BTN_BACK); //for sim
  rcChannels[11]=ch_from_button(BTN_OK);

}

double reverseElevonVal(double val) {
  return (val-992) * -1 + 992;
}

double addElevonVal(double val1, double val2) {
  double out = (val1 -992 + val2 -992) + 992;
  out = (out < 172) ? 172 : out;
  out = (out > 1811) ? 1811 : out;
  return out;
}

void elevon_mixer() { //elevon/skid mixer used for flying wing without fc
   /*
  throttle = map(analogRead(joystick_T), 3620, 860, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = map(analogRead(joystick_Y), 714, 3680, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = map(analogRead(joystick_P), 740, 3560, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = map(analogRead(joystick_R), 3460, 920, RC_CHANNEL_MIN, RC_CHANNEL_MAX);  
*/
  throttle = ch_from_throttle(joystick_T, 650, 3505);
  yaw = ch_from_axis(joystick_Y, 3777, 550);
  pitch = ch_from_axis(joystick_P, 3705, 365);
  roll = ch_from_axis(joystick_R, 580, 3545);

  throttle = throttle + (throttle_fine - 127) * 2;
  yaw = yaw + (yaw_fine - 127) * 2;
  pitch = pitch + (pitch_fine - 127) * 2;
  roll = roll + (roll_fine - 127) * 2;

  throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[0] = yaw;
  rcChannels[1] = throttle;
  rcChannels[2] = addElevonVal(roll, pitch);
  rcChannels[3] = addElevonVal(reverseElevonVal(roll), pitch);
  rcChannels[4]=ch_from_switch(AUX1);

  rcChannels[5]=ch_from_switch(AUX2);
  rcChannels[6]=ch_from_switch(AUX3);
  rcChannels[7]=ch_from_switch(AUX4);
  //rcChannels[8]=map(digitalRead(AUX5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
 // rcChannels[9]=map(digitalRead(AUX6), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

 // rcChannels[10]=map(analogRead(POT1), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX); //for sim
  //rcChannels[11]=map(analogRead(POT2), 4096, 0, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[12]=ch_from_button(BTN_BACK); //for sim
  rcChannels[13]=ch_from_button(BTN_OK);

}
/*
void HT_on_YE() { //mixer for head tracker input from skyzone goggles

  throttle = map(analogRead(joystick_T), 3620, 860, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = map(trainer_values[7], PPM_MIN, PPM_MAX, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = map(trainer_values[8], PPM_MIN, PPM_MAX, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = map(analogRead(joystick_R), 3460, 920, RC_CHANNEL_MIN, RC_CHANNEL_MAX);  

  throttle = throttle + (throttle_fine - 127) * 2;
 
  roll = roll + (roll_fine - 127) * 2;  

  throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[0]=yaw;
  rcChannels[1]=throttle;
  rcChannels[2]=roll;
  rcChannels[3]=pitch;
  rcChannels[4]=map(digitalRead(AUX1), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[5]=map(analogRead(AUX2), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[6]=map(analogRead(AUX3), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[7]=map(digitalRead(AUX4), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[8]=map(digitalRead(AUX5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[9]=map(digitalRead(AUX6), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
}

void PPM_passthrough() { //mixer for passthrough from  trainer port - high on AUX5 means passthrough
  
  if (digitalRead(AUX5) == HIGH) {
    
    for (int i = 1; i < trainer_port_channels+1; i++) {
      rcChannels[i-1]=map(trainer_values[i], PPM_MIN, PPM_MAX, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    }
    
    rcChannels[8]=map(digitalRead(AUX5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    rcChannels[9]=map(digitalRead(AUX6), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    throttle = map(trainer_values[2], PPM_MIN, PPM_MAX, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    yaw = map(trainer_values[1], PPM_MIN, PPM_MAX, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    pitch = map(trainer_values[3], PPM_MIN, PPM_MAX, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    roll = map(trainer_values[4], PPM_MIN, PPM_MAX, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  } else {

    throttle = map(analogRead(joystick_T), 3620, 860, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    yaw = map(analogRead(joystick_Y), 714, 3680, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    pitch = map(analogRead(joystick_P), 740, 3560, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    roll = map(analogRead(joystick_R), 3460, 920, RC_CHANNEL_MIN, RC_CHANNEL_MAX);  

    throttle = throttle + (throttle_fine - 127) * 2;
    yaw = yaw + (yaw_fine - 127) * 2;
    pitch = pitch + (pitch_fine - 127) * 2;
    roll = roll + (roll_fine - 127) * 2;  

    throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

    rcChannels[0]=yaw;
    rcChannels[1]=throttle;
    rcChannels[2]=roll;
    rcChannels[3]=pitch;
    rcChannels[4]=map(digitalRead(AUX1), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    rcChannels[5]=map(analogRead(AUX2), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    rcChannels[6]=map(analogRead(AUX3), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    rcChannels[7]=map(digitalRead(AUX4), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    rcChannels[8]=map(digitalRead(AUX5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
    rcChannels[9]=map(digitalRead(AUX6), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  }
  
}

void PPM_out() { //mixer dedicated for PPM out from trainer port

  throttle = map(analogRead(joystick_T), 3620, 860, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = map(analogRead(joystick_Y), 714, 3680, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = map(analogRead(joystick_P), 740, 3560, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = map(analogRead(joystick_R), 3460, 920, RC_CHANNEL_MIN, RC_CHANNEL_MAX);  

  throttle = throttle + (throttle_fine - 127) * 2;
  yaw = yaw + (yaw_fine - 127) * 2;
  pitch = pitch + (pitch_fine - 127) * 2;
  roll = roll + (roll_fine - 127) * 2;  

  throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  ppm_out_values[0]=map(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX, PPM_MIN, PPM_MAX);
  ppm_out_values[1]=map(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX, PPM_MIN, PPM_MAX);
  ppm_out_values[2]=map(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX, PPM_MIN, PPM_MAX);
  ppm_out_values[3]=map(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX, PPM_MIN, PPM_MAX);
  ppm_out_values[4]=map(digitalRead(AUX1), 0, 1, PPM_MIN, PPM_MAX);
  ppm_out_values[5]=map(analogRead(AUX2), 0, 4096, PPM_MIN, PPM_MAX);
  ppm_out_values[6]=map(analogRead(AUX3), 0, 4096, PPM_MIN, PPM_MAX);
  ppm_out_values[7]=map(digitalRead(AUX4), 0, 1, PPM_MIN, PPM_MAX);
  ppm_out_values[8] = frame_length - (ppm_out_values[0] + ppm_out_values[1] + ppm_out_values[2] + ppm_out_values[3] + ppm_out_values[4] + ppm_out_values[5]+ ppm_out_values[6] + ppm_out_values[7]); 
  
} */
/*
void (*doMixing[number_of_mixers])() { 
  RTAE1234,
  AETR1234,
  elevon_mixer,
  HT_on_YE,
  PPM_passthrough,
  PPM_out,
  calib
}; */

void dirft_mixer() { //default mixer

  throttle = ch_from_throttle(joystick_T, 650, 3505);
  yaw = ch_from_axis(joystick_Y, 3777, 550);
  pitch = map(analogRead(joystick_P), 3705, 365, (RC_CHANNEL_MIN + 650), (RC_CHANNEL_MAX - 700));
  roll = ch_from_axis(joystick_R, 580, 3545);

  throttle = throttle + (throttle_fine - 127) * 2;
  yaw = yaw + (yaw_fine - 127) * 2;
  pitch = pitch + (pitch_fine - 127) * 2;
  roll = roll + (roll_fine - 127) * 2;

  throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[0]=yaw;
  rcChannels[1]=throttle;
  rcChannels[2]=roll;
  rcChannels[3]=pitch;
  rcChannels[4]=ch_from_switch(AUX1);

  rcChannels[5]=ch_from_switch(AUX2);
  rcChannels[6]=ch_from_switch(AUX3);
  rcChannels[7]=ch_from_switch(AUX4);
  //rcChannels[8]=map(digitalRead(AUX5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  //rcChannels[9]=map(digitalRead(AUX6), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  //rcChannels[10]=map(analogRead(POT1), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX); //for sim
  //rcChannels[11]=map(analogRead(POT2), 4096, 0, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[12]=ch_from_button(BTN_BACK); //for sim
  rcChannels[13]=ch_from_button(BTN_OK);

}
void (*doMixing[])() = {
  RTAE1234,
  AETR1234,
  elevon_mixer,
  dirft_mixer,
  calib
};

#define number_of_mixers (sizeof(doMixing) / sizeof(doMixing[0]))
static_assert(sizeof(mixer_labels) / sizeof(mixer_labels[0]) == number_of_mixers,
              "mixer_labels and doMixing must have the same number of entries");