#pragma once

#include <memory.h>
#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx_ll_i2c.h"


#define SSD1306_SCR_W (128)
#define SSD1306_SCR_H (64)
#define SSD1306_I2C_ADDRESS_0 (0x3c << 1)
#define SSD1306_I2C_ADDRESS_1 (0x3d << 1)

#define SSD1306_PAGE_HEIGHT (8)
#define SSD1306_FB_SIZE (SSD1306_SCR_W * (SSD1306_SCR_H / SSD1306_PAGE_HEIGHT))

typedef struct {
  I2C_HandleTypeDef *hi2c;
  uint8_t i2c_addr;
  volatile uint8_t flush_state;
  uint8_t *fb1;
  uint8_t *fb2;
  uint8_t *fbptr;
} ssd1306_display_t;

void ssd1306_init(ssd1306_display_t *display);
void ssd1306_on(ssd1306_display_t *display);
void ssd1306_off(ssd1306_display_t *display);
void ssd1306_flush(ssd1306_display_t *display);
bool ssd1306_poll_flush_complete();
void ssd1306_flush_complete_callback(ssd1306_display_t *display);

