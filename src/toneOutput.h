#pragma once

#include "main.h"
#include "stdio.h"

void toneOutputInit();
// void toneOutputWrite(uint8_t channel, uint16_t freq);
void toneOutputNote(uint8_t channel, uint8_t midiNote);
void toneOutputFreq(uint8_t channel, uint16_t frequency, bool newNote = true);


#define MIDI_FREQ_MULTIPLIER    5