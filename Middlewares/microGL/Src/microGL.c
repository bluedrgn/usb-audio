
#include "microGL.h"
#include <memory.h>
#include <stdint.h>

#define PAGE_HEIGHT (8)
#define NO_PAGES (FB->height / PAGE_HEIGHT)
#define PAGE_MAX (NO_PAGES - 1)
#define FB_SIZE ((FB->width * FB->height) / PAGE_HEIGHT)

// #define SET_MASK(target_field, mask) (target_field) |= (mask)
// #define CLEAR_MASK(target_field, mask) (target_field) &= ~(mask)
// #define INVERT_MASK(target_field, mask) (target_field) ^= (mask)


typedef struct {
  int16_t right;
  int16_t down;
} _byte_step_t;

typedef struct {
  uint8_t page;
  uint8_t bit;
  uint16_t byte;
} _page_byte_bit_t;


static inline void __updateFB(microGL_Framebuffer_t *FB);
static inline uint8_t _pages_total(microGL_Framebuffer_t *FB);
static inline uint8_t _page(microGL_Framebuffer_t *FB, int16_t y);
static inline uint16_t _buffer_size(microGL_Framebuffer_t *FB);
static inline _byte_step_t _byte_step(microGL_Framebuffer_t *FB);
static inline _page_byte_bit_t _page_byte_bit(microGL_Framebuffer_t *FB, int16_t x, int16_t y);
static inline void _draw_vertical_line(microGL_Framebuffer_t *FB, int16_t x, int16_t top, int16_t bottom);
static inline void _draw_horizontal_line(microGL_Framebuffer_t *FB, int16_t y, int16_t left, int16_t right);
static inline void _set_pixel(microGL_Framebuffer_t *FB, int16_t x, int16_t y);
static inline void _reset_pixel(microGL_Framebuffer_t *FB, int16_t x, int16_t y);
static uint8_t _print_char(microGL_Framebuffer_t *FB, int16_t left, int16_t top, uint8_t character, const font_t *fnt);
static void _draw_filled_rectangle(microGL_Framebuffer_t *FB, int16_t left, int16_t top, int16_t right, int16_t bottom);
static void _draw_hollow_rectangle(microGL_Framebuffer_t *FB, int16_t left, int16_t top, int16_t right, int16_t bottom);
static void _draw_byte_vertical(microGL_Framebuffer_t *FB, int16_t left, int16_t top, uint8_t byte);
static void _draw_byte_horizontal(microGL_Framebuffer_t *FB, int16_t left, int16_t top, uint8_t byte);
static inline void _mask_set(microGL_Framebuffer_t *FB, uint8_t *ptr, uint8_t mask);
static inline void _mask_clear(microGL_Framebuffer_t *FB, uint8_t *ptr, uint8_t mask);


inline void __updateFB(microGL_Framebuffer_t *FB) {
  if (FB->buf_ptr != NULL) {
    FB->buf = *(FB->buf_ptr);
  }
}

inline uint8_t _pages_total(microGL_Framebuffer_t *FB) {
  return FB->height / PAGE_HEIGHT;
}

inline uint8_t _page(microGL_Framebuffer_t *FB, int16_t y) {
  return (_pages_total(FB) - 1) - (y / PAGE_HEIGHT);
}

inline uint16_t _buffer_size(microGL_Framebuffer_t *FB) {
  return (FB->width * _pages_total(FB));
}

inline _byte_step_t _byte_step(microGL_Framebuffer_t *FB) {
  switch (FB->scan_mode) {
  case MICROGL_SM_RD:
    return (_byte_step_t){.down=FB->width, .right=1};
  case MICROGL_SM_DR:
    return (_byte_step_t){.down=1, .right=_pages_total(FB)};
  case MICROGL_SM_LD:
    return (_byte_step_t){.down=FB->width, .right=-1};
  case MICROGL_SM_DL:
    return (_byte_step_t){.down=1, .right=-_pages_total(FB)};
  case MICROGL_SM_RU:
    return (_byte_step_t){.down=-FB->width, .right=1};
  case MICROGL_SM_UR:
    return (_byte_step_t){.down=-1, .right=_pages_total(FB)};
  case MICROGL_SM_LU:
    return (_byte_step_t){.down=-FB->width, .right=-1};
  case MICROGL_SM_UL:
    return (_byte_step_t){.down=-1, .right=-_pages_total(FB)};
  default:
    return (_byte_step_t){.down=0, .right=0};
  }
}

inline _page_byte_bit_t _page_byte_bit(microGL_Framebuffer_t *FB, int16_t x, int16_t y) {
  _page_byte_bit_t ret;
  _byte_step_t stp;

  stp = _byte_step(FB);
  ret.page = _page(FB, y);
  ret.bit = y % PAGE_HEIGHT;

  if (stp.right >= 0) {
    ret.byte = (stp.right * x);
  }
  else {
    ret.byte = _buffer_size(FB) + (stp.right * (x + 1));
  }

  if (stp.down >= 0) {
    ret.byte += (stp.down * ret.page);
  }
  else {
    /* TODO to check */
    ret.byte += (_pages_total(FB) - 1) + (stp.down * ret.page);
  }
  
  return ret;
}

void microGL_clear(microGL_Framebuffer_t *FB, const uint8_t *bg_img) {
  __updateFB(FB);
  
  if (bg_img == NULL)
    memset(FB->buf, 0, _buffer_size(FB));
  else
    memcpy(FB->buf, bg_img, _buffer_size(FB));
}

void microGL_set_draw_mode(microGL_Framebuffer_t *FB, microGL_draw_mode_t draw_mode) {
  switch (draw_mode) {
  case MICROGL_DM_PSET:
  case MICROGL_DM_PRES:
  case MICROGL_DM_PINV:
    FB->draw_mode = draw_mode;
    break;
  default:
    break;
  }
}

void microGL_draw_horizontal_line(microGL_Framebuffer_t *FB, int16_t x1, int16_t x2, int16_t y) {
  int16_t left, right;

  __updateFB(FB);
  
  if (x2 >= x1) {
    left = x1;
    right = x2;
  } else {
    left = x2;
    right = x1;
  }

  _draw_horizontal_line(FB, y, left, right);
}

void microGL_draw_vertical_line(microGL_Framebuffer_t *FB, int16_t x, int16_t y1, int16_t y2) {
  int16_t top, bottom;

  __updateFB(FB);
  
  if (y2 >= y1) {
    bottom = y1;
    top = y2;
  } else {
    bottom = y2;
    top = y1;
  }

  _draw_vertical_line(FB, x, top, bottom);
}

void microGL_draw_line(microGL_Framebuffer_t *FB, int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
  int16_t dx = x2 - x1;
  int16_t dy = y2 - y1;
  int16_t dx2;
  int16_t dy2;
  int16_t di;
  int8_t dx_sym = (dx > 0) ? 1 : -1;
  int8_t dy_sym = (dy > 0) ? 1 : -1;

  __updateFB(FB);
  
  if (dx == 0) {
    microGL_draw_vertical_line(FB, x1, y1, y2);
    return;
  }
  if (dy == 0) {
    microGL_draw_horizontal_line(FB, x1, x2, y1);
    return;
  }

  dx = dx * dx_sym;
  dy = dy * dy_sym;
  dx2 = dx << 1;
  dy2 = dy << 1;

  if (dx >= dy) {
    di = dy2 - dx;
    while (x1 != x2) {
      microGL_set_pixel(FB, x1, y1);
      x1 += dx_sym;
      if (di < 0) {
        di = di + dy2;
      } else {
        di = (di + (dy2 - dx2));
        y1 += dy_sym;
      }
    }
  } else {
    di = (int16_t)(dx2 - dy);
    while (y1 != y2) {
      microGL_set_pixel(FB, x1, y1);
      y1 += dy_sym;
      if (di < 0) {
        di = (int16_t)(di + dx2);
      } else {
        di = (int16_t)(di + (dx2 - dy2));
        x1 += dx_sym;
      }
    }
  }

  microGL_set_pixel(FB, x1, y1);
}

void microGL_draw_rectangle(microGL_Framebuffer_t *FB, int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool filled) {
  int16_t left, top, right, bottom;

  __updateFB(FB);
  
  if (x2 >= x1) {
    left = x1;
    right = x2;
  } else {
    left = x2;
    right = x1;
  }

  if (y2 >= y1) {
    bottom = y1;
    top = y2;
  } else {
    bottom = y2;
    top = y1;
  }

  if (filled) {
    /* boundary crop */
    if (left < 0)
      left = 0;
    if (bottom < 0)
      bottom = 0;
    if (top >= FB->height)
      top = FB->height - 1;
    if (right >= FB->width)
      right = FB->width - 1;
    _draw_filled_rectangle(FB, left, top, right, bottom);
  } else {
    _draw_hollow_rectangle(FB, left, top, right, bottom);
  }
}

void _draw_hollow_rectangle(microGL_Framebuffer_t *FB, int16_t left, int16_t top, int16_t right, int16_t bottom) {
  _draw_horizontal_line(FB, top, left, right - 1);
  _draw_vertical_line(FB, left, top - 1, bottom);
  _draw_vertical_line(FB, right, top, bottom + 1);
  _draw_horizontal_line(FB, bottom, left + 1, right);
}

void _draw_filled_rectangle(microGL_Framebuffer_t *FB, int16_t left, int16_t top, int16_t right, int16_t bottom) {
  static const uint8_t bottom_byte_lsb_lut[PAGE_HEIGHT] =
    {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
  static const uint8_t top_byte_lsb_lut[PAGE_HEIGHT] =
    {0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF};
  static const uint8_t bottom_byte_msb_lut[PAGE_HEIGHT] =
    {0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};
  static const uint8_t top_byte_msb_lut[PAGE_HEIGHT] =
    {0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};
  
  uint8_t top_mask, bottom_mask, *ptr, i;
  _page_byte_bit_t topleft, bottomleft;
  _byte_step_t step;

  step = _byte_step(FB);
  topleft = _page_byte_bit(FB, top, left);
  bottomleft = _page_byte_bit(FB, bottom, left);

  if (FB->bit_order == MICROGL_LSB_FIRST) {
    top_mask = top_byte_lsb_lut[topleft.bit];
    bottom_mask = bottom_byte_lsb_lut[bottomleft.bit];
  }
  else {
    top_mask = top_byte_msb_lut[topleft.bit];
    bottom_mask = bottom_byte_msb_lut[bottomleft.bit];
  }

  if (bottomleft.page == topleft.page) {
    top_mask &= bottom_mask;
  }

  ptr = &(FB->buf)[topleft.byte];
  switch (FB->draw_mode) {
  case MICROGL_DM_PSET:
    for (uint8_t x = left; x <= right; x++) {
      (*ptr) |= (top_mask);
      ptr += step.right;
    }
    break;
  case MICROGL_DM_PRES:
    for (uint8_t x = left; x <= right; x++) {
      (*ptr) &= ~(top_mask);
      ptr += step.right;
    }
    break;
  case MICROGL_DM_PINV:
    for (uint8_t x = left; x <= right; x++) {
      (*ptr) ^= (top_mask);
      ptr += step.right;
    }
    break;
  default:
    break;
  }

  if (bottomleft.page == topleft.page) {
    return;
  }

  i = 1;
  for (uint8_t page = topleft.page + 1; page < bottomleft.page; page++, i++) {
    ptr = &(FB->buf)[topleft.byte + (i * step.down)];
    switch (FB->draw_mode) {
    case MICROGL_DM_PSET:
      for (uint8_t x = left; x <= right; x++) {
        *ptr = 0xFF;
        ptr += step.right;
      }
      break;
    case MICROGL_DM_PRES:
      for (uint8_t x = left; x <= right; x++) {
        *ptr = 0x00;
        ptr += step.right;
      }
      break;
    case MICROGL_DM_PINV:
      for (uint8_t x = left; x <= right; x++) {
        *ptr = ~(*ptr);
        ptr += step.right;
      }
      break;
    default:
      break;
    }
  }

  ptr = &(FB->buf)[bottomleft.byte];
  switch (FB->draw_mode) {
  case MICROGL_DM_PSET:
    for (uint8_t x = left; x <= right; x++) {
      *ptr |= bottom_mask;
      ptr += step.right;
    }
    break;
  case MICROGL_DM_PRES:
    for (uint8_t x = left; x <= right; x++) {
      *ptr &= ~bottom_mask;
      ptr += step.right;
    }
    break;
  case MICROGL_DM_PINV:
    for (uint8_t x = left; x <= right; x++) {
      *ptr ^= bottom_mask;
      ptr += step.right;
    }
    break;
  default:
    break;
  }
}

void microGL_draw_circle(microGL_Framebuffer_t *FB, int16_t center_x, int16_t center_y, int16_t radius) {
  int16_t err = (int16_t)(1 - radius);
  int16_t dx = 0;
  int16_t dy = (int16_t)(-2 * radius);
  int16_t x = 0;
  int16_t y = radius;

  __updateFB(FB);
  
  while (x < y) {
    if (err >= 0) {
      dy += 2;
      err = (int16_t)(err + dy);
      y--;
    }
    dx += 2;
    err = (int16_t)(err + dx + 1);
    x++;

    microGL_set_pixel(FB, (center_x + x), (center_y + y));
    microGL_set_pixel(FB, (center_x + x), (center_y - y));
    microGL_set_pixel(FB, (center_x - x), (center_y + y));
    microGL_set_pixel(FB, (center_x - x), (center_y - y));
    microGL_set_pixel(FB, (center_x + y), (center_y + x));
    microGL_set_pixel(FB, (center_x + y), (center_y - x));
    microGL_set_pixel(FB, (center_x - y), (center_y + x));
    microGL_set_pixel(FB, (center_x - y), (center_y - x));
  }

  microGL_set_pixel(FB, center_x + radius, center_y);
  microGL_set_pixel(FB, center_x - radius, center_y);
  microGL_set_pixel(FB, center_x, center_y + radius);
  microGL_set_pixel(FB, center_x, center_y - radius);
}

void microGL_draw_bitmap(microGL_Framebuffer_t *FB, const uint8_t *bmp, int16_t top, int16_t left, int16_t width, int16_t height) {
  __updateFB(FB);
  
  for (int16_t y_pos = top; y_pos < top + height; y_pos += PAGE_HEIGHT) {
    for (int16_t x_pos = left; x_pos < left + width; x_pos++) {
      if (*bmp != 0) {
        _draw_byte_vertical(FB, y_pos, x_pos, *bmp);
      }
      bmp++;
    }
  }
}

int16_t microGL_print_text(microGL_Framebuffer_t *FB, const uint8_t *str, int16_t left, int16_t top, const font_t *font) {
  if (str == NULL) {
    return 0;
  }

  __updateFB(FB);
  
  int16_t width_limit = (FB->width - font->width - 1);

  while (*str != '\0' && left < width_limit) {
    left += _print_char(FB, left, top, *str, font);
    str++;
  }

  return left >= FB->width ? FB->width - 1 : left;
}

void microGL_set_pixel(microGL_Framebuffer_t *FB, int16_t x, int16_t y) {
  /* sanity check */
  if ((x < 0) || (y < 0) || (x >= FB->width) || (y >= FB->height)) {
    return;
  }
  __updateFB(FB);
  _set_pixel(FB, x, y);
}

inline void _set_pixel(microGL_Framebuffer_t *FB, int16_t x, int16_t y) {
  _page_byte_bit_t pix;
  uint8_t mask, *ptr;

  pix =  _page_byte_bit(FB, x, y);
  ptr = &(FB->buf)[pix.byte];
  if (FB->bit_order == MICROGL_LSB_FIRST) {
    mask = 0x80 >> pix.bit;
  }
  else {
    mask = 0x01 << pix.bit;
  }

  _mask_set(FB, ptr, mask);
}

void microGL_reset_pixel(microGL_Framebuffer_t *FB, int16_t x, int16_t y) {
  /* sanity check */
  if ((x < 0) || (y < 0) || (x >= FB->width) || (y >= FB->height)) {
    return;
  }
  __updateFB(FB);
  _reset_pixel(FB, x, y);
}

inline void _reset_pixel(microGL_Framebuffer_t *FB, int16_t x, int16_t y) {
  _page_byte_bit_t pix;
  uint8_t mask, *ptr;

  pix =  _page_byte_bit(FB, x, y);
  ptr = &(FB->buf)[pix.byte];
  if (FB->bit_order == MICROGL_LSB_FIRST) {
    mask = 0x80 >> pix.bit;
  }
  else {
    mask = 0x01 << pix.bit;
  }

  _mask_clear(FB, ptr, mask);
}

inline void _draw_horizontal_line(microGL_Framebuffer_t *FB, int16_t y, int16_t left, int16_t right) {
  _draw_filled_rectangle(FB, left, y, right, y);
}

inline void _draw_vertical_line(microGL_Framebuffer_t *FB, int16_t x, int16_t top, int16_t bottom) {
  _draw_filled_rectangle(FB, x, top, x, bottom);
}

inline void _mask_set(microGL_Framebuffer_t *FB, uint8_t *ptr, uint8_t mask) {
  switch (FB->draw_mode) {
  case MICROGL_DM_PSET:
  default:
    *ptr |= mask;
    break;
  case MICROGL_DM_PRES:
    *ptr &= ~mask;
    break;
  case MICROGL_DM_PINV:
    *ptr ^= mask;
    break;
  }
}

inline void _mask_clear(microGL_Framebuffer_t *FB, uint8_t *ptr, uint8_t mask) {
  switch (FB->draw_mode) {
  case MICROGL_DM_PSET:
  default:
    *ptr &= ~mask;
    break;
  case MICROGL_DM_PRES:
    *ptr |= mask;
    break;
  case MICROGL_DM_PINV:
    break;
  }
}

static uint8_t _print_char(microGL_Framebuffer_t *FB, int16_t left, int16_t top, uint8_t character, const font_t *fnt) {
  const uint8_t *char_bmp;
  int16_t right;
  int16_t bottom;

  if (character < fnt->min_char || character > fnt->max_char) {
    character = fnt->max_char;
  }

  switch (fnt->scan) {
  case FONT_V:
    char_bmp = &fnt->characters[(character - fnt->min_char) * fnt->width];

    if (fnt->height <= 8) {
      for (right = left + fnt->width; left < right; left++) {
        _draw_byte_vertical(FB, left, top, *char_bmp);
        char_bmp++;
      }
      _draw_byte_vertical(FB, left, top, 0);
      break;
    }

    for (right = left + fnt->width; left < right; left++) {
      for (bottom = top + fnt->height; top > bottom; top += 8) {
        if (*char_bmp != 0) {
          _draw_byte_vertical(FB, left, top, *char_bmp);
        }
        char_bmp++;
      }
    }

    break;
  case FONT_H:
    char_bmp = &fnt->characters[(character - fnt->min_char) * fnt->height];

    if (fnt->width <= 8) {
      for (bottom = top - fnt->height; top > bottom; top--) {
        _draw_byte_horizontal(FB, left, top, *char_bmp);
        char_bmp++;
      }
      break;
    }

    for (bottom = top + fnt->height; top > bottom; top--) {
      for (right = left + fnt->width; left < right; left += 8) {
        if (*char_bmp != 0) {
          _draw_byte_horizontal(FB, left, top, *char_bmp);
        }
        char_bmp++;
      }
    }

    break;
  default:
    break;
  }

  return fnt->width + 1;
}


static inline uint8_t _bitreverse(uint8_t n) {
  static const uint8_t lut[16] = {
  0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
  0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf, };
   // Reverse the top and bottom nibble then swap them.
   return (lut[n&0xf] << 4) | lut[n>>4];
}

void _draw_byte_vertical(microGL_Framebuffer_t *FB, int16_t x, int16_t top, uint8_t byte) {
  _page_byte_bit_t pix;
  _byte_step_t stp;
  uint8_t *ptr;

  if (FB->bit_order == MICROGL_MSB_FIRST) {
    byte = _bitreverse(byte);
  }

  stp = _byte_step(FB);
  pix = _page_byte_bit(FB, x, top);

  ptr = &(FB->buf)[pix.byte];

  if (pix.bit == 7) {
    _mask_set(FB, ptr, byte);
    _mask_clear(FB, ptr, ~byte);
  } else {
    _mask_set(FB, ptr, byte << (7 - pix.bit));
    _mask_clear(FB, ptr, (~byte) << (7 - pix.bit));
    _mask_set(FB, ptr + stp.down, byte >> (pix.bit + 1));
    _mask_clear(FB, ptr + stp.down, (~byte) >> (pix.bit + 1));
  }
}

void _draw_byte_horizontal(microGL_Framebuffer_t *FB, int16_t left, int16_t y, uint8_t byte) {
  for (int16_t x = left; (x < (left + 8)) && (x < FB->width); x++, byte >>= 1) {
    if (x < 0) continue;
    if (byte & 0x01) {
      _set_pixel(FB, x, y);
    } else {
      _reset_pixel(FB, x, y);
    }
  }
}
