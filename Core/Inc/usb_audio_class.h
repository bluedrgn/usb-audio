#pragma once


#include  "usbd_ioreq.h"
#include <stdint.h>


typedef struct {
  void (*stream_start)(uint32_t sample_rate);
  void (*stream_end)(void);
  void (*audio_data_received)(int16_t* buff, uint16_t size);
  void (*set_volume)(uint8_t volume);
  void (*set_mute)(uint8_t mute);
} USB_Audio_IntfTypeDef;

extern USBD_ClassTypeDef USB_Audio_Class;

void USB_Audio_RegisterInterface(USBD_HandleTypeDef *pdev, USB_Audio_IntfTypeDef *intf);

