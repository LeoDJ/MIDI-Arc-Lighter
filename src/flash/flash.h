#pragma once

#include "sfud.h"
#include "fatfs.h"

void flashInit();
sfud_err flashRead(uint32_t addr, size_t size, uint8_t *data);
sfud_err flashWrite(uint32_t addr, size_t size, const uint8_t *data);
sfud_err flashReadBlock(uint32_t blockAddr, uint32_t blockLen, uint8_t *data);
sfud_err flashWriteBlock(uint32_t blockAddr, uint32_t blockLen, const uint8_t *data);
uint32_t flashGetBlockSize();
void flashLs(char *path);
void flashPrintFile(char *path);

FRESULT f_lseek_retry(FIL* fp, DWORD ofs, uint32_t timeout = 1000);
FRESULT f_read_retry(FIL* fp, void* buff, UINT btr, UINT* br, uint32_t timeout = 1000);

typedef union {
    struct {
        uint16_t day : 5;
        uint16_t month : 4;
        uint16_t year : 7;
    };
    uint16_t raw;
} FatFsDate_t;

typedef union {
    struct {
        uint8_t second2 : 5; // seconds / 2 (0-29)
        uint8_t minute : 6;
        uint8_t hour : 5;
    };
    uint16_t raw;
} FatFsTime_t;