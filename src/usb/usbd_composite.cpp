/**
 * Composite USB Class 
 * Made from the STM32 usbd_template.c
 * And this tutorial: https://sudonull.com/post/68144-CDC-MSC-USB-Composite-Device-on-STM32-HAL
 */

#include "usbd_composite.h"
#include "usbd_ctlreq.h"
#include "usbd_cdc.h"
#include "usbd_midi.h"


static uint8_t USBD_Composite_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_Composite_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_Composite_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t *USBD_Composite_GetCfgDesc(uint16_t *length);
static uint8_t *USBD_Composite_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t USBD_Composite_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_Composite_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_Composite_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_Composite_EP0_TxReady(USBD_HandleTypeDef *pdev);
static uint8_t USBD_Composite_SOF(USBD_HandleTypeDef *pdev);
static uint8_t USBD_Composite_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USBD_Composite_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);


USBD_ClassTypeDef  USBD_Composite_ClassDriver = {
    USBD_Composite_Init,
    USBD_Composite_DeInit,
    USBD_Composite_Setup,
    NULL, // USBD_Composite_EP0_TxReady,
    USBD_Composite_EP0_RxReady,
    USBD_Composite_DataIn,
    USBD_Composite_DataOut,
    NULL, // USBD_Composite_SOF,
    NULL, // USBD_Composite_IsoINIncomplete,
    NULL, // USBD_Composite_IsoOutIncomplete,
    USBD_Composite_GetCfgDesc,
    USBD_Composite_GetCfgDesc,
    USBD_Composite_GetCfgDesc,
    USBD_Composite_GetDeviceQualifierDesc,
};

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma data_alignment=4
#endif
/* USB TEMPLATE device Configuration Descriptor */
static uint8_t USBD_Composite_CfgDesc[USB_Composite_CONFIG_DESC_SIZ] = {
    0x09,                           /* bLength: Configuation Descriptor size */
    USB_DESC_TYPE_CONFIGURATION,    /* bDescriptorType: Configuration */
    USB_Composite_CONFIG_DESC_SIZ,  /* wTotalLength: Bytes returned */
    0x00,
    0x04,               	        /*bNumInterfaces: 4 interfaces (CDC + MIDI) */
    0x01,               	        /*bConfigurationValue: Configuration value*/
    0x02,               	        /*iConfiguration: Index of string descriptor describing the configuration*/
    0xC0,               	        /*bmAttributes: bus powered and Supports Remote Wakeup */
    0x32,               	        /*MaxPower 100 mA: this current is used for detecting Vbus*/



#if 1   // CDC
    /*---------------------------------------------------------------------------*/
    // IAD descriptor for CDC interfaces

    0x08,                   /* bLength */
	0x0B,                   /* bDescriptorType: IAD */
	COMP_INTERFACE_IDX_CDC, /* bFirstInterface */
	0x02,                   /* bInterfaceCount: 2 for CDC */
	0x02,                   /* bFunctionClass: Communication Interface Class */
	0x02,                   /* bFunctionSubClass: Abstract Control Model */
	0x01,                   /* bFunctionProtocol: Common AT commands */
	0x00,                   /* iFunction (Index of string descriptor describing this function) */

    /*---------------------------------------------------------------------------*/
    // CDC Descriptor

    /*Interface Descriptor */
    0x09,                   /* bLength: Interface Descriptor size */
    USB_DESC_TYPE_INTERFACE, /* bDescriptorType: Interface */
    /* Interface descriptor type */
    COMP_INTERFACE_IDX_CDC, /* bInterfaceNumber: Number of Interface */
    0x00,                   /* bAlternateSetting: Alternate setting */
    0x01,                   /* bNumEndpoints: One endpoints used */
    0x02,                   /* bInterfaceClass: Communication Interface Class */
    0x02,                   /* bInterfaceSubClass: Abstract Control Model */
    0x01,                   /* bInterfaceProtocol: Common AT commands */
    0x00,                   /* iInterface: */

    /*Header Functional Descriptor*/
    0x05,   /* bLength: Endpoint Descriptor size */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x00,   /* bDescriptorSubtype: Header Func Desc */
    0x10,   /* bcdCDC: spec release number */
    0x01,

    /*Call Management Functional Descriptor*/
    0x05,                           /* bFunctionLength */
    0x24,                           /* bDescriptorType: CS_INTERFACE */
    0x01,                           /* bDescriptorSubtype: Call Management Func Desc */
    0x00,                           /* bmCapabilities: D0+D1 */
    COMP_INTERFACE_IDX_CDC_DATA,    /* bDataInterface: 1 */

    /*ACM Functional Descriptor*/
    0x04,   /* bFunctionLength */
    0x24,   /* bDescriptorType: CS_INTERFACE */
    0x02,   /* bDescriptorSubtype: Abstract Control Management desc */
    0x02,   /* bmCapabilities */

    /*Union Functional Descriptor*/
    0x05,                           /* bFunctionLength */
    0x24,                           /* bDescriptorType: CS_INTERFACE */
    0x06,                           /* bDescriptorSubtype: Union func desc */
    COMP_INTERFACE_IDX_CDC,         /* bMasterInterface: Communication class interface */
    COMP_INTERFACE_IDX_CDC_DATA,    /* bSlaveInterface0: Data Class Interface */

    /*Endpoint 2 Descriptor*/
    0x07,                           /* bLength: Endpoint Descriptor size */
    USB_DESC_TYPE_ENDPOINT,         /* bDescriptorType: Endpoint */
    COMP_EP_IDX_CDC_CMD_IN,         /* bEndpointAddress */
    0x03,                           /* bmAttributes: Interrupt */
    LOBYTE(CDC_CMD_PACKET_SIZE),    /* wMaxPacketSize: */
    HIBYTE(CDC_CMD_PACKET_SIZE),
    CDC_FS_BINTERVAL,               /* bInterval: */

    /*-------------------------*/
    // CDC Data class interface descriptor
    
    0x09,                           /* bLength: Endpoint Descriptor size */
    USB_DESC_TYPE_INTERFACE,        /* bDescriptorType: */
    COMP_INTERFACE_IDX_CDC_DATA,    /* bInterfaceNumber: Number of Interface */
    0x00,                           /* bAlternateSetting: Alternate setting */
    0x02,                           /* bNumEndpoints: Two endpoints used */
    0x0A,                           /* bInterfaceClass: CDC */
    0x00,                           /* bInterfaceSubClass: */
    0x00,                           /* bInterfaceProtocol: */
    0x00,                           /* iInterface: */

    /*Endpoint OUT Descriptor*/
    0x07,                                   /* bLength: Endpoint Descriptor size */
    USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint */
    COMP_EP_IDX_CDC,                        /* bEndpointAddress */
    0x02,                                   /* bmAttributes: Bulk */
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize: */
    HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    
    0x00,                                   /* bInterval: ignore for Bulk transfer */

    /*Endpoint IN Descriptor*/
    0x07,                                   /* bLength: Endpoint Descriptor size */
    USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint */
    COMP_EP_IDX_CDC_IN,                     /* bEndpointAddress */
    0x02,                                   /* bmAttributes: Bulk */
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize: */
    HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    
    0x00,                                   /* bInterval: ignore for Bulk transfer */
#endif

#if 1   // MIDI    

    /*---------------------------------------------------------------------------*/
    // IAD descriptor for MIDI interfaces

    0x08,                   /* bLength */
	0x0B,                   /* bDescriptorType: IAD */
	COMP_INTERFACE_IDX_MIDI,/* bFirstInterface */
	0x02,                   /* bInterfaceCount: 2 for MIDI */
	0x01,                   /* bFunctionClass: Audio */
	0x00,                   /* bFunctionSubClass: Undefined */
	0x00,                   /* bFunctionProtocol: Undefined */
	0x00,                   /* iFunction (Index of string descriptor describing this function) */

    /*---------------------------------------------------------------------------*/
    // MIDI Descriptor

    // The Audio Interface Collection
    // Standard AC Interface Descriptor (midi10.pdf, B.3.1)
    0x09,                       // bLength
    USB_DESC_TYPE_INTERFACE,    // bDescriptorType: Interface
    COMP_INTERFACE_IDX_MIDI,    // bInterfaceNumber
    0x00,                       // bAlternateSetting
    0x00,                       // bNumEndpoints: 0
    0x01,                       // bInterfaceClass: Audio
    0x01,                       // bInterfaceSubClass: AudioControl
    0x00,                       // bInterfaceProtocol: Undefined
    0x00,                       // iInterface: Index of string descriptor

    // Class-specific AC Interface Descriptor (midi10.pdf, B.3.2)
    0x09,   // bLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x01,   // bDescriptorSubtype: Header
    0x00,   // bcdADC: Revision of class spec: 1.0
    0x01,   
    0x09,   // wTotalLength
    0x00,   
    0x01,   // bInCollection: Number of streaming interfaces: 1
    0x01,   // baInterfaceNr(1): AudioStreaming interface 1 belongs tothis AudioControl interface

    // MIDIStreaming Interface Descriptors (midi10.pdf, B.4.1)
    0x09,                           // bLength
    USB_DESC_TYPE_INTERFACE,        // bDescriptorType: Interface
    COMP_INTERFACE_IDX_MIDI_STREAM, // bInterfaceNumber
    0x00,                           // bAlternateSetting
    0x02,                           // bNumEndpoints: 2
    0x01,                           // bInterfaceClass: Audio
    0x03,                           // bInterfaceSubClass: MIDIStreaming
    0x00,                           // bInterfaceProtocol
    0x00,                           // iInterface: Index of string descriptor

    // Class-Specific MS Interface Header Descriptor (midi10.pdf, 6.1.2.1)
    0x07,   // bLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x01,   // bDescriptorSubtype: Header
    0x00,   // bcdMSC: Revision of class spec: 1.0
    0x01,   
    0x41,   // wTotalLength
    0x00,                

    // MIDI IN Jack Descriptor (midi10.pdf, B.4.3)
    0x06,   // bLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x02,   // bDescriptorSubtype: MIDI_IN_JACK
    0x01,   // bJackType: Embedded
    0x01,   // bJackID: 1
    0x00,   // iJack: unused
    
    0x06, 0x24, 0x02, 0x02, 0x02, 0x00, // see above

    // MIDI OUT Jack Descriptors (midi10.pdf, B.4.4)
    0x09,   // bLength
    0x24,   // bDescriptorType: CS_INTERFACE
    0x03,   // bDescriptorSubtype: MIDI_OUT_JACK
    0x01,   // bJackType: Embedded
    0x03,   // bJackID: 3
    0x01,   // bNrInputPins: 1
    0x02,   // BaSourceID(1): ID of the Entity to which this Pin is connected: 2
    0x01,   // BaSourcePin(1): Output Pin number of the Entity to which this Input Pin is connected: 1
    0x00,   // iJack: unused

    0x09, 0x24, 0x03, 0x02, 0x04, 0x01, 0x01, 0x01, 0x00, // see above

    // Standard Bulk OUT Endpoint Descriptor
    0x09,                   // bLength
    USB_DESC_TYPE_ENDPOINT, // bDescriptorType
    COMP_EP_IDX_MIDI,       // bEndpointAddress
    0x02,                   // bmAttributes: Bulk
    0x40,                   // wMaxPacketSize: 64
    0x00, 
    0x00,                   // bInterval: ignore for Bulk transfer
    0x00,                   // bRefresh: unused
    0x00,                   // bSynchAddress: unused
    
    // Class-specific MS Bulk OUT Endpoint Descriptor (midi10.pdf, B.5.2)
    0x05,   // bLength
    0x25,   // bDescriptorType: CS_Endpoint
    0x01,   // bDescriptorSubtype: MS_General
    0x01,   // bNumEmbMIDIJack: Number of embedded MIDI IN Jacks: 1
    0x01,   // BaAssocJackID(1): ID of the Embedded MIDI IN Jack: 1

    // Standard Bulk IN Endpoint Descriptor
    0x09,                   // bLength
    USB_DESC_TYPE_ENDPOINT, // bDescriptorType
    COMP_EP_IDX_MIDI_IN,    // bEndpointAddress
    0x02,                   // bmAttributes: Bulk, not shared
    0x40,                   // wMaxPacketSize: 64
    0x00,
    0x00,                   // bInterval: ignore for Bulk transfer
    0x00,                   // bRefresh: unused
    0x00,                   // bSynchAddress: unused
    
    // Class-specific MS Bulk IN Endpoint Descriptor (midi10.pdf, B.6.2)
    0x05,   // bLength
    0x25,   // bDescriptorType: CS_Endpoint
    0x01,   // bDescriptorSubtype: MS_General
    0x01,   // bNumEmbMIDIJack: Number of embedded MIDI OUT Jacks
    0x03,   // BaAssocJackID(1): ID of the Embedded MIDI OUT Jack
#endif 


};

#if defined ( __ICCARM__ ) /*!< IAR Compiler */
#pragma data_alignment=4
#endif
/* USB Standard Device Descriptor */
static uint8_t USBD_Composite_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] =
{
    USB_LEN_DEV_QUALIFIER_DESC,
    USB_DESC_TYPE_DEVICE_QUALIFIER,
    0x00,
    0x02,
    0x00,
    0x00,
    0x00,
    0x40,
    0x01,
    0x00,
};

/**
    * @brief  USBD_Composite_Init
    *         Initialize the TEMPLATE interface
    * @param  pdev: device instance
    * @param  cfgidx: Configuration index
    * @retval status
    */
static uint8_t USBD_Composite_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    printf("[USB Init] ");
    uint8_t ret = 0;
    ret = USBD_CDC.Init(pdev, cfgidx);
    printf("CDC: %d ", ret);
    if (ret != USBD_OK)
        return ret;
    ret = USBD_MIDI.Init(pdev, cfgidx);
    printf("MIDI: %d ", ret);
    if (ret != USBD_OK)
        return ret;
    printf("\n");
    
    return USBD_OK;
}

/**
    * @brief  USBD_Composite_Init
    *         DeInitialize the TEMPLATE layer
    * @param  pdev: device instance
    * @param  cfgidx: Configuration index
    * @retval status
    */
static uint8_t USBD_Composite_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
    printf("[USB Deinit]\n");
    USBD_CDC.DeInit(pdev, cfgidx);
    USBD_MIDI.DeInit(pdev, cfgidx);
    return USBD_OK;
}

/**
    * @brief  USBD_Composite_Setup
    *         Handle the TEMPLATE specific requests
    * @param  pdev: instance
    * @param  req: usb requests
    * @retval status
    */
static uint8_t USBD_Composite_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
    USBD_StatusTypeDef ret = USBD_OK;

    // switch (req->bmRequest & USB_REQ_TYPE_MASK)
    // {
    //     case USB_REQ_TYPE_CLASS :
    //         switch (req->bRequest)
    //         {
    //             default:
    //                 USBD_CtlError(pdev, req);
    //                 ret = USBD_FAIL;
    //                 break;
    //         }
    //         break;

    //     case USB_REQ_TYPE_STANDARD:
    //         switch (req->bRequest)
    //         {
    //             default:
    //                 USBD_CtlError(pdev, req);
    //                 ret = USBD_FAIL;
    //                 break;
    //         }
    //         break;

    //     default:
    //         USBD_CtlError(pdev, req);
    //         ret = USBD_FAIL;
    //         break;
    // }

    uint8_t recipient = req->bmRequest & USB_REQ_RECIPIENT_MASK;

    // not sure which style I like better, 'switch' needs two calls, but 'if' looks more confusing
    // if (recipient == USB_REQ_RECIPIENT_INTERFACE && req->wIndex == COMP_INTERFACE_IDX_CDC ||
    //     recipient == USB_REQ_RECIPIENT_ENDPOINT && (epIdx == COMP_EP_IDX_CDC || epIdx == COMP_EP_IDX_CDC_CMD)) {
    //     return USBD_CDC_Setup(pdev, req);
    // }

    printf("[USB Setup] dev: %d, recipient: %d, index: %d\n", pdev->id, recipient, req->wIndex);

    if (recipient == USB_REQ_RECIPIENT_INTERFACE) {
        switch (req->wIndex) {
            case COMP_INTERFACE_IDX_CDC:
                return USBD_CDC.Setup(pdev, req);
            case COMP_INTERFACE_IDX_MIDI:
            case COMP_INTERFACE_IDX_MIDI_STREAM:
                return USBD_MIDI.Setup(pdev, req);
        }
    }
    else if (recipient == USB_REQ_RECIPIENT_ENDPOINT) {
        switch (req->wIndex & 0x7F) { // endpoint index without direction bit
            case COMP_EP_IDX_CDC:
            case COMP_EP_IDX_CDC_CMD:
                return USBD_CDC.Setup(pdev, req);
            case COMP_EP_IDX_MIDI:
            case COMP_EP_IDX_MIDI_IN:
                return USBD_MIDI.Setup(pdev, req);
        }
    }

    return ret;
}


/**
    * @brief  USBD_Composite_GetCfgDesc
    *         return configuration descriptor
    * @param  length : pointer data length
    * @retval pointer to descriptor buffer
    */
static uint8_t *USBD_Composite_GetCfgDesc(uint16_t *length) {
    printf("[USB GetCfgDesc]\n");
    *length = sizeof(USBD_Composite_CfgDesc);
    return USBD_Composite_CfgDesc;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t *USBD_Composite_DeviceQualifierDescriptor(uint16_t *length) {
    printf("[USB DevQualDesc]\n");
    *length = sizeof(USBD_Composite_DeviceQualifierDesc);
    return USBD_Composite_DeviceQualifierDesc;
}


/**
    * @brief  USBD_Composite_DataIn
    *         handle data IN Stage
    * @param  pdev: device instance
    * @param  epnum: endpoint index
    * @retval status
    */
static uint8_t USBD_Composite_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    printf("[USB DataIn] EP: %d\n", epnum);

    switch(epnum) {
        case COMP_EP_IDX_CDC:
        case COMP_EP_IDX_CDC_CMD:
            return USBD_CDC.DataIn(pdev, epnum);
        case COMP_EP_IDX_MIDI:
            return USBD_MIDI.DataIn(pdev, epnum);
        default:
            return USBD_OK;
    }
}

/**
    * @brief  USBD_Composite_DataOut
    *         handle data OUT Stage
    * @param  pdev: device instance
    * @param  epnum: endpoint index
    * @retval status
    */
static uint8_t USBD_Composite_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum) {
    printf("[USB DataOut] EP: %d\n", epnum);

    switch(epnum) {
        case COMP_EP_IDX_CDC:
        case COMP_EP_IDX_CDC_CMD:
            return USBD_CDC.DataOut(pdev, epnum);
        case COMP_EP_IDX_MIDI:
            return USBD_MIDI.DataOut(pdev, epnum);
        default:
            return USBD_OK;
    }
}

/**
    * @brief  USBD_Composite_EP0_RxReady
    *         handle EP0 Rx Ready event
    * @param  pdev: device instance
    * @retval status
    */
static uint8_t USBD_Composite_EP0_RxReady(USBD_HandleTypeDef *pdev) {
    printf("[USB EP0_RxReady]\n");
    // no nice way to differentiate between classes, hopefully only CDC needs this handler
    return USBD_CDC.EP0_RxReady(pdev);
}
/**
    * @brief  USBD_Composite_EP0_TxReady
    *         handle EP0 TRx Ready event
    * @param  pdev: device instance
    * @retval status
    */
static uint8_t USBD_Composite_EP0_TxReady(USBD_HandleTypeDef *pdev) {

    return USBD_OK;
}
/**
    * @brief  USBD_Composite_SOF
    *         handle SOF event
    * @param  pdev: device instance
    * @retval status
    */
static uint8_t USBD_Composite_SOF(USBD_HandleTypeDef *pdev) {

    return USBD_OK;
}
/**
    * @brief  USBD_Composite_IsoINIncomplete
    *         handle data ISO IN Incomplete event
    * @param  pdev: device instance
    * @param  epnum: endpoint index
    * @retval status
    */
static uint8_t USBD_Composite_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum) {

    return USBD_OK;
}
/**
    * @brief  USBD_Composite_IsoOutIncomplete
    *         handle data ISO OUT Incomplete event
    * @param  pdev: device instance
    * @param  epnum: endpoint index
    * @retval status
    */
static uint8_t USBD_Composite_IsoOutIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum) {

    return USBD_OK;
}

/**
* @brief  DeviceQualifierDescriptor
*         return Device Qualifier descriptor
* @param  length : pointer data length
* @retval pointer to descriptor buffer
*/
uint8_t *USBD_Composite_GetDeviceQualifierDesc(uint16_t *length) {
    *length = sizeof(USBD_Composite_DeviceQualifierDesc);
    return USBD_Composite_DeviceQualifierDesc;
}

