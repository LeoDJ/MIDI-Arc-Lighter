#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include  "usbd_ioreq.h"


#define USB_Composite_CONFIG_DESC_SIZ      (9 + 8 + 58 + 8 + 92 + 23) // configuration descr + IAD + CDC descr + IAD + MIDI descr + MSC descr

#define IN_EP_DIR   0x80 // Direction bit

#define COMP_INTERFACE_IDX_CDC          0x00
#define COMP_INTERFACE_IDX_CDC_DATA     0x01
#define COMP_INTERFACE_IDX_MIDI         0x02
#define COMP_INTERFACE_IDX_MIDI_STREAM  0x03
#define COMP_INTERFACE_IDX_MSC          0x04

#define COMP_EP_IDX_CDC             0x02
#define COMP_EP_IDX_CDC_CMD         0x03
#define COMP_EP_IDX_MIDI            0x01    // don't know why, but can't use 3 for MIDI EP ??
#define COMP_EP_IDX_MSC             0x04

#define COMP_EP_IDX_CDC_IN          (COMP_EP_IDX_CDC | IN_EP_DIR)
#define COMP_EP_IDX_CDC_CMD_IN      (COMP_EP_IDX_CDC_CMD | IN_EP_DIR)
#define COMP_EP_IDX_MIDI_IN         (COMP_EP_IDX_MIDI | IN_EP_DIR)
#define COMP_EP_IDX_MSC_IN          (COMP_EP_IDX_MSC | IN_EP_DIR)



extern USBD_ClassTypeDef  USBD_Composite_ClassDriver;


#ifdef __cplusplus
}
#endif
