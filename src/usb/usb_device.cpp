#include "usb_device.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_core.h"
#include "usbd_desc.h"

#include "usbd_composite.h"
#include "usbd_midi.h"
#include "usbd_midi_if.h"
#include "usbd_msc.h"
#include "usbd_storage_if.h"

USBD_HandleTypeDef hUsbDeviceFS;

void MX_USB_DEVICE_Init(void) {
    if (USBD_Init(&hUsbDeviceFS, &FS_Desc, DEVICE_FS) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_RegisterClass(&hUsbDeviceFS, &USBD_Composite_ClassDriver) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_CDC_RegisterInterface(&hUsbDeviceFS, &USBD_Interface_fops_FS) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_MIDI_RegisterInterface(&hUsbDeviceFS, &USBD_MIDI_Interface_fops_FS) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_MSC_RegisterStorage(&hUsbDeviceFS, &USBD_Storage_Interface_fops_FS) != USBD_OK) {
        Error_Handler();
    }
    if (USBD_Start(&hUsbDeviceFS) != USBD_OK) {
        Error_Handler();
    }
}

