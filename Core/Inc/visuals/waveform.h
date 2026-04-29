/*
 * waveform.h
 *
 *  Created on: Apr 26, 2026
 *      Author: bluedrgn
 */

#pragma once

#include "microGL.h"
#include <stddef.h>
#include <stdint.h>

#define WAVEFORM_BUFFER_SIZE 

typedef enum {
  WAVEFORM_HORIZONTAL,
  WAVEFORM_VERTICAL
} waveform_orientation;

typedef struct {
  int16_t max;
  int16_t min;
} waveform_minmax_t;

typedef struct {
  waveform_orientation orientation;
  int16_t amplitude;
  int16_t length;
  int16_t origo_x;
  int16_t origo_y;
  uint32_t scale;
  waveform_minmax_t *buffer;
  // waveform_minmax_t accu;
  uint16_t w_ptr;
  uint16_t scale_remainder;
} waveform_TypeDef;

void waveform_init(waveform_TypeDef *waveform, waveform_orientation orientation,
  int16_t amp, int16_t len, int16_t x, int16_t y, uint32_t scale,
  waveform_minmax_t *buffer);
void waveform_update(waveform_TypeDef *waveform, float *buffer, size_t buffer_size);
void waveform_draw(waveform_TypeDef *waveform, microGL_canvas *canvas);