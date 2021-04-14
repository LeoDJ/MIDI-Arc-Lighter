#include "toneOutput.h"

#include "tim.h"

bool toggleCh1OnNextTimerUpdate = false;
bool toggleCh2OnNextTimerUpdate = false;

// MIDI note frequencies multiplied by 5 for a bit more accuracy
#define MIDI_FREQ_MULTIPLIER    5
const uint16_t midiNoteFreq[] = {
    41,    43,    46,    49,    52,    55,    58,    61,    65,    69,    73,
    77,    82,    87,    92,    97,    103,   109,   116,   122,   130,   138,
    146,   154,   164,   173,   184,   194,   206,   218,   231,   245,   260,
    275,   291,   309,   327,   346,   367,   389,   412,   437,   462,   490,
    519,   550,   583,   617,   654,   693,   734,   778,   824,   873,   925,
    980,   1038,  1100,  1165,  1235,  1308,  1386,  1468,  1556,  1648,  1746,
    1850,  1960,  2076,  2200,  2331,  2469,  2616,  2772,  2937,  3111,  3296,
    3492,  3700,  3920,  4153,  4400,  4662,  4939,  5232,  5544,  5873,  6223,
    6593,  6985,  7400,  7840,  8306,  8800,  9323,  9878,  10465, 11087, 11747,
    12445, 13185, 13969, 14800, 15680, 16612, 17600, 18647, 19755, 20930, 22175,
    23493, 24890, 26370, 27938, 29600, 31360, 33224, 35200, 37293, 39511, 41860,
    44349, 46986, 49780, 52740, 55876, 59199, 62719};


void toneOutputInit() {
    // need to initialize the tone timers first, so they don't trigger an interrupt and switch on the arc on init
    HAL_TIM_Base_Start_IT(&TONE_CH1_TIMER);
    HAL_TIM_Base_Start_IT(&TONE_CH2_TIMER);
    TONE_CH1_TIMER.Instance->CR1 &= ~TIM_CR1_CEN;   // disable timer
    TONE_CH2_TIMER.Instance->CR1 &= ~TIM_CR1_CEN;

    HAL_TIM_Base_Start_IT(&TONE_MODULATION_TIMER);
    HAL_TIM_PWM_Start(&TONE_MODULATION_TIMER, TIM_CHANNEL_1);
    TONE_MODULATION_TIMER.Instance->CCR1 = 0;
    HAL_TIM_PWM_Start(&TONE_MODULATION_TIMER, TIM_CHANNEL_2);
    TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC; // inverted channel, off = max value
}

const TIM_HandleTypeDef *toneTimers[] = {&TONE_CH1_TIMER, &TONE_CH2_TIMER};

// multiplied frequency
void toneOutputWrite(uint8_t channel, uint16_t freq) {
    if (channel >= 2) {
        return;
    }

    // printf("tone %d %d\n", channel, freq);

    if (freq < 20 * MIDI_FREQ_MULTIPLIER) {   // disable tone output
        toneTimers[channel]->Instance->CR1 &= ~TIM_CR1_CEN;     // disable tone timer
        switch (channel) {                                      // disable PWM channel
            case 0: TONE_MODULATION_TIMER.Instance->CCR1 = 0; break;
            case 1: TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC; break;
        }
    }
    else {
        toneTimers[channel]->Instance->CNT = 0;             // reset timer counter value
        toneTimers[channel]->Instance->ARR = NOTE_FREQ * MIDI_FREQ_MULTIPLIER / freq / 2;   // set compare to calculated frequency
        toneTimers[channel]->Instance->CR1 |= TIM_CR1_CEN;  // enable timer
    }
}

void toneOutputFreq(uint8_t channel, uint16_t frequency) {
    toneOutputWrite(channel, frequency * MIDI_FREQ_MULTIPLIER);
}

void toneOutputNote(uint8_t channel, uint8_t midiNote) {
    if (midiNote > 127) {
        toneOutputWrite(channel, 0); // turn off note for invalid note
        return;
    }

    toneOutputWrite(channel, midiNoteFreq[midiNote]);
}

inline void toggleCh1() {
    if (TONE_MODULATION_TIMER.Instance->CCR1 == 0) { // toggle PWM channel 1
        TONE_MODULATION_TIMER.Instance->CCR1 = PWM_PRESC * PWM_DUTY_CYCLE / 100;
    }
    else {
        TONE_MODULATION_TIMER.Instance->CCR1 = 0;
    }
}

inline void toggleCh2() {
    if (TONE_MODULATION_TIMER.Instance->CCR2 == PWM_PRESC) { // toggle PWM channel 2 (inverted), off = max value
        TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC * (100 - PWM_DUTY_CYCLE) / 100; // inverted
    }
    else {
        TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC; // inverted channel, off = max value
    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TONE_MODULATION_TIMER.Instance) {
        if (toggleCh1OnNextTimerUpdate) {
            toggleCh1OnNextTimerUpdate = false;
            toggleCh1();
        }
        else if (toggleCh2OnNextTimerUpdate) {
            toggleCh2OnNextTimerUpdate = false;
            toggleCh2();
        }
        return;
    }

    bool downcounting = TONE_MODULATION_TIMER.Instance->CR1 & TIM_CR1_DIR;
    if (htim->Instance == TONE_CH1_TIMER.Instance) {
        if (downcounting) {
            /**
             * Because of center aligned PWM, the capture compare register values would be loaded 
             * at an inconvenient time, despite having CCR preload enabled.
             * This is because center aligned PWM will generate an update event every time it switches counting directions.
             * So the preloaded CCR value will be transferred into the CCR register on the next update, despite the direction.
             * This leads to glitched PWM pulses with only half the supposed length.
             * The workaround is to wait for the next TIM1 update event and toggle the PWM channel then.
             */
            toggleCh1OnNextTimerUpdate = true;
        }
        else {
            toggleCh1();
        }
    }
    else if (htim->Instance == TONE_CH2_TIMER.Instance) {
        if (!downcounting) {
            toggleCh2OnNextTimerUpdate = true;
        }
        else {
            toggleCh2();
        }
    }
}