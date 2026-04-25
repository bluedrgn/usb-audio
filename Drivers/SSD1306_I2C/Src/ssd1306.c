#include "ssd1306.h"
#include "stm32_hal_legacy.h"
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_def.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#define NO_PAGES (SSD1306_SCR_H / SSD1306_PAGE_HEIGHT)
#define PAGE_MAX (NO_PAGES - 1)

typedef enum __attribute__((packed)) ssd1306_commands {
    SSD1306_CMD_SET_COL_LOW = 0x00,         /**< Sets the lower 4 bits of the column address,  modify lower 4 bits. */
    SSD1306_CMD_SET_COL_HIGH = 0x10,        /**< Sets the higher 4 bits of the column address, modify lower 4 bits. */
    SSD1306_CMD_ADDR_MODE = 0x20,           /**< Next byte sets addressing mode: 0=horizontal; 1=vertical; 2=page. */
    SSD1306_CMD_PARTIAL_COL_RANGE = 0x21,   /**< Next two bytes set column start and end address. (0-127) */
    SSD1306_CMD_PARTIAL_PAGE_RANGE = 0x22,  /**< Next two bytes set page start and end address. (0-7) */
    SSD1306_CMD_SET_START_LINE = 0x40,      /**< Sets the display start line, modify lower 6 bits. */
    SSD1306_CMD_SET_CONTRAST = 0x81,        /**< Next byte sets display contrast. (0-255) */
    SSD1306_CMD_CHARGE_PUMP_ENABLE = 0x8D,  /**< Next byte sets charge pump: 0x14=enable; 0xAF=disable. */
    SSD1306_CMD_SEG_REMAP_NORM = 0xA0,      /**< Sets column 0 to SEG0. (Coordinate x increasing.) */
    SSD1306_CMD_SEG_REMAP_INV = 0xA1,       /**< Sets column 127 to SEG0. (Coordinate x decreasing.) */
    SSD1306_CMD_DISPLAY_BY_RAM = 0xA4,      /**< Configures the display to show the RAM contents. */
    SSD1306_CMD_DISPLAY_FORCE_ON = 0xA5,    /**< Configures the display to full on. */
    SSD1306_CMD_INV_OFF = 0xA6,             /**< Configures the display not to invert the data RAM data. */
    SSD1306_CMD_INV_ON = 0xA7,              /**< Sets the display to invert the RAM. */
    SSD1306_CMD_SET_MUX = 0xA8,             /**< Next byte sets multiplexer ratio. (15-63) */
    SSD1306_CMD_DISP_OFF = 0xAE,            /**< Sets the display to off. */
    SSD1306_CMD_DISP_ON = 0xAF,             /**< Sets the display to on. */
    SSD1306_CMD_SET_PAGE = 0xB0,            /**< Sets page start address, modify lower 3 bits. */
    SSD1306_CMD_SCAN_NORM = 0xC0,           /**< Sets the internal scanning of COM0 COM[n-1]. (Increasing y coordinate.) */
    SSD1306_CMD_SCAN_INV = 0xC8,            /**< Sets the internal scan from COM[n-1] to COM0. (Decreasing y-coordinate.) */
    SSD1306_CMD_SET_OFFSET = 0xD3,          /**< Next byte sets display vertical offset. (0-63) */
    SSD1306_CMD_CLOCKDIV = 0xD5,            /**< Next byte sets the display clock pre-scaler. (See datasheet.) */
    SSD1306_CMD_SET_CHARGE = 0xD9,          /**< Next byte sets the display's discharge/precharge time. (See datasheet.)) */
    SSD1306_CMD_COM_HW = 0xDA,              /**< Next byte sets the mode of the internal COM pins. (See datasheet.) */
    SSD1306_CMD_VCOM_DSEL = 0xDB,           /**< Next byte sets the voltage on the common pads at the de-selection. (See datasheet.) */
    SSD1306_CMD_NOP = 0xE3,                 /**< No operation */
} ssd1306_commands_t;

// static variables
static ssd1306_display_t *__currently_flushing = NULL;

void Error_Handler(void);
static void __flush(void);
static void __wait_flush_complete(void);

void ssd1306_init(ssd1306_display_t *display) {
  const uint8_t init_sequence[] = {
    SSD1306_CMD_DISP_OFF,
    SSD1306_CMD_SET_MUX, 63,
    SSD1306_CMD_SET_OFFSET, 0,
    SSD1306_CMD_SET_START_LINE + 0,
    SSD1306_CMD_SEG_REMAP_NORM,
    SSD1306_CMD_SCAN_NORM,
    SSD1306_CMD_COM_HW, 0x12,
    SSD1306_CMD_SET_CONTRAST, 255,
    SSD1306_CMD_DISPLAY_BY_RAM,
    SSD1306_CMD_INV_OFF,
    SSD1306_CMD_CLOCKDIV, 0x80,
    SSD1306_CMD_CHARGE_PUMP_ENABLE, 0x14
  };


  HAL_I2C_Mem_Write(display->hi2c, display->i2c_addr, 0x00, 1,
                    (uint8_t *)init_sequence, sizeof(init_sequence), 4);

  memset(display->fb1, 0, SSD1306_FB_SIZE);
  if (display->fb2 != NULL) {
    memset(display->fb2, 0, SSD1306_FB_SIZE);
    display->fbptr = display->fb2;
  } else {
    display->fbptr = display->fb1;
  }

  display->flush_state = 0;
  ssd1306_flush(display);
  // __wait_flush_complete();
  // ssd1306_on(display);
}

void ssd1306_on(ssd1306_display_t *display) {
  uint8_t cmd;
  cmd = SSD1306_CMD_DISP_ON;
  __wait_flush_complete();
  HAL_I2C_Mem_Write(display->hi2c, display->i2c_addr, 0x00, 1, &cmd, 1, 1);
}

void ssd1306_off(ssd1306_display_t *display) {
  uint8_t cmd;
  cmd = SSD1306_CMD_DISP_OFF;
  __wait_flush_complete();
  HAL_I2C_Mem_Write(display->hi2c, display->i2c_addr, 0x00, 1, &cmd, 1, 1);
}

__weak void ssd1306_flush_complete_callback(ssd1306_display_t *display) {}

bool ssd1306_poll_flush_complete() {
  return __currently_flushing == NULL;
}

void __wait_flush_complete() {
  while (!ssd1306_poll_flush_complete()) {
    HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON,
                           PWR_SLEEPENTRY_WFE_NO_EVT_CLEAR);
  }
}

void ssd1306_flush(ssd1306_display_t *display) {
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
           (SSD1306_SCR_H / SSD1306_PAGE_HEIGHT) * 2) {
    ssd1306_display_t *just_flushed;
    // HAL_GPIO_WritePin(GPIOC, GPIO_PIN_14, GPIO_PIN_SET);
    just_flushed = __currently_flushing;
    __currently_flushing = NULL;
    just_flushed->flush_state = 0;
    ssd1306_flush_complete_callback(just_flushed);
    return;
  }

  if (HAL_I2C_GetState(__currently_flushing->hi2c) != HAL_I2C_STATE_READY) {
    Error_Handler();
  }

  if ((__currently_flushing->flush_state & 1) == 0) {
    static uint8_t page_address[] = {SSD1306_CMD_SET_PAGE,
                                     SSD1306_CMD_SET_COL_LOW,
                                     SSD1306_CMD_SET_COL_HIGH};
    page_address[0] =
        SSD1306_CMD_SET_PAGE + (__currently_flushing->flush_state / 2);
    HAL_I2C_Mem_Write_DMA(__currently_flushing->hi2c,
                          __currently_flushing->i2c_addr, 0x00, 1, page_address,
                          sizeof(page_address));
  } else {
    HAL_I2C_Mem_Write_DMA(
        __currently_flushing->hi2c, __currently_flushing->i2c_addr, 0x40, 1,
        fb_tmp + ((__currently_flushing->flush_state / 2) * SSD1306_SCR_W),
        SSD1306_SCR_W);
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
