/** A graphic library that targets small 1-bit per pixel type displays. */

#pragma once

#include "fonts.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum microGL_draw_mode {
  MICROGL_DM_PSET = 0x00, /**< Pixel on. */
  MICROGL_DM_PRES = 0x01, /**< Pixel off. */
  MICROGL_DM_PINV = 0x02  /**< Pixel inverted. */
} microGL_draw_mode_t;

typedef enum microGL_scan_mode {
  MICROGL_SM_RD,  /* Right -> Down */
  MICROGL_SM_DR,  /* Down -> Right */
  MICROGL_SM_LD,  /* Left -> Down */
  MICROGL_SM_DL,  /* Down -> Left */
  MICROGL_SM_RU,  /* Right -> Up */
  MICROGL_SM_UR,  /* Up -> Right */
  MICROGL_SM_LU,  /* Left -> Up */
  MICROGL_SM_UL   /* Up -> Left */
} microGL_scan_mode_t;

// typedef enum microGL_byte_align {
//   MICROGL_VERTICAL_BYTE,
//   MICROGL_HORIZONTAL_BYTE
// } microGL_byte_align_t;

typedef enum microGL_bit_order {
  MICROGL_LSB_FIRST,
  MICROGL_MSB_FIRST
} microGL_bit_order_t;

typedef struct {
  microGL_draw_mode_t draw_mode;
  microGL_scan_mode_t scan_mode;
  // microGL_byte_align_t byte_align;
  microGL_bit_order_t bit_order;
  uint16_t width;
  uint16_t height;
  uint8_t *buf;  /* pointer to a sufficiently sized buffer to hold the whole fame data. */
  uint8_t **buf_ptr;  /* optionally a pointer to a buffer can be set */
} microGL_Framebuffer_t;


void microGL_clear(microGL_Framebuffer_t *FB, const uint8_t *bg_img);
void microGL_set_draw_mode(microGL_Framebuffer_t *FB, microGL_draw_mode_t pixel_mode);
void microGL_draw_vertical_line(microGL_Framebuffer_t *FB, int16_t x, int16_t y1, int16_t y2);
void microGL_draw_horizontal_line(microGL_Framebuffer_t *FB, int16_t x1, int16_t x2, int16_t y);
void microGL_draw_line(microGL_Framebuffer_t *FB, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void microGL_draw_rectangle(microGL_Framebuffer_t *FB, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool filled);
void microGL_draw_circle(microGL_Framebuffer_t *FB, int16_t center_x, int16_t center_y, int16_t radius);
void microGL_draw_bitmap(microGL_Framebuffer_t *FB, const uint8_t *bmp, int16_t left, int16_t top, int16_t width, int16_t height);
int16_t microGL_print_text(microGL_Framebuffer_t *FB, const uint8_t *str, int16_t left, int16_t top, const font_t *font);
void microGL_set_pixel(microGL_Framebuffer_t *FB, int16_t x, int16_t y);
void microGL_reset_pixel(microGL_Framebuffer_t *FB, int16_t x, int16_t y);
