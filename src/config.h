#pragma once

#define PWM_FREQ        18000   // Hz
#define PWM_DUTY_CYCLE  66      // %, on-time of FETs
#define NOTE_FREQ       1000000 // Hz, accuracy of note timers

#define TONE_MODULATION_TIMER   htim1
#define TONE_CH1_TIMER          htim14
#define TONE_CH2_TIMER          htim15

#define PRINTF_UART             huart1

// generated defines
#define PWM_PRESC   ((F_CPU / PWM_FREQ / 2) - 1) // /2 because of center alignment
#define NOTE_PRESC  ((F_CPU / NOTE_FREQ) - 1)