
#define trainer_port_channels 8
#define PPM_MAX 2096
#define PPM_MID 1540
#define PPM_MIN 985

#define PPM_out_MAX 2000
#define PPM_out_MID 1500
#define PPM_out_MIN 1000
#define frame_length 22500 //first 8(0 - 7) channels are real channels, last (8) channel is time left in a single frame
volatile int trainer_values[trainer_port_channels + 1];
volatile long current_micros_trainer = 0;
volatile long current_tr_pulse = 0;
volatile int current_trainer_ch = 0;
volatile int ppm_out_ch;
volatile long ppm_out_values[trainer_port_channels + 1];

void PPM_in_ISR() {
    current_tr_pulse = micros() - current_micros_trainer;
  current_micros_trainer = micros();

  trainer_values[current_trainer_ch] = current_tr_pulse;

  if (current_tr_pulse > 2500 || current_trainer_ch >= 9) {
    current_trainer_ch = 0;
  }

  current_trainer_ch++;

}



