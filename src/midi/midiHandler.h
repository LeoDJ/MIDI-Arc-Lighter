#pragma once

#include <stdint.h>
#include "main.h"

void midiHandlerInit();
void midiHandlerArpLoop();
void midiHandlerUpdateNotes();

void midiHandlerNoteOn(uint8_t ch, uint8_t note, uint8_t vel);
void midiHandlerNoteOff(uint8_t ch, uint8_t note, uint8_t vel);
void midiHandlerPitchBend(uint8_t ch, uint16_t bendVal);
void midiHandlerCtrlChange(uint8_t ch, uint8_t num, uint8_t value);