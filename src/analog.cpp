#include "analog.h"

#include <stm32f0xx_hal.h>
#include <adc.h>
#include <stdio.h>

uint16_t adcSamples[NUM_ADC_CHANNELS];
uint16_t adcSamplesIdx = 0;
bool newSamples = false;
uint32_t lastSample = 0;

void analogConvCplt() {
    // printing too much information here somehow crashes the STM
    // printf("Analog conversion complete %8d\n", HAL_GetTick());
    // printf("%4d\n", adcSamples[4]); 
    // printf("%4d, %4d, %4d, %4d, %4d\n", adcSamples[0], adcSamples[1], adcSamples[2], adcSamples[3], adcSamples[4]);
    
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc_p) {
    HAL_GPIO_WritePin(LED_2_B_GPIO_Port, LED_2_B_Pin, GPIO_PIN_SET);
    if (hadc_p == &hadc) {
        // if (__HAL_ADC_GET_FLAG(hadc_p, ADC_FLAG_EOC)) {   // end of conversion
        //     if (adcSamplesIdx < NUM_ADC_CHANNELS) {
        //         adcSamples[adcSamplesIdx++] = HAL_ADC_GetValue(hadc_p);
        //     }
        // }
        // if (__HAL_ADC_GET_FLAG(hadc_p, ADC_FLAG_EOS)) {   // end of sequence
        //     // analogConvCplt();
        //     newSamples = true;
        //     adcSamplesIdx = 0;
        // }
        newSamples = true;
        
    }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
    printf("ADC Error 0x%08lX (%ld)\n", hadc->ErrorCode, hadc->ErrorCode);
    
    HAL_GPIO_WritePin(LED_1_R_GPIO_Port, LED_1_R_Pin, GPIO_PIN_SET);
}

void analogInit() {
    printf("Init ADC...\n");
    HAL_ADC_Start_DMA(&hadc, (uint32_t *)adcSamples, NUM_ADC_CHANNELS);
    
    // HAL_ADC_Start_IT(&hadc);
}

void analogLoop() {
    if (newSamples) {
        newSamples = false;
        // ToDo: averaging
        // ToDo: vbat is off by about 80mV (3.990V instead of 4.070V measured), vdda is spot on (+-2mV)
        uint32_t vdda = VREFINT_VOLTAGE * (*VREFINT_CAL) / adcSamples[CHAN_VREF];               // calculate MCU supply voltage in mV
        uint32_t vbat = adcSamples[CHAN_VBAT] * VIN_DIVIDER_RATIO * vdda / 4095;                // calculate battery voltage in mV

        //See RM0091 section 13.9
        int32_t temperature = ((adcSamples[CHAN_TEMP_INT] * vdda) / VREFINT_VOLTAGE) - (int32_t) *TEMP30_CAL_ADDR;
        temperature *= (int32_t)(110000 - 30000);
        temperature = temperature / (int32_t)(*TEMP110_CAL_ADDR - *TEMP30_CAL_ADDR);
        temperature += 30000;

        printf("%4d, %4d, %4d, %4d, %4d\n", adcSamples[0], adcSamples[1], adcSamples[2], adcSamples[3], adcSamples[4]);
        printf("cal: %4d, vrefint: %4d, vdda: %4d, vbat: %4d, temp: %6d\n", (*VREFINT_CAL), adcSamples[CHAN_VREF], vdda, vbat, temperature);
    }

    if (HAL_GetTick() - lastSample > ANALOG_SAMPLE_RATE) {
        lastSample = HAL_GetTick();
        // HAL_ADC_Start_DMA(&hadc, (uint32_t *)adcSamples, NUM_ADC_CHANNELS);
    }
}