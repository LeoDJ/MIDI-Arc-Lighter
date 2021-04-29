#pragma once

#define PWM_FREQ        22000   // Hz
#define PWM_DUTY_CYCLE  66      // %, on-time of FETs
#define NOTE_FREQ       1000000 // Hz, accuracy of note timers

#define TONE_MODULATION_TIMER   htim1
#define TONE_TIMER_0            htim14  // tone timers are mapped to hardware channels based on odd/evenness
#define TONE_TIMER_1            htim15  // to add additional tone timers, go to CubeMX, enter "NOTE_PRESC" as Prescaler and enable the gloabl TIM interrupt
// #define TONE_TIMER_2            htim16  // and add them in toneOutput.cpp
// #define TONE_TIMER_3            htim17
#define NUM_CONCURRENT_NOTES    8       // supported notes for arpeggio
#define NUM_CHANNELS            2       // hardware arc channels
#define ARPEGGIO_FREQ           30      // Hz


#define PRINTF_UART             huart1

// generated defines
#define PWM_PRESC   ((F_CPU / PWM_FREQ / 2) - 1) // /2 because of center alignment
#define NOTE_PRESC  ((F_CPU / NOTE_FREQ) - 1)
#define ARPEGGIO_MS (1000 / ARPEGGIO_FREQ)