#include "flash.h"

const sfud_flash *flash;

void flashInit() {
    if (sfud_init() == SFUD_SUCCESS) {
        // printf("SFUD init success!\n");
        flash = sfud_get_device_table() + 0;
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