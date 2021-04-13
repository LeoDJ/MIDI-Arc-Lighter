#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include  "usbd_ioreq.h"


#define USB_Composite_CONFIG_DESC_SIZ      (9 + 8 + 58 + 8 + 92) // configuration descr + IAD + CDC descr + IAD + MIDI descr

#define IN_EP_DIR   0x80 // Direction bit

#define COMP_INTERFACE_IDX_CDC          0x00
#define COMP_INTERFACE_IDX_CDC_DATA     0x01
#define COMP_INTERFACE_IDX_MIDI         0x02
#define COMP_INTERFACE_IDX_MIDI_STREAM  0x03

#define COMP_EP_IDX_CDC             0x01
#define COMP_EP_IDX_CDC_CMD         0x02
#define COMP_EP_IDX_MIDI            0x03
#define COMP_EP_IDX_MIDI_IN         COMP_EP_IDX_MIDI | IN_EP_DIR




extern USBD_ClassTypeDef  USBD_Composite_ClassDriver;


#ifdef __cplusplus
}
#endif
