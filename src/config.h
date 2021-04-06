#pragma once

#define PWM_FREQ    15800   // Hz
#define NOTE_FREQ   1000000 // Hz, accuracy of note timers



// generated defines
#define PWM_PRESC   ((F_CPU / PWM_FREQ / 2) - 1) // /2 because of center alignment
#define NOTE_PRESC  ((F_CPU / NOTE_FREQ) - 1)