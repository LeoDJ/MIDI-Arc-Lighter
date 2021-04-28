#include "toneOutput.h"

#include "tim.h"


bool toggleChOnNextTimerUpdate[NUM_CHANNELS] = {0};

TIM_HandleTypeDef *toneTimers[] = {&TONE_TIMER_0, &TONE_TIMER_1, &TONE_TIMER_2, &TONE_TIMER_3};
const int numToneTimers = sizeof(toneTimers) / sizeof(toneTimers[0]);
bool toneTimerOutputState[numToneTimers];

void toneOutputInit() {
    // need to initialize the tone timers first, so they don't trigger an interrupt and switch on the arc on init
    for (int i = 0; i < numToneTimers; i++) {
        HAL_TIM_Base_Start_IT(toneTimers[i]);           // initialize timer
        toneTimers[i]->Instance->CR1 &= ~TIM_CR1_CEN;   // disable timer
    }

    HAL_TIM_Base_Start_IT(&TONE_MODULATION_TIMER);
    HAL_TIM_PWM_Start(&TONE_MODULATION_TIMER, TIM_CHANNEL_1);
    TONE_MODULATION_TIMER.Instance->CCR1 = 0;
    HAL_TIM_PWM_Start(&TONE_MODULATION_TIMER, TIM_CHANNEL_2);
    TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC; // inverted channel, off = max value
}


void toneOutputWrite(uint8_t channel, float freq, bool newNote) {
    if (channel >= NUM_CONCURRENT_NOTES) {
        return;
    }

    // printf("\ntone %d %d\n", channel, freq);

    if (freq < 20) {   // disable tone output
        toneTimers[channel]->Instance->CR1 &= ~TIM_CR1_CEN;     // disable tone timer
        toneTimers[channel]->Instance->DIER &= ~TIM_DIER_UIE;   // disable tone timer update interrupt
        toneTimerOutputState[channel] = false;                  // clear channel output status flag

        // check if all tone timers for given hardware channel are off
        uint8_t hwChannel = channel % NUM_CHANNELS;
        bool allTonesOnChSilent = true;
        for (int i = 0; i < numToneTimers / 2; i++) {
            uint8_t timerIdx = hwChannel + (i * NUM_CHANNELS);
            if (toneTimers[timerIdx]->Instance->CR1 & TIM_CR1_CEN) {
                allTonesOnChSilent = false;
                break;
            }
        }

        // when no tone is playing anymore on hardware channel, disable it
        if (allTonesOnChSilent) {
            toggleChOnNextTimerUpdate[channel] = false;             // reset updateFlag just in case it's being set
            switch (hwChannel) {                                    // disable PWM channel
                case 0: TONE_MODULATION_TIMER.Instance->CCR1 = 0; break;
                case 1: TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC; break;
            }
        }
    }
    else {
        toneTimers[channel]->Instance->ARR = NOTE_FREQ / freq / 2;  // set compare to calculated frequency
        if (newNote) {
            // toneTimers[channel]->Instance->EGR |= TIM_EGR_UG;       // generate update event to load new ARR value from preload register
            toneTimers[channel]->Instance->CNT = 0;                 // reset timer counter value
            toneTimers[channel]->Instance->DIER |= TIM_DIER_UIE;    // enable tone timer update interrupt
            toneTimers[channel]->Instance->CR1 |= TIM_CR1_CEN;      // enable timer
            
        }
    }
}

typedef struct {
    uint8_t activeTimerCount;   // number of timers currently running
    uint8_t timerOutputCount;   // number of timers that are currently outputting 1
} activeTimerCount_t;

inline activeTimerCount_t getActiveTimersForHwChannel(uint8_t hwChannel) {
    uint8_t activeTimers = 0;
    uint8_t activeTimerOutputs = 0;
    for (int i = 0; i < numToneTimers / 2; i++) {
        uint8_t timerIdx = hwChannel + (i * NUM_CHANNELS);
        if (toneTimers[timerIdx]->Instance->CR1 & TIM_CR1_CEN) {
            activeTimers++;
        }
        if (toneTimerOutputState[timerIdx]) {
            activeTimerOutputs++;
        }
    }
    return {activeTimers, activeTimerOutputs};
}

#define POLYPHONY_PERCENT   10

inline void toggleCh(uint8_t channel) {
    // printf("%d %d %d %d\n", 
    //     toneTimerOutputState[0],
    //     toneTimerOutputState[1],
    //     toneTimerOutputState[2],
    //     toneTimerOutputState[3]);
    activeTimerCount_t state = getActiveTimersForHwChannel(channel);
    uint32_t duty = (state.activeTimerCount == 2) ? (PWM_DUTY_CYCLE - POLYPHONY_PERCENT + POLYPHONY_PERCENT * (state.timerOutputCount)) : (PWM_DUTY_CYCLE);
    printf("ch: %d, duty: %2d, timers: %d, on: %d\n", channel, duty, state.activeTimerCount, state.timerOutputCount);
    switch (channel) {
        case 0:
            if (state.timerOutputCount || state.activeTimerCount == 2) { // toggle PWM channel 1
                TONE_MODULATION_TIMER.Instance->CCR1 = PWM_PRESC * duty / 100;
            }
            else {
                // if (!toneTimerOutputState[0] && !toneTimerOutputState[2])
                    TONE_MODULATION_TIMER.Instance->CCR1 = 0;
            }
        break;
        case 1:
            if (state.timerOutputCount || state.activeTimerCount == 2) { // toggle PWM channel 2 (inverted), off = max value
                TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC * (100 - duty) / 100; // inverted
            }
            else {
                // if (!toneTimerOutputState[1] && !toneTimerOutputState[3])
                TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC; // inverted channel, off = max value
            }
        break;
    }
}


void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TONE_MODULATION_TIMER.Instance) {
        for (int i = 0; i < NUM_CHANNELS; i++) {
            if (toggleChOnNextTimerUpdate[i]) {
                toggleChOnNextTimerUpdate[i] = false;
                toggleCh(i);
            }
        }
        return;
    }

    bool downcounting = TONE_MODULATION_TIMER.Instance->CR1 & TIM_CR1_DIR;
    for (int timIdx = 0; timIdx < numToneTimers; timIdx++) {
        if (htim->Instance == toneTimers[timIdx]->Instance) {
            uint8_t hwChannel = timIdx % NUM_CHANNELS;
            toneTimerOutputState[timIdx] = !toneTimerOutputState[timIdx];   // toggle timer output state
            if (downcounting) {
                /**
                 * Because of center aligned PWM, the capture compare register values would be loaded 
                 * at an inconvenient time, despite having CCR preload enabled.
                 * This is because center aligned PWM will generate an update event every time it switches counting directions.
                 * So the preloaded CCR value will be transferred into the CCR register on the next update, despite the direction.
                 * This leads to glitched PWM pulses with only half the supposed length.
                 * The workaround is to wait for the next TIM1 update event and toggle the PWM channel then.
                 */
                toggleChOnNextTimerUpdate[hwChannel] = true;
            }
            else {
                toggleCh(hwChannel);
            }
            break; // break out of for loop
        }
    }
}