#pragma once

#include <stdint.h>

#define ANALOG_SAMPLE_RATE   100     // ms
#define VIN_DIVIDER_RATIO           2       // ratio of the supply voltage resistor divider
#define AVERAGING_FACTOR            10      // 1/x moving average
#define VREFINT_VOLTAGE             3300    // mV, internal reference voltage of the STM32F103
#define ANALOG_SENSORS_INIT_TIME    1000    // ms, time after boot until data is valid

#define VREFINT_CAL_ADDR            0x1FFFF7BA  // address of calibration value, datasheet p. 19
#define VREFINT_CAL                 ((uint16_t*) VREFINT_CAL_ADDR)
//Temperature sensor raw value at 30 degrees C, VDDA=3.3V
#define TEMP30_CAL_ADDR             ((uint16_t*) ((uint32_t) 0x1FFFF7B8))
//Temperature sensor raw value at 110 degrees C, VDDA=3.3V
#define TEMP110_CAL_ADDR            ((uint16_t*) ((uint32_t) 0x1FFFF7C2))

enum adcChannelMap {
    CHAN_VBAT,
    CHAN_NTC1,
    CHAN_NTC2,
    CHAN_TEMP_INT,
    CHAN_VREF,
    NUM_ADC_CHANNELS    // leave at end
};

void analogLoop();
void analogInit();

uint16_t getVcc();          // return VCC in mV
uint16_t getAmbientLight(); // return ambient light sensor value (0-4095)