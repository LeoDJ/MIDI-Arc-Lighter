/**
 * Composite USB Class 
 * Made from the STM32 usbd_template.c
 * And this tutorial: https://sudonull.com/post/68144-CDC-MSC-USB-Composite-Device-on-STM32-HAL
 */

#include "usbd_composite.h"
#include "usbd_ctlreq.h"
#include "usbd_cdc.h"


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
    0x02,               	        /*bNumInterfaces: 2 interfaces (CDC) */
    0x01,               	        /*bConfigurationValue: Configuration value*/
    0x02,               	        /*iConfiguration: Index of string descriptor describing the configuration*/
    0xC0,               	        /*bmAttributes: bus powered and Supports Remote Wakeup */
    0x32,               	        /*MaxPower 100 mA: this current is used for detecting Vbus*/

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
	/* 08 bytes */

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
    CDC_CMD_EP,                     /* bEndpointAddress */
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
    CDC_OUT_EP,                             /* bEndpointAddress */
    0x02,                                   /* bmAttributes: Bulk */
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize: */
    HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    
    0x00,                                   /* bInterval: ignore for Bulk transfer */

    /*Endpoint IN Descriptor*/
    0x07,                                   /* bLength: Endpoint Descriptor size */
    USB_DESC_TYPE_ENDPOINT,                 /* bDescriptorType: Endpoint */
    CDC_IN_EP,                              /* bEndpointAddress */
    0x02,                                   /* bmAttributes: Bulk */
    LOBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    /* wMaxPacketSize: */
    HIBYTE(CDC_DATA_FS_MAX_PACKET_SIZE),    
    0x00                                    /* bInterval: ignore for Bulk transfer */

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
    uint8_t ret = 0;
    ret = USBD_CDC.Init(pdev, cfgidx);
    if (ret != USBD_OK)
        return ret;
    printf("[USB Init] CDC: %d\n", ret);
    
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
        }
    }
    else if (recipient == USB_REQ_RECIPIENT_ENDPOINT) {
        switch (req->wIndex & 0x7F) { // endpoint index without direction bit
            case COMP_EP_IDX_CDC:
            case COMP_EP_IDX_CDC_CMD:
                return USBD_CDC.Setup(pdev, req);
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

