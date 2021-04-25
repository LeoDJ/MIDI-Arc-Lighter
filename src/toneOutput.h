#pragma once

#include "main.h"
#include "stdio.h"

void toneOutputInit();
void toneOutputWrite(uint8_t channel, float freq, bool newNote = true);