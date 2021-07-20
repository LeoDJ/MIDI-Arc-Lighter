#pragma once

#include "sfud.h"

void flashInit();
sfud_err flashRead(uint32_t addr, size_t size, uint8_t *data);
sfud_err flashWrite(uint32_t addr, size_t size, const uint8_t *data);
sfud_err flashReadBlock(uint32_t blockAddr, uint32_t blockLen, uint8_t *data);
sfud_err flashWriteBlock(uint32_t blockAddr, uint32_t blockLen, const uint8_t *data);
uint32_t flashGetBlockSize();
void flashLs(char *path);