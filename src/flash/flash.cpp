#include "flash.h"
#include "fatfs.h"
#include <string.h>

const sfud_flash *flash;

void flashInit() {
    if (sfud_init() == SFUD_SUCCESS) {
        // printf("SFUD init success!\n");
        flash = sfud_get_device_table() + 0;
    }
    f_mount(&USERFatFS, "", 0);
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
    uint32_t startAddr = blockAddr * flashGetBlockSize();
    uint32_t len = blockLen * flashGetBlockSize();
    return flashRead(startAddr, len, data);
}

sfud_err flashWriteBlock(uint32_t blockAddr, uint32_t blockLen, const uint8_t *data) {
    uint32_t startAddr = blockAddr * flashGetBlockSize();
    uint32_t len = blockLen * flashGetBlockSize();
    return flashWrite(startAddr, len, data);
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
                printf("%s/%s\n", path, fInfo.fname);
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