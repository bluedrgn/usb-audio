/** A graphic library that targets small 1-bit per pixel type displays. */

#pragma once

#include "fonts.h"
#include <stdint.h>
#include <stdbool.h>

typedef enum {
  MICROGL_DRAWMODE_SET = 0x00, /**< Pixel on. */
  MICROGL_DRAWMODE_RES = 0x01, /**< Pixel off. */
  MICROGL_DRAWMODE_INV = 0x02  /**< Pixel invert. */
} microGL_drawmode_t;

typedef enum {
  MICROGL_SCANMODE_RIGHTDOWN,  /**< Right -> Down */
  MICROGL_SCANMODE_DOWNRIGHT,  /**< Down -> Right */
  MICROGL_SCANMODE_LEFTDOWN,   /**< Left -> Down */
  MICROGL_SCANMODE_DOWNLEFT,   /**< Down -> Left */
  MICROGL_SCANMODE_RIGHTUP,    /**< Right -> Up */
  MICROGL_SCANMODE_UPRIGHT,    /**< Up -> Right */
  MICROGL_SCANMODE_LEFTUP,     /**< Left -> Up */
  MICROGL_SCANMODE_UPLEFT      /**< Up -> Left */
} microGL_scanmode_t;

// typedef enum {
//   MICROGL_VERTICAL_BYTE,
//   MICROGL_HORIZONTAL_BYTE
// } microGL_bytealign_t;

typedef enum {
  MICROGL_LSB_FIRST,
  MICROGL_MSB_FIRST
} microGL_bitorder_t;

typedef struct {
  microGL_drawmode_t draw_mode;
  microGL_scanmode_t scan_mode;
  // microGL_bytealign_t byte_align;
  microGL_bitorder_t bit_order;
  uint16_t width;
  uint16_t height;
  uint8_t *buf;  /* pointer to a sufficiently sized buffer to hold the whole fame data. */
  uint8_t **buf_ptr;  /* optionally a pointer to a buffer can be set */
} microGL_canvas;

void microGL_init_canvas(microGL_canvas *canvas, uint16_t width, uint16_t height, microGL_scanmode_t scan_mode, microGL_bitorder_t bit_order, uint8_t **buf_ptr);
void microGL_clear(microGL_canvas *canvas, const uint8_t *bg_img);
void microGL_set_draw_mode(microGL_canvas *canvas, microGL_drawmode_t pixel_mode);
void microGL_draw_vertical_line(microGL_canvas *canvas, int16_t x, int16_t y1, int16_t y2);
void microGL_draw_horizontal_line(microGL_canvas *canvas, int16_t x1, int16_t x2, int16_t y);
void microGL_draw_line(microGL_canvas *canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void microGL_draw_rectangle(microGL_canvas *canvas, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool filled);
void microGL_draw_circle(microGL_canvas *canvas, int16_t center_x, int16_t center_y, int16_t radius);
void microGL_draw_bitmap(microGL_canvas *canvas, const uint8_t *bmp, int16_t left, int16_t top, int16_t width, int16_t height);
int16_t microGL_print_text(microGL_canvas *canvas, const uint8_t *str, int16_t left, int16_t top, const font_t *font);
void microGL_set_pixel(microGL_canvas *canvas, int16_t x, int16_t y);
void microGL_reset_pixel(microGL_canvas *canvas, int16_t x, int16_t y);
