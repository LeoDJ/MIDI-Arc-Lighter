#include "analog.h"

#include <stm32f0xx_hal.h>
#include <adc.h>
#include <stdio.h>

uint16_t adcSamples[NUM_ADC_CHANNELS];
uint32_t adcAvgSum[NUM_ADC_CHANNELS];
bool newSamples = false;
uint32_t lastSample = 0;

void analogConvCplt() {
    // printing too much information here somehow crashes the STM
    // printf("Analog conversion complete %8d\n", HAL_GetTick());
    // printf("%4d\n", adcSamples[4]); 
    // printf("%4d, %4d, %4d, %4d, %4d\n", adcSamples[0], adcSamples[1], adcSamples[2], adcSamples[3], adcSamples[4]);
    
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc_p) {
    if (hadc_p == &hadc) {
        newSamples = true;
    }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
    printf("ADC Error 0x%08lX (%ld)\n", hadc->ErrorCode, hadc->ErrorCode);
    // HAL_GPIO_WritePin(LED_1_R_GPIO_Port, LED_1_R_Pin, GPIO_PIN_SET);
}

void analogInit() {
    printf("Init ADC...\n");
    HAL_ADC_Start_DMA(&hadc, (uint32_t *)adcSamples, NUM_ADC_CHANNELS);
    
    // HAL_ADC_Start_IT(&hadc);
}

void analogLoop() {
    if (newSamples) {
        newSamples = false;
        
        for (int i = 0; i < NUM_ADC_CHANNELS; i++) {    
            if (adcAvgSum[i] == 0) {   // initialize moving average
                adcAvgSum[i] = adcSamples[i] * AVERAGING_FACTOR;
            }
            else {
                adcAvgSum[i] -= adcAvgSum[i] / AVERAGING_FACTOR;
                adcAvgSum[i] += adcSamples[i];
            }
        }
    }

    if (HAL_GetTick() - lastSample > ANALOG_SAMPLE_RATE) {
        lastSample = HAL_GetTick();
        // HAL_ADC_Start_DMA(&hadc, (uint32_t *)adcSamples, NUM_ADC_CHANNELS);

        uint16_t avgAdcSamples[NUM_ADC_CHANNELS];
        for (int i = 0; i < NUM_ADC_CHANNELS; i++) {
            avgAdcSamples[i] = adcAvgSum[i] / AVERAGING_FACTOR;
        }

        // ToDo: vbat is off by about 80mV (3.990V instead of 4.070V measured), vdda is spot on (+-2mV)
        uint32_t vdda = VREFINT_VOLTAGE * (*VREFINT_CAL) / avgAdcSamples[CHAN_VREF];               // calculate MCU supply voltage in mV
        uint32_t vbat = avgAdcSamples[CHAN_VBAT] * VIN_DIVIDER_RATIO * vdda / 4095;                // calculate battery voltage in mV

        // Calculate temperature. See RM0091 section 13.9
        int32_t temperature = ((avgAdcSamples[CHAN_TEMP_INT] * vdda) / VREFINT_VOLTAGE) - (int32_t) *TEMP30_CAL_ADDR;
        temperature *= (int32_t)(110000 - 30000);
        temperature = temperature / (int32_t)(*TEMP110_CAL_ADDR - *TEMP30_CAL_ADDR);
        temperature += 30000;

        // printf("%4d, %4d, %4d, %4d, %4d\n", avgAdcSamples[0], avgAdcSamples[1], avgAdcSamples[2], avgAdcSamples[3], avgAdcSamples[4]);
        // printf("cal: %4d, vrefint: %4d, vdda: %4d, vbat: %4d, temp: %6d\n", (*VREFINT_CAL), avgAdcSamples[CHAN_VREF], vdda, vbat, temperature);

        // check for undervoltage
        if (vbat < BATTERY_CRITICAL_THRESHHOLD && HAL_GetTick() > ANALOG_INIT_TIME) {
            printf("UNDEVOLTAGE DETECTED! (%d mV) SHUTTING DOWN!\n", vbat);

            // set leds to red
            HAL_GPIO_WritePin(GPIOB, LED_1_G_Pin | LED_1_B_Pin | LED_2_G_Pin | LED_2_B_Pin, GPIO_PIN_SET); // all LEDs are on port B, TODO: build helper func
            HAL_GPIO_WritePin(GPIOB, LED_1_R_Pin | LED_2_R_Pin, GPIO_PIN_RESET);

            HAL_Delay(500);
            
            // shutdown STM32
            HAL_PWR_EnterSTANDBYMode();
        }
        // check for over temperature
        if (temperature > OVERTEMPERATURE_THRESHHOLD && HAL_GetTick() > ANALOG_INIT_TIME) {
            printf("OVERTEMPERATURE DETECTED! (%ld m°C) SHUTTING DOWN!\n", temperature);

            // set leds to orange
            HAL_GPIO_WritePin(GPIOB, LED_1_R_Pin | LED_1_G_Pin | LED_2_R_Pin | LED_2_G_Pin, GPIO_PIN_RESET); // all LEDs are on port B, TODO: build helper func
            HAL_GPIO_WritePin(GPIOB, LED_1_B_Pin | LED_2_B_Pin, GPIO_PIN_SET);

            HAL_Delay(500);
            
            // shutdown STM32
            HAL_PWR_EnterSTANDBYMode();
        }

        printf("%8ld System State - Bat: %4dmV, int. temp.: %d.%d C\n", HAL_GetTick(), vbat, temperature / 1000, temperature % 1000 / 100);
    }
}