#pragma once

#include <memory.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_ll_i2c.h"


#define SH1106_SCR_W (128)
#define SH1106_SCR_H (64)
#define SH1106_I2C_ADDRESS_0 (0x3c << 1)
#define SH1106_I2C_ADDRESS_1 (0x3d << 1)

#define SH1106_PAGE_HEIGHT (8)
#define SH1106_FB_SIZE (SH1106_SCR_W * (SH1106_SCR_H / SH1106_PAGE_HEIGHT))

typedef struct {
  I2C_HandleTypeDef *hi2c;
  uint8_t i2c_addr;
  volatile uint8_t flush_state;
  uint8_t *fb1;
  uint8_t *fb2;
  uint8_t *fbptr;
} sh1106_display_t;

void sh1106_init(sh1106_display_t *display);
void sh1106_on(sh1106_display_t *display);
void sh1106_off(sh1106_display_t *display);
void sh1106_flush(sh1106_display_t *display);
bool sh1106_poll_flush_complete();
void sh1106_flush_complete_callback(sh1106_display_t *display);

