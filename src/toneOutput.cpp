#include "toneOutput.h"

#include "tim.h"

bool toggleCh1OnNextTimerUpdate = false;
bool toggleCh2OnNextTimerUpdate = false;


void toneOutputInit() {
    HAL_TIM_Base_Start_IT(&TONE_MODULATION_TIMER);
    HAL_TIM_PWM_Start(&TONE_MODULATION_TIMER, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&TONE_MODULATION_TIMER, TIM_CHANNEL_2);

    HAL_TIM_Base_Start_IT(&TONE_CH1_TIMER);
    HAL_TIM_Base_Start_IT(&TONE_CH2_TIMER);
    TONE_CH1_TIMER.Instance->CR1 &= ~TIM_CR1_CEN;   // disable timer
    TONE_CH2_TIMER.Instance->CR1 &= ~TIM_CR1_CEN;
}

const TIM_HandleTypeDef *toneTimers[] = {&TONE_CH1_TIMER, &TONE_CH2_TIMER};

void toneOutputWrite(uint8_t channel, uint16_t frequency) {
    if (channel >= 2) {
        return;
    }

    if (frequency < 20) {   // disable tone output
        toneTimers[channel]->Instance->CR1 &= ~TIM_CR1_CEN;     // disable tone timer
        switch (channel) {                                      // disable PWM channel
            case 0: TONE_MODULATION_TIMER.Instance->CCR1 = 0; break;
            case 1: TONE_MODULATION_TIMER.Instance->CCR2 = PWM_PRESC; break;
        }
    }
    else {
        toneTimers[channel]->Instance->CNT = 0;                         // reset timer counter value
        toneTimers[channel]->Instance->ARR = NOTE_FREQ / 2 / frequency; // set compare to calculated frequency
        toneTimers[channel]->Instance->CR1 |= TIM_CR1_CEN;              // enable timer
    }
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
    if (htim->Instance == TIM1) {
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
    if (htim->Instance == TIM14) {
        if(downcounting) {
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
    else if (htim->Instance == TIM15) {
        if(!downcounting) {
            toggleCh2OnNextTimerUpdate = true;
        }
        else {
            toggleCh2();
        }
    }
}