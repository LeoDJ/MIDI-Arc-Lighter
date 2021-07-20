#include "ff_gen_drv.h"
#include "flash.h"

DSTATUS fsInit(BYTE pdrv) {
    return 0;
}

DSTATUS fsStatus(BYTE pdrv) {
    return 0;
}

/**
  * @brief  Reads Sector(s) 
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data buffer to store read data
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to read (1..128)
  * @retval DRESULT: Operation result
  */
DRESULT fsRead(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
    sfud_err res = flashReadBlock(sector, count, buff);
    return (res == SFUD_SUCCESS) ? RES_OK : RES_ERROR;
}

/**
  * @brief  Writes Sector(s)  
  * @param  pdrv: Physical drive number (0..)
  * @param  *buff: Data to be written
  * @param  sector: Sector address (LBA)
  * @param  count: Number of sectors to write (1..128)
  * @retval DRESULT: Operation result
  */
#if _USE_WRITE == 1
DRESULT fsWrite(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
    sfud_err res = flashWriteBlock(sector, count, buff);
    return (res == SFUD_SUCCESS) ? RES_OK : RES_ERROR;
}
#endif

/**
  * @brief  I/O control operation  
  * @param  pdrv: Physical drive number (0..)
  * @param  cmd: Control code
  * @param  *buff: Buffer to send/receive control data
  * @retval DRESULT: Operation result
  */
#if _USE_IOCTL == 1
DRESULT fsIoctl (BYTE pdrv, BYTE cmd, void *buff) {
    DRESULT res = RES_ERROR;
    return res;
}
#endif


Diskio_drvTypeDef USER_Driver = {
    fsInit,
    fsStatus,
    fsRead,
    #if _USE_WRITE
        fsWrite,
    #endif
    #if _USE_IOCTL == 1
        fsIoctl
    #endif
};