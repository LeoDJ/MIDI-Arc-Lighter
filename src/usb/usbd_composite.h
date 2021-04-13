#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include  "usbd_ioreq.h"


#define USB_Composite_CONFIG_DESC_SIZ      (9 + 8 + 58) // configuration descr + IAD + CDC descr


#define COMP_INTERFACE_IDX_CDC      0x00
#define COMP_INTERFACE_IDX_CDC_DATA 0x01

#define COMP_EP_IDX_CDC             0x00
#define COMP_EP_IDX_CDC_CMD         0x01


#define IN_EP_DIR   0x80 // Direction bit


extern USBD_ClassTypeDef  USBD_Composite_ClassDriver;


#ifdef __cplusplus
}
#endif
