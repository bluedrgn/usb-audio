#include "sh1106.h"
#include "stm32_hal_legacy.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#define NO_PAGES (SH1106_SCR_H / SH1106_PAGE_HEIGHT)
#define PAGE_MAX (NO_PAGES - 1)

typedef enum __attribute__((packed)) sh1106_commands {
  SH1106_CMD_SET_COL_LOW = 0x00,  /**< Set the lower 4 bits of the column address,  modify lower 4 bits. */
  SH1106_CMD_SET_COL_HIGH = 0x10, /**< Set the higher 4 bits of the column address, modify lower 4 bits. */
  SH1106_CMD_SET_PUMP_VOLTAGE = 0x30, /**< Sets DC-Dc output value, modify lower 2bits. */
  SH1106_CMD_SET_START_LINE = 0x40, /**< Set the display start line to 0. */
  SH1106_CMD_SET_CONTRAST = 0x81,   /**< Contrast control. */
  SH1106_CMD_SEG_REMAP_NORM = 0xA0, /**< Sets column 0 to SEG0. (Coordinate x increasing.) */
  SH1106_CMD_SEG_REMAP_INV = 0xA1,  /**< Sets column 127 to SEG0. (Coordinate x decreasing.) */
  SH1106_CMD_DISPLAY_BY_RAM = 0xA4, /**< Configures the display to show the RAM contents. */
  SH1106_CMD_DISPLAY_FORCE_ON = 0xA5, /**< Configures the display to full on. */
  SH1106_CMD_INV_OFF = 0xA6,    /**< Configures the display not to invert the data RAM data. */
  SH1106_CMD_INV_ON = 0xA7,     /**< Sets the display to invert the RAM. */
  SH1106_CMD_SET_MUX = 0xA8,    /**< Sets the multiplexer ratio. */
  SH1106_CMD_DCDC_MODE = 0xAD,  /**< Sets the DC-DC pump of the display. */
  SH1106_CMD_DISP_OFF = 0xAE,   /**< Sets the display to off. */
  SH1106_CMD_DISP_ON = 0xAF,    /**< Sets the display to on. */
  SH1106_CMD_PAGE_ADDR = 0xB0,  /**< Sets the page address to 0. */
  SH1106_CMD_SCAN_NORM = 0xC0,  /**< Sets the internal scanning of COM0 COM[n-1]. (Increasing y coordinate.) */
  SH1106_CMD_SCAN_INV = 0xC8,   /**< Sets the internal scan from COM[n-1] to COM0. (Decreasing y-coordinate.) */
  SH1106_CMD_SET_OFFSET = 0xD3, /**< Set the display COM offset. */
  SH1106_CMD_CLOCKDIV = 0xD5,   /**< Sets the display clock pre-scaler. */
  SH1106_CMD_SET_CHARGE = 0xD9, /**< Sets the display's discharge/precharge time. display. */
  SH1106_CMD_COM_HW = 0xDA,     /**< Sets the mode of the internal COM pins. */
  SH1106_CMD_VCOM_DSEL = 0xDB,  /**< Sets the voltage on the common pads at the  de-selection. */
} sh1106_commands_t;

// static variables
static sh1106_display_t *__currently_flushing = NULL;

void Error_Handler(void);
static void __flush(void);
static void __wait_flush_complete(void);

void sh1106_init(sh1106_display_t *display) {
  const uint8_t init_sequence[] = {
    SH1106_CMD_DISP_OFF,
    SH1106_CMD_SET_PUMP_VOLTAGE + 2,
    SH1106_CMD_SET_START_LINE + 0,
    SH1106_CMD_SET_CONTRAST, 128,
    SH1106_CMD_SEG_REMAP_INV,
    SH1106_CMD_DISPLAY_BY_RAM,
    SH1106_CMD_INV_OFF,
    SH1106_CMD_SET_MUX, 63,
    SH1106_CMD_DCDC_MODE, 0x8b,
    SH1106_CMD_SCAN_INV,
    SH1106_CMD_SET_OFFSET, 0,
    SH1106_CMD_CLOCKDIV, 0x50,
    SH1106_CMD_SET_CHARGE, 0x22,
    SH1106_CMD_COM_HW, 0x12,
    SH1106_CMD_VCOM_DSEL, 0x35
  };

  HAL_I2C_Mem_Write(display->hi2c, display->i2c_addr, 0x00, 1,
                    (uint8_t *)init_sequence, sizeof(init_sequence), 4);

  memset(display->fb1, 0, SH1106_FB_SIZE);
  if (display->fb2 != NULL) {
    memset(display->fb2, 0, SH1106_FB_SIZE);
    display->fbptr = display->fb2;
  } else {
    display->fbptr = display->fb1;
  }

  display->flush_state = 0;
  // __flush();
  // __wait_flush_complete(display);
}

void sh1106_on(sh1106_display_t *display) {
  uint8_t cmd;
  cmd = SH1106_CMD_DISP_ON;
  HAL_I2C_Mem_Write(display->hi2c, display->i2c_addr, 0x00, 1, &cmd, 1, 1);
  HAL_Delay(50);
}

void sh1106_off(sh1106_display_t *display) {
  uint8_t cmd;
  cmd = SH1106_CMD_DISP_OFF;
  HAL_I2C_Mem_Write(display->hi2c, display->i2c_addr, 0x00, 1, &cmd, 1, 1);
}

__weak void sh1106_flush_complete_callback(sh1106_display_t *display) {}

bool sh1106_poll_flush_complete() {
  return __currently_flushing == NULL;
}

void __wait_flush_complete() {
  while (!sh1106_poll_flush_complete()) {
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON,
                           PWR_SLEEPENTRY_WFE_NO_EVT_CLEAR);
  }
}

void sh1106_flush(sh1106_display_t *display) {
  __wait_flush_complete();
  __currently_flushing = display;
  __flush();
}

void __flush() {
  static uint8_t *fb_tmp;

  if (__currently_flushing == NULL) {
    return;
  }

  if (__currently_flushing->flush_state == 0) {
    if (__currently_flushing->fb2 == NULL) {
      __currently_flushing->fbptr = __currently_flushing->fb1;
      fb_tmp = __currently_flushing->fb1;
    } else {
      if (__currently_flushing->fbptr == __currently_flushing->fb1) {
        __currently_flushing->fbptr = __currently_flushing->fb2;
        fb_tmp = __currently_flushing->fb1;
      } else {
        __currently_flushing->fbptr = __currently_flushing->fb1;
        fb_tmp = __currently_flushing->fb2;
      }
    }
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_RESET);
  }

  /* flush completed */
  else if (__currently_flushing->flush_state >=
           (SH1106_SCR_H / SH1106_PAGE_HEIGHT) * 2) {
    sh1106_display_t *just_flushed;
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
    just_flushed = __currently_flushing;
    __currently_flushing = NULL;
    just_flushed->flush_state = 0;
    sh1106_flush_complete_callback(just_flushed);
    return;
  }

  if (HAL_I2C_GetState(__currently_flushing->hi2c) != HAL_I2C_STATE_READY) {
    Error_Handler();
  }

  if ((__currently_flushing->flush_state & 1) == 0) {
    static uint8_t page_address[] = {SH1106_CMD_PAGE_ADDR,
                                     SH1106_CMD_SET_COL_LOW + 2,
                                     SH1106_CMD_SET_COL_HIGH};
    page_address[0] =
        SH1106_CMD_PAGE_ADDR + (__currently_flushing->flush_state / 2);
    HAL_I2C_Mem_Write_DMA(__currently_flushing->hi2c,
                          __currently_flushing->i2c_addr, 0x00, 1, page_address,
                          sizeof(page_address));
  } else {
    HAL_I2C_Mem_Write_DMA(
        __currently_flushing->hi2c, __currently_flushing->i2c_addr, 0x40, 1,
        fb_tmp +
            ((__currently_flushing->flush_state / 2) * SH1106_SCR_W),
        SH1106_SCR_W);
  }

  __currently_flushing->flush_state++;
}

void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c) {
  if (__currently_flushing != NULL && hi2c == __currently_flushing->hi2c) {
    __flush();
  }
}

void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c) {
    Error_Handler();
}
