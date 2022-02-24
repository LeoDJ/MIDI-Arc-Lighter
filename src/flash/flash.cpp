#include "flash.h"
#include <string.h>
#include "util.h"

const sfud_flash *flash;
volatile bool flashBusy = false;

void flashInit() {
    if (sfud_init() == SFUD_SUCCESS) {
        // printf("SFUD init success!\n");
        flash = sfud_get_device_table() + 0;
    }
    FRESULT res = f_mount(&USERFatFS, "", 0);
    if (res != FR_OK) {
        printf("[FLASH] FS mount status: %d\n", res);
    }
}

sfud_err flashRead(uint32_t addr, size_t size, uint8_t *data) {
    if (flash) {
        return sfud_read(flash, addr, size, data);
    }
    return SFUD_ERR_NOT_FOUND;
}

sfud_err flashWrite(uint32_t addr, size_t size, const uint8_t *data) {
    if (flash) {
        return sfud_erase_write(flash, addr, size, data);
    }
    return SFUD_ERR_NOT_FOUND;
}

sfud_err flashReadBlock(uint32_t blockAddr, uint32_t blockLen, uint8_t *data) {
    if (!flashBusy) {
        flashBusy = true;
        uint32_t startAddr = blockAddr * flashGetBlockSize();
        uint32_t len = blockLen * flashGetBlockSize();
        // printf("[FLASH] Free Heap: %d\n", estimateFreeHeap(16));
        sfud_err res = flashRead(startAddr, len, data);
        flashBusy = false;
        return res;
    }
    else {
        return SFUD_ERR_READ;
    }
}

sfud_err flashWriteBlock(uint32_t blockAddr, uint32_t blockLen, const uint8_t *data) {
    if (!flashBusy) {
        flashBusy = true;
        uint32_t startAddr = blockAddr * flashGetBlockSize();
        uint32_t len = blockLen * flashGetBlockSize();
        sfud_err res = flashWrite(startAddr, len, data);
        flashBusy = false;
        return res;
    }
    else {
        return SFUD_ERR_WRITE;
    }
}

uint32_t flashGetBlockSize() {
    return flash->chip.erase_gran;
}

FRESULT scanFiles(char* path) {
    FRESULT res;
    DIR dir;
    static FILINFO fInfo;

    res = f_opendir(&dir, path);
    if (res == FR_OK) {
        while (true) {
            res = f_readdir(&dir, &fInfo);              // read next directory item
            if (res != FR_OK || fInfo.fname[0] == 0) {  // break at end of directory
                break;
            }
            if (fInfo.fattrib & AM_DIR) {               // if directory
                UINT pathLen = strlen(path);
                sprintf(&path[pathLen], "/%s", fInfo.fname);
                res = scanFiles(path);                  // recurse folder;
                if (res != FR_OK) {
                    break;
                }
                path[pathLen] = 0;                      // clear slash to restore path
            }
            else {
                FatFsDate_t date = {.raw = fInfo.fdate};
                FatFsTime_t time = {.raw = fInfo.ftime};
                printf("%d-%02d-%02d %02d:%02d:%02d  %8ld  %s/%s\n", (uint16_t)date.year + 1980, date.month, date.day, time.hour, time.minute, time.second2 * 2, fInfo.fsize, path, fInfo.fname);

            }
        }
    }
    else {
        printf("[ls] couldn't open directory %s err: %d\n", path, res);
    }
    res = f_closedir(&dir);
    return res;
}

void flashLs(char *path) {
    char pathBuf[256];
    strcpy(pathBuf, path);
    scanFiles(pathBuf);
}

void flashPrintFile(char *path) {
    FIL file;
    FRESULT res = f_open(&file, path, FA_READ);
    if (res == FR_OK) {
        const uint16_t chunkSize = 64;
        char buf[chunkSize];
        unsigned int numRead = chunkSize;
        do {
            res = f_read(&file, buf, chunkSize, &numRead);
            if (res == FR_OK) {
                fwrite(buf, 1, numRead, stdout);
            }
            else {
                printf("File read failed. Err %d\n", res);
                break;
            }
        } while (numRead == chunkSize);
        printf("\n");
    }
    else {
        printf("Coudln't open file. Err %d\n", res);
    }
    f_close(&file);
}

/**
 *  @param fp Pointer to the file object
 *  @param ofs File pointer from top of file
 *  @param timeout Retry timeout in ms
 */
FRESULT f_lseek_retry(FIL* fp, DWORD ofs, uint32_t timeout) {
    uint32_t start = HAL_GetTick();
    FRESULT res;
    do {
        res = f_lseek(fp, ofs);
        // printf("f_lseek %d\n", res);
    } while (res == FR_DISK_ERR && HAL_GetTick() - start <= timeout);
    return res;
}

/**
 *  @param fp Pointer to the file object
 *  @param buff Pointer to data buffer
 *  @param btr Number of bytes to read
 *  @param br Pointer to number of bytes read
 *  @param timeout Retry timeout in ms
 */
FRESULT f_read_retry(FIL* fp, void* buff, UINT btr, UINT* br, uint32_t timeout) {
    uint32_t start = HAL_GetTick();
    FRESULT res;
    do {
        res = f_read(fp, buff, btr, br);
        // printf("f_read %d\n", res);
    } while (res == FR_DISK_ERR && HAL_GetTick() - start <= timeout);
    return res;
}