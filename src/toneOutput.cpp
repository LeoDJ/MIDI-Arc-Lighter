#include "toneOutput.h"

#include "tim.h"


bool toggleChOnNextTimerUpdate[NUM_CHANNELS] = {0};


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

void toneOutputWrite(uint8_t channel, float freq, bool newNote) {
    if (channel >= 2) {
        return;
    }

    // printf("tone %d %d\n", channel, freq);

    if (freq < 20) {   // disable tone output
        toneTimers[channel]->Instance->CR1 &= ~TIM_CR1_CEN;     // disable tone timer
        toneTimers[channel]->Instance->DIER &= ~TIM_DIER_UIE;   // disable tone timer update interrupt
        toggleChOnNextTimerUpdate[channel] = false;             // reset updateFlag just in case it's being set
        switch (channel) {                                      // disable PWM channel
            case 0: TONE_MODULATION_TIMER.Instance->CCR1 = 0; break;
            case 1: TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC; break;
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

inline void toggleCh(uint8_t channel) {
    switch (channel) {
        case 0:
            if (TONE_MODULATION_TIMER.Instance->CCR1 == 0) { // toggle PWM channel 1
                TONE_MODULATION_TIMER.Instance->CCR1 = PWM_PRESC * PWM_DUTY_CYCLE / 100;
            }
            else {
                TONE_MODULATION_TIMER.Instance->CCR1 = 0;
            }
        break;
        case 1:
            if (TONE_MODULATION_TIMER.Instance->CCR2 == PWM_PRESC) { // toggle PWM channel 2 (inverted), off = max value
                TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC * (100 - PWM_DUTY_CYCLE) / 100; // inverted
            }
            else {
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
            toggleChOnNextTimerUpdate[0] = true;
        }
        else {
            toggleCh(0);
        }
    }
    else if (htim->Instance == TONE_CH2_TIMER.Instance) {
        if (!downcounting) {
            toggleChOnNextTimerUpdate[1] = true;
        }
        else {
            toggleCh(1);
        }
    }
}