#include "analog.h"

#include <stm32f0xx_hal.h>
#include <adc.h>
#include <stdio.h>

uint16_t adcSamples[NUM_ADC_CHANNELS];

void analogConvCplt() {
    // printing too much information here somehow crashes the STM
    // printf("Analog conversion complete %8d\n", HAL_GetTick());
    printf("%4d\n", adcSamples[0]); 
    // printf("%4d, %4d, %4d, %4d, %4d\n", adcSamples[0], adcSamples[1], adcSamples[2], adcSamples[3], adcSamples[4]);
    
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc_p) {
    HAL_GPIO_WritePin(LED_2_B_GPIO_Port, LED_2_B_Pin, GPIO_PIN_SET);
    if(hadc_p == &hadc) {
        analogConvCplt();
    }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *hadc) {
    printf("ADC Error 0x%08lX (%ld)\n", hadc->ErrorCode, hadc->ErrorCode);
    
    HAL_GPIO_WritePin(LED_1_R_GPIO_Port, LED_1_R_Pin, GPIO_PIN_SET);
}

void analogInit() {
    HAL_ADC_Start_DMA(&hadc, (uint32_t *)adcSamples, NUM_ADC_CHANNELS);
    printf("Init ADC...\n");
}