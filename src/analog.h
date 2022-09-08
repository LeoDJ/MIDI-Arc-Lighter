#pragma once

#include <stdint.h>

#define ANALOG_SENSOR_SAMPLE_RATE   100     // ms
#define VIN_DIVIDER_RATIO           2       // ratio of the supply voltage resistor divider
#define AVERAGING_FACTOR            10      // 1/x moving average
#define VREFINT_VOLTAGE             1200    // mV, internal reference voltage of the STM32F103
#define ANALOG_SENSORS_INIT_TIME    1000    // ms, time after boot until data is valid

enum adcChannelMap {
    VBAT,
    NTC1,
    NTC2,
    TEMP_INT,
    VREF,
    NUM_ADC_CHANNELS    // leave at end
};

void analogLoop();
void analogInit();

uint16_t getVcc();          // return VCC in mV
uint16_t getAmbientLight(); // return ambient light sensor value (0-4095)