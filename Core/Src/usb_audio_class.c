#include "usb_audio_class.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include "usbd_conf.h"
#include "usbd_def.h"
#include "usbd_ctlreq.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
// #ifdef DEBUG
// #include <stdio.h>
// #endif

/* https://usb.org/document-library/audio-device-document-10 */


/* Feature Unit Control selectors */
#define MUTE_CONTROL      0x01
#define VOLUME_CONTROL    0x02

#define USBD_AUDIO_FREQ                               48000U
#define USBD_MAX_NUM_INTERFACES                       1U
#define AUDIO_BINTERVAL                               1U
#define AUDIO_OUT_EP                                  1U
#define USB_AUDIO_CONFIG_DESC_SIZE                    109U
#define AUDIO_INTERFACE_DESC_SIZE                     9U
#define USB_AUDIO_DESC_SIZE                           9U
#define AUDIO_STANDARD_ENDPOINT_DESC_SIZE             9U
#define AUDIO_STREAMING_ENDPOINT_DESC_SIZE            7U
#define AUDIO_DESCRIPTOR_TYPE                         0x21U
#define USB_DEVICE_CLASS_AUDIO                        0x01U
#define AUDIO_SUBCLASS_AUDIOCONTROL                   0x01U
#define AUDIO_SUBCLASS_AUDIOSTREAMING                 0x02U
#define AUDIO_PROTOCOL_UNDEFINED                      0x00U
#define AUDIO_STREAMING_GENERAL                       0x01U
#define AUDIO_STREAMING_FORMAT_TYPE                   0x02U

/* Audio Descriptor Types */
#define AUDIO_INTERFACE_DESCRIPTOR_TYPE               0x24U
#define AUDIO_ENDPOINT_DESCRIPTOR_TYPE                0x25U

/* Audio Control Interface Descriptor Subtypes */
#define AUDIO_CONTROL_HEADER                          0x01U
#define AUDIO_CONTROL_INPUT_TERMINAL                  0x02U
#define AUDIO_CONTROL_OUTPUT_TERMINAL                 0x03U
#define AUDIO_CONTROL_FEATURE_UNIT                    0x06U
#define AUDIO_INPUT_TERMINAL_DESC_SIZE                12U
#define AUDIO_OUTPUT_TERMINAL_DESC_SIZE               9U
#define AUDIO_STREAMING_INTERFACE_DESC_SIZE           7U
#define AUDIO_CONTROL_MUTE                            0x01U
#define AUDIO_CONTROL_VOLUME                          0x02U
#define AUDIO_FORMAT_TYPE_I                           0x01U
#define AUDIO_FORMAT_TYPE_III                         0x03U
#define AUDIO_ENDPOINT_GENERAL                        0x01U
#define AUDIO_OUT_STREAMING_CTRL                      0x02U
#define AUDIO_OUT_TC                                  0x01U
#define AUDIO_IN_TC                                   0x02U
#define AUDIO_DELAY                                   10U

/* Class specific request codes */
#define AUDIO_REQ_SET         0x00
#define AUDIO_REQ_GET         0x80
#define AUDIO_REQ_SET_CUR     0x01
#define AUDIO_REQ_GET_CUR     0x81
#define AUDIO_REQ_SET_MIN     0x02
#define AUDIO_REQ_GET_MIN     0x82
#define AUDIO_REQ_SET_MAX     0x03
#define AUDIO_REQ_GET_MAX     0x83
#define AUDIO_REQ_SET_RES     0x04
#define AUDIO_REQ_GET_RES     0x84

#define AUDIO_OUT_PACKET \
  (uint16_t)(((USBD_AUDIO_FREQ * 2U * 2U) / 1000U))

#define AUDIO_SAMPLE_FREQ(frq) \
  (uint8_t)(frq), (uint8_t)((frq >> 8)), (uint8_t)((frq >> 16))

#define AUDIO_PACKET_SZE(frq) \
  (uint8_t)(((frq * 2U * 2U) / 1000U) & 0xFFU), (uint8_t)((((frq * 2U * 2U) / 1000U) >> 8) & 0xFFU)


typedef struct {
  uint8_t reqest;
  uint8_t unit_id;
  uint8_t selector;
  uint16_t len;
  uint8_t data[USB_MAX_EP0_SIZE];
} USB_Audio_ControlTypeDef;

typedef struct {
  uint8_t mute;
  int16_t volume;
} USB_Audio_StatusTypeDef;

typedef struct {
  // uint32_t alt_setting;
  USB_Audio_ControlTypeDef control;
  USB_Audio_StatusTypeDef status;
  uint8_t RxBuff[AUDIO_OUT_PACKET];
} USB_Audio_HandleTypeDef;


static uint8_t USB_Audio_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USB_Audio_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USB_Audio_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t *USB_Audio_GetCfgDesc(uint16_t *length);
static uint8_t *USB_Audio_GetDeviceQualifierDesc(uint16_t *length);
static uint8_t USB_Audio_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USB_Audio_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USB_Audio_EP0_RxReady(USBD_HandleTypeDef *pdev);
static uint8_t USB_Audio_EP0_TxSent(USBD_HandleTypeDef *pdev);
static uint8_t USB_Audio_SOF(USBD_HandleTypeDef *pdev);
static uint8_t USB_Audio_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t USB_Audio_IsoOUTIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum);

static void AUDIO_REQ_Get(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void AUDIO_REQ_Set(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static void *USB_Audio_GetAudioHeaderDesc(uint8_t *pConfDesc);


USBD_ClassTypeDef USB_Audio_Class = {
  .Init = USB_Audio_Init,
  .DeInit = USB_Audio_DeInit,
  .Setup = USB_Audio_Setup,
  .EP0_TxSent = USB_Audio_EP0_TxSent,
  .EP0_RxReady = USB_Audio_EP0_RxReady,
  .DataIn = USB_Audio_DataIn,
  .DataOut = USB_Audio_DataOut,
  .SOF = USB_Audio_SOF,
  .IsoINIncomplete = USB_Audio_IsoINIncomplete,
  .IsoOUTIncomplete = USB_Audio_IsoOUTIncomplete,
  .GetFSConfigDescriptor = USB_Audio_GetCfgDesc,
  .GetDeviceQualifierDescriptor = USB_Audio_GetDeviceQualifierDesc,
};


/* USB AUDIO 1.0 device Configuration Descriptor */
static uint8_t USBD_AudioPlayer_CfgDesc[USB_AUDIO_CONFIG_DESC_SIZE]
__attribute__((aligned(4))) = {
  /* Configuration 1 */
  0x09,                               /* bLength */
  USB_DESC_TYPE_CONFIGURATION,        /* bDescriptorType */
  LOBYTE(USB_AUDIO_CONFIG_DESC_SIZE), /* wTotalLength */
  HIBYTE(USB_AUDIO_CONFIG_DESC_SIZE),
  0x02,                               /* bNumInterfaces */
  0x01,                               /* bConfigurationValue */
  0x00,                               /* iConfiguration */
  #if (USBD_SELF_POWERED == 1U)
  0xC0,                                   /* bmAttributes: Bus Powered according to user configuration */
  #else
  0x80,                               /* bmAttributes: Bus Powered according to user configuration */
  #endif
  USBD_MAX_POWER,                     /* MaxPower (mA) */
  /* 09 byte*/

  /* USB Speaker Standard interface descriptor */
  AUDIO_INTERFACE_DESC_SIZE,          /* bLength */
  USB_DESC_TYPE_INTERFACE,           /* bDescriptorType */
  0x00,                              /* bInterfaceNumber */
  0x00,                              /* bAlternateSetting */
  0x00,                              /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,            /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOCONTROL,       /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,          /* bInterfaceProtocol */
  0x00,                              /* iInterface */
  /* 09 byte*/

  /* USB Speaker Class-specific AC Interface Descriptor */
  AUDIO_INTERFACE_DESC_SIZE,          /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,    /* bDescriptorType */
  AUDIO_CONTROL_HEADER,               /* bDescriptorSubtype */
  0x00,          /* 1.00 */           /* bcdADC */
  0x01,
  0x27,                               /* wTotalLength */
  0x00,
  0x01,                               /* bInCollection */
  0x01,                               /* baInterfaceNr */
  /* 09 byte*/

  /* USB Speaker Input Terminal Descriptor */
  AUDIO_INPUT_TERMINAL_DESC_SIZE,     /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,    /* bDescriptorType */
  AUDIO_CONTROL_INPUT_TERMINAL,       /* bDescriptorSubtype */
  0x01,                               /* bTerminalID */
  0x01,                               /* wTerminalType AUDIO_TERMINAL_USB_STREAMING   0x0101 */
  0x01,
  0x00,                               /* bAssocTerminal */
  0x01,                               /* bNrChannels */
  0x00,                               /* wChannelConfig 0x0000  Mono */
  0x00,
  0x00,                               /* iChannelNames */
  0x00,                               /* iTerminal */
  /* 12 byte*/

  /* USB Speaker Audio Feature Unit Descriptor */
  0x09,                               /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,    /* bDescriptorType */
  AUDIO_CONTROL_FEATURE_UNIT,         /* bDescriptorSubtype */
  AUDIO_OUT_STREAMING_CTRL,           /* bUnitID */
  0x01,                               /* bSourceID */
  0x01,                               /* bControlSize */
  AUDIO_CONTROL_VOLUME |              /* bmaControls(0) */
      AUDIO_CONTROL_MUTE,
  0,                                  /* bmaControls(1) */
  0x00,                               /* iTerminal */
  /* 09 byte */

  /* USB Speaker Output Terminal Descriptor */
  0x09,                               /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,    /* bDescriptorType */
  AUDIO_CONTROL_OUTPUT_TERMINAL,      /* bDescriptorSubtype */
  0x03,                               /* bTerminalID */
  0x01,                               /* wTerminalType "Speaker" (0x0301) */
  0x03,
  0x00,                               /* bAssocTerminal */
  AUDIO_OUT_STREAMING_CTRL,           /* bSourceID */
  0x00,                               /* iTerminal */
  /* 09 byte */

  /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Zero Bandwidth */
  /* Interface 1, Alternate Setting 0                                              */
  AUDIO_INTERFACE_DESC_SIZE,          /* bLength */
  USB_DESC_TYPE_INTERFACE,            /* bDescriptorType */
  0x01,                               /* bInterfaceNumber */
  0x00,                               /* bAlternateSetting */
  0x00,                               /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,             /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,      /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,           /* bInterfaceProtocol */
  0x00,                               /* iInterface */
  /* 09 byte*/

  /* USB Speaker Standard AS Interface Descriptor - Audio Streaming Operational */
  /* Interface 1, Alternate Setting 1                                           */
  AUDIO_INTERFACE_DESC_SIZE,          /* bLength */
  USB_DESC_TYPE_INTERFACE,            /* bDescriptorType */
  0x01,                               /* bInterfaceNumber */
  0x01,                               /* bAlternateSetting */
  0x01,                               /* bNumEndpoints */
  USB_DEVICE_CLASS_AUDIO,             /* bInterfaceClass */
  AUDIO_SUBCLASS_AUDIOSTREAMING,      /* bInterfaceSubClass */
  AUDIO_PROTOCOL_UNDEFINED,           /* bInterfaceProtocol */
  0x00,                               /* iInterface */
  /* 09 byte*/

  /* USB Speaker Audio Streaming Interface Descriptor */
  AUDIO_STREAMING_INTERFACE_DESC_SIZE,/* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,    /* bDescriptorType */
  AUDIO_STREAMING_GENERAL,            /* bDescriptorSubtype */
  0x01,                               /* bTerminalLink */
  AUDIO_DELAY,                        /* bDelay */
  0x01,                               /* wFormatTag AUDIO_FORMAT_PCM  0x0001 */
  0x00,
  /* 07 byte*/

  /* USB Speaker Audio Type III Format Interface Descriptor */
  0x0B,                               /* bLength */
  AUDIO_INTERFACE_DESCRIPTOR_TYPE,    /* bDescriptorType */
  AUDIO_STREAMING_FORMAT_TYPE,        /* bDescriptorSubtype */
  AUDIO_FORMAT_TYPE_I,                /* bFormatType */
  0x02,                               /* bNrChannels */
  0x02,                               /* bSubFrameSize :  2 Bytes per frame (16bits) */
  16,                                 /* bBitResolution (16-bits per sample) */
  0x01,                               /* bSamFreqType only one frequency supported */
  AUDIO_SAMPLE_FREQ(USBD_AUDIO_FREQ),      /* Audio sampling frequency coded on 3 bytes */
  /* 11 byte*/

  /* Endpoint 1 - Standard Descriptor */
  AUDIO_STANDARD_ENDPOINT_DESC_SIZE,  /* bLength */
  USB_DESC_TYPE_ENDPOINT,             /* bDescriptorType */
  AUDIO_OUT_EP,                       /* bEndpointAddress 1 out endpoint */
  USBD_EP_TYPE_ISOC,                  /* bmAttributes */
  AUDIO_PACKET_SZE(USBD_AUDIO_FREQ),       /* wMaxPacketSize in Bytes (Freq(Samples)*2(Stereo)*2(HalfWord)) */
  AUDIO_BINTERVAL,                    /* bInterval */
  0x00,                               /* bRefresh */
  0x00,                               /* bSynchAddress */
  /* 09 byte*/

  /* Endpoint - Audio Streaming Descriptor */
  AUDIO_STREAMING_ENDPOINT_DESC_SIZE, /* bLength */
  AUDIO_ENDPOINT_DESCRIPTOR_TYPE,     /* bDescriptorType */
  AUDIO_ENDPOINT_GENERAL,             /* bDescriptor */
  0x00,                               /* bmAttributes */
  0x00,                               /* bLockDelayUnits */
  0x00,                               /* wLockDelay */
  0x00,
  /* 07 byte*/
} ;

/* USB Standard Device Descriptor */
static uint8_t USBD_AudioPlayer_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC]
__attribute__((aligned(4))) =
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


static uint8_t AUDIOOutEpAdd = AUDIO_OUT_EP;


void __stream_start(USBD_HandleTypeDef *pdev, uint32_t sample_rate) {
  USB_Audio_IntfTypeDef *intf = pdev->pUserData[pdev->classId];
  if (intf != NULL && intf->stream_start != NULL)
    intf->stream_start(sample_rate);
}

void __stream_end(USBD_HandleTypeDef *pdev) {
  USB_Audio_IntfTypeDef *intf = pdev->pUserData[pdev->classId];
  if (intf != NULL && intf->stream_end != NULL)
    intf->stream_end();
}

void __audio_data_received(USBD_HandleTypeDef *pdev, int16_t* buff, uint16_t size) {
  USB_Audio_IntfTypeDef *intf = pdev->pUserData[pdev->classId];
  if (intf != NULL && intf->audio_data_received != NULL)
    intf->audio_data_received(buff, size);
}

void __set_volume(USBD_HandleTypeDef *pdev) {
  USB_Audio_IntfTypeDef *intf = pdev->pUserData[pdev->classId];
  USB_Audio_HandleTypeDef *inst = pdev->pClassDataCmsit[pdev->classId];
  if (inst != NULL && intf != NULL && intf->set_volume != NULL)
    intf->set_volume(inst->status.volume);
}

void __set_mute(USBD_HandleTypeDef *pdev) {
  USB_Audio_IntfTypeDef *intf = pdev->pUserData[pdev->classId];
  USB_Audio_HandleTypeDef *inst = pdev->pClassDataCmsit[pdev->classId];
  if (inst != NULL && intf != NULL && intf->set_mute != NULL)
    intf->set_mute(inst->status.mute);
}

void USB_Audio_RegisterInterface(USBD_HandleTypeDef *pdev, USB_Audio_IntfTypeDef *intf) {
  pdev->pUserData[pdev->classId] = intf;
}


/**
-------------------
USB event handlers:
-------------------
*/
static uint8_t USB_Audio_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
  UNUSED(cfgidx);
  static USB_Audio_HandleTypeDef USB_Audio_Instance;
  USB_Audio_HandleTypeDef* instance = &USB_Audio_Instance;

  if (instance == NULL) {
    pdev->pClassDataCmsit[pdev->classId] = NULL;
    return USBD_EMEM;
  }

  pdev->pClassDataCmsit[pdev->classId] = (void *)instance;
  pdev->pClassData = pdev->pClassDataCmsit[pdev->classId];
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].bInterval = AUDIO_BINTERVAL;

  /* Open Out Endpoint */
  (void)USBD_LL_OpenEP(pdev, AUDIOOutEpAdd, USBD_EP_TYPE_ISOC, AUDIO_OUT_PACKET);
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].is_used = 1U;

  /* Prepare Out Endpoint to receive 1st packet */
  (void)USBD_LL_PrepareReceive(pdev, AUDIOOutEpAdd, instance->RxBuff,
                               AUDIO_OUT_PACKET);

  return (uint8_t)USBD_OK;
}


static uint8_t USB_Audio_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx) {
  UNUSED(cfgidx);

  /* Open EP OUT */
  USBD_LL_CloseEP(pdev, AUDIOOutEpAdd);
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].is_used = 0U;
  pdev->ep_out[AUDIOOutEpAdd & 0xFU].bInterval = 0U;

  __stream_end(pdev);

  return (uint8_t)USBD_OK;
}


/* handling all AUDIO_CLASS specific requests */
static uint8_t USB_Audio_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
  USB_Audio_HandleTypeDef *instance;
  uint16_t len;
  uint8_t *pbuf;
  uint16_t status_info = 0U;
  USBD_StatusTypeDef ret = USBD_OK;

  // #ifdef DEBUG
  // printf("%08lx Setup %02hx %02hx %04x %04x %04x\r\n", HAL_GetTick(), req->bmRequest, req->bRequest, req->wValue, req->wIndex, req->wLength);
  // #endif

  instance = (USB_Audio_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (instance == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  switch (req->bmRequest & USB_REQ_TYPE_MASK) {
    case USB_REQ_TYPE_CLASS:
      switch (req->bRequest & 0xF0) {
        case AUDIO_REQ_GET:
          AUDIO_REQ_Get(pdev, req);
          break;

        case AUDIO_REQ_SET:
          AUDIO_REQ_Set(pdev, req);
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      switch (req->bRequest) {
        static uint8_t IFID = 0;
        case USB_REQ_GET_STATUS:
          if (pdev->dev_state == USBD_STATE_CONFIGURED) {
            (void)USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
          } else {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_GET_DESCRIPTOR:
          if ((req->wValue >> 8) == AUDIO_DESCRIPTOR_TYPE) {
            pbuf = (uint8_t *)USB_Audio_GetAudioHeaderDesc(pdev->pConfDesc);
            if (pbuf != NULL) {
              len = MIN(USB_AUDIO_DESC_SIZE, req->wLength);
              (void)USBD_CtlSendData(pdev, pbuf, len);
            } else {
              USBD_CtlError(pdev, req);
              ret = USBD_FAIL;
            }
          }
          break;

        case USB_REQ_GET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED) {
            (void)USBD_CtlSendData(pdev, &IFID, 1U);
          } else {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_SET_INTERFACE:
          if (pdev->dev_state == USBD_STATE_CONFIGURED) {
            if ((uint8_t)(req->wValue) <= USBD_MAX_NUM_INTERFACES) {
              IFID = (uint8_t)(req->wValue);
              if (IFID) {
                __stream_start(pdev, USBD_AUDIO_FREQ);
              } else {
                __stream_end(pdev);
              }
            }
            else {
              /* Call the error management function (command will be NAKed */
              USBD_CtlError(pdev, req);
              ret = USBD_FAIL;
            }
          } else {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        case USB_REQ_CLEAR_FEATURE:
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;
    default:
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
      break;
  }

  return (uint8_t)ret;
}


static uint8_t *USB_Audio_GetCfgDesc(uint16_t *length)
{
  *length = (uint16_t)sizeof(USBD_AudioPlayer_CfgDesc);

  return USBD_AudioPlayer_CfgDesc;
}


/* handles Data In Stage */
static uint8_t USB_Audio_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum) {
  UNUSED(pdev);
  UNUSED(epnum);
  /* Only OUT data are processed */
  return (uint8_t)USBD_OK;
}


/* handles Data Out Stage */
static uint8_t USB_Audio_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum) {
  uint16_t PacketSize;
  USB_Audio_HandleTypeDef *instance;

  instance = (USB_Audio_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (instance == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  if (epnum == AUDIOOutEpAdd) {
    /* Get received data packet length */
    PacketSize = (uint16_t)USBD_LL_GetRxDataSize(pdev, epnum);

    /* Packet received Callback */
    __audio_data_received(pdev, (int16_t*)instance->RxBuff, PacketSize/2);

    /* Prepare Out endpoint to receive next audio packet */
    (void)USBD_LL_PrepareReceive(pdev, AUDIOOutEpAdd, instance->RxBuff, AUDIO_OUT_PACKET);
  }

  return (uint8_t)USBD_OK;
}


/* handles EP0 Rx Ready event */
static uint8_t USB_Audio_EP0_RxReady(USBD_HandleTypeDef *pdev) {
  USB_Audio_HandleTypeDef *inst;
  inst = (USB_Audio_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (inst == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  if (inst->control.unit_id != AUDIO_OUT_STREAMING_CTRL) {
    Error_Handler();
  }

  switch (inst->control.selector) {
  case AUDIO_CONTROL_MUTE:
    // #ifdef DEBUG
    // printf("mute %u", inst->control.len);
    // #endif
    switch (inst->control.reqest) {
    case AUDIO_REQ_SET_CUR:
      inst->status.mute = *inst->control.data;
      // #ifdef DEBUG
      // printf(" SET_CUR %hu\r\n", *inst->control.data);
      // #endif
      __set_mute(pdev);
      break;

    default:
      Error_Handler();
    }
    break;

  case AUDIO_CONTROL_VOLUME:
    // #ifdef DEBUG
    // printf("volume %u", inst->control.len);
    // #endif
    switch (inst->control.reqest) {
    case AUDIO_REQ_SET_CUR:
      inst->status.volume = *(int16_t*)inst->control.data;
      // #ifdef DEBUG
      // printf(" SET_CUR %d\r\n", *(int16_t*)inst->control.data);
      // #endif
      __set_volume(pdev);
      break;

    // #ifdef DEBUG
    // case AUDIO_REQ_SET_MIN:
    //   printf(" SET_MIN %d\r\n", *(int16_t*)inst->control.data);
    //   break;

    // case AUDIO_REQ_SET_MAX:
    //   printf(" SET_MAX %d\r\n", *(int16_t*)inst->control.data);
    //   break;

    // case AUDIO_REQ_SET_RES:
    //   printf(" SET_RES %d\r\n", *(int16_t*)inst->control.data);
    //   break;
    // #endif

    default:
      Error_Handler();
    }
    break;

  default:
    Error_Handler();
  }

  return (uint8_t)USBD_OK;
}


/* handles EP0 TRx Ready event */
static uint8_t USB_Audio_EP0_TxSent(USBD_HandleTypeDef *pdev) {
  UNUSED(pdev);
  /* Only OUT control data are processed */
  return (uint8_t)USBD_OK;
}


/* handles the SOF event */
static uint8_t USB_Audio_SOF(USBD_HandleTypeDef *pdev) {
  UNUSED(pdev);
  return (uint8_t)USBD_OK;
}


/* handles the data ISO IN Incomplete event */
static uint8_t USB_Audio_IsoINIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum) {
  UNUSED(pdev);
  UNUSED(epnum);
  return (uint8_t)USBD_OK;
}


/* handles the data ISO OUT Incomplete event */
static uint8_t USB_Audio_IsoOUTIncomplete(USBD_HandleTypeDef *pdev, uint8_t epnum) {
  USB_Audio_HandleTypeDef *instance;
  instance = (USB_Audio_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (instance == NULL) {
    return (uint8_t)USBD_FAIL;
  }

  if (epnum == AUDIOOutEpAdd) {
    /* Prepare Out endpoint to receive next audio packet */
    (void)USBD_LL_PrepareReceive(pdev, AUDIOOutEpAdd, instance->RxBuff, AUDIO_OUT_PACKET);
  }

  return (uint8_t)USBD_OK;
}


/* Handles the Get Audio control request. */
static void AUDIO_REQ_Get(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
  USB_Audio_HandleTypeDef *instance;
  instance = (USB_Audio_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (instance == NULL) {
    return;
  }

  memset(instance->control.data, 0, USB_MAX_EP0_SIZE);

  switch (HIBYTE(req->wValue)) {
  case MUTE_CONTROL:
  #ifdef DEBUG
  #endif
    printf("mute %u", req->wLength);
    switch (req->bRequest) {
    case AUDIO_REQ_GET_CUR:
  #ifdef DEBUG
  #endif
      printf(" GET_CUR %hu\r\n", instance->status.mute);
      *instance->control.data = instance->status.mute;
      break;

    default:
      Error_Handler();
    }
    break;

  case VOLUME_CONTROL:
    #ifdef DEBUG
    printf("volume %u", req->wLength);
    #endif
    switch (req->bRequest) {
    case AUDIO_REQ_GET_CUR:
      #ifdef DEBUG
      printf(" GET_CUR %d\r\n", instance->status.volume);
      #endif
      *(int16_t*)instance->control.data = instance->status.volume;
      break;

    case AUDIO_REQ_GET_MIN:
      #ifdef DEBUG
      printf(" GET_MIN -32767\r\n");
      #endif
      *(int16_t*)instance->control.data = -32767;
      break;

    case AUDIO_REQ_GET_MAX:
      #ifdef DEBUG
      printf(" GET_MAX 0\r\n");
      #endif
      *(int16_t*)instance->control.data = 0;
      break;

    case AUDIO_REQ_GET_RES:
      #ifdef DEBUG
      printf(" GET_RES 1\r\n");
      #endif
      *(int16_t*)instance->control.data = 1;
      break;

    default:
      Error_Handler();
    }
    break;
  
  default:
    Error_Handler();
  }

  /* Send the current mute state */
  (void)USBD_CtlSendData(pdev, instance->control.data, MIN(req->wLength, USB_MAX_EP0_SIZE));
}

/* Handles the Set Audio control request. */
static void AUDIO_REQ_Set(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req) {
  USB_Audio_HandleTypeDef *instance;
  instance = (USB_Audio_HandleTypeDef *)pdev->pClassDataCmsit[pdev->classId];

  if (instance == NULL) {
    return;
  }

  if (req->wLength != 0U) {
    instance->control.reqest = req->bRequest;     /* request type */
    instance->control.selector = HIBYTE(req->wValue); /* Control Selector */
    instance->control.unit_id = HIBYTE(req->wIndex);  /* Feature Unit ID */
    instance->control.len = MIN(req->wLength, USB_MAX_EP0_SIZE);  /* Set the request data length */

    /* Prepare the reception of the buffer over EP0 */
    (void)USBD_CtlPrepareRx(pdev, instance->control.data, instance->control.len);
  }
}


static uint8_t *USB_Audio_GetDeviceQualifierDesc(uint16_t *length) {
  *length = (uint16_t)sizeof(USBD_AudioPlayer_DeviceQualifierDesc);

  return USBD_AudioPlayer_DeviceQualifierDesc;
}


static void *USB_Audio_GetAudioHeaderDesc(uint8_t *pConfDesc) {
  USBD_ConfigDescTypeDef *desc = (USBD_ConfigDescTypeDef *)(void *)pConfDesc;
  USBD_DescHeaderTypeDef *pdesc = (USBD_DescHeaderTypeDef *)(void *)pConfDesc;
  uint8_t *pAudioDesc =  NULL;
  uint16_t ptr;

  if (desc->wTotalLength > desc->bLength) {
    ptr = desc->bLength;

    while (ptr < desc->wTotalLength) {
      pdesc = USBD_GetNextDesc((uint8_t *)pdesc, &ptr);
      if ((pdesc->bDescriptorType == AUDIO_INTERFACE_DESCRIPTOR_TYPE) &&
          (pdesc->bDescriptorSubType == AUDIO_CONTROL_HEADER)) {
        pAudioDesc = (uint8_t *)pdesc;
        break;
      }
    }
  }
  return pAudioDesc;
}
