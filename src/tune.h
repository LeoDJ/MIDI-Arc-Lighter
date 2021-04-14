// this can be a simple .h file, because it's only included in one cpp file (main.cpp)
#include "main.h"
#include "toneOutput.h"


#define f4  349
#define f4s 370
#define g4  392
#define g4s 415
#define a4  440
#define a4s 466
#define b4  494
#define c5  523
#define c5s 554
#define d5  587
#define d5s 622
#define e5  659
#define f5  698
#define f5s 740
#define g5  784
#define g5s 831

// Zelda chest opening sound
uint16_t ch1Notes[] =           {g4,  a4,  b4,  c5s, 0,   0,   0,   0,   g4s, a4s, c5,  d5,  0,   0,   0,   0,   a4,  b4,  c5s, d5s, 0,   0,   0,   0,   a4s, c5,  d5,  e5,  0,   0,   0,   0,   b4,  c5s, d5s, f5,  0,   0,   0,   0,   c5s, d5s, f5s, g5,  0,   0,   0,   0,   0,   c5,  c5s, d5,  d5s, 0};
uint16_t ch2Notes[] =           {0,   0,   0,   0,   g4,  a4,  b4,  c5s, 0,   0,   0,   0,   g4s, a4s, c5,  d5,  0,   0,   0,   0,   a4,  b4,  c5s, d5s, 0,   0,   0,   0,   a4s, c5,  d5,  e5,  0,   0,   0,   0,   c5,  d5,  e5,  f5s, 0,   0,   0,   0,   d5,  e5,  f5s, g5s, 0,   f4,  f4s, g4,  g4s, 0};
#define NUM_NOTES (sizeof(ch1Notes) / sizeof(ch1Notes[0]))
uint16_t timings[NUM_NOTES] =   {104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 104, 416, 208, 208, 208, 624, 3000};

void playTune() {
    for (int i = 0; i < NUM_NOTES; i++) {
        toneOutputFreq(0, ch1Notes[i]);
        toneOutputFreq(1, ch2Notes[i]);
        HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
        HAL_Delay(timings[i]);
    }
}