#pragma once

#define number_of_mixers 2



const char* mixer_labels[number_of_mixers] = {"default", "elevon"};

void default_mixer() {
 
  throttle = map(analogRead(joystick_T), 3620, 860, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = map(analogRead(joystick_Y), 714, 3680, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = map(analogRead(joystick_P), 740, 3560, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = map(analogRead(joystick_R), 3460, 920, RC_CHANNEL_MIN, RC_CHANNEL_MAX);  

  throttle = throttle + (throttle_fine - 127) * 4;
  yaw = yaw + (yaw_fine - 127) * 4;
  pitch = pitch + (pitch_fine - 127) * 4;
  roll = roll + (roll_fine - 127) * 4;  

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

double reverseElevonVal(double val) {
  return (val-992) * -1 + 992;
}

double addElevonVal(double val1, double val2) {
  double out = (val1 -992 + val2 -992) + 992;
  out = (out < 172) ? 172 : out;
  out = (out > 1811) ? 1811 : out;
  return out;
}

void elevon_mixer() {
   
  throttle = map(analogRead(joystick_T), 3620, 860, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = map(analogRead(joystick_Y), 714, 3680, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = map(analogRead(joystick_P), 740, 3560, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = map(analogRead(joystick_R), 3460, 920, RC_CHANNEL_MIN, RC_CHANNEL_MAX);  

  throttle = throttle + (throttle_fine - 127) * 4;
  yaw = yaw + (yaw_fine - 127) * 4;
  pitch = pitch + (pitch_fine - 127) * 4;
  roll = roll + (roll_fine - 127) * 4;  

  throttle = constrain(throttle, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  yaw = constrain(yaw, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  pitch = constrain(pitch, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  roll = constrain(roll, RC_CHANNEL_MIN, RC_CHANNEL_MAX);

  rcChannels[0] = yaw;
  rcChannels[1] = throttle;
  rcChannels[2] = addElevonVal(roll, pitch);;
  rcChannels[3] = addElevonVal(reverseElevonVal(roll), pitch);
  rcChannels[4] = map(digitalRead(AUX1), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[5] = map(analogRead(AUX2), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[6] = map(analogRead(AUX3), 0, 4096, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[7] = map(digitalRead(AUX4), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[8] = map(digitalRead(AUX5), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
  rcChannels[9] = map(digitalRead(AUX6), 0, 1, RC_CHANNEL_MIN, RC_CHANNEL_MAX);
   
}

void (*doMixing[number_of_mixers])() { 
  default_mixer,
  elevon_mixer
};