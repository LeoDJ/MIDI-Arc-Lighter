#pragma once

#include "sfud.h"

void flashInit();
sfud_err flashRead(uint32_t addr, size_t size, uint8_t *data);
sfud_err flashWrite(uint32_t addr, size_t size, const uint8_t *data);