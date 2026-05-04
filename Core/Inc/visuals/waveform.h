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

typedef enum {
  WAVEFORM_HORIZONTAL,
  WAVEFORM_VERTICAL
} waveform_orientation;

typedef struct waveform_t* waveform_HandleTypeDef;

void waveform_init(waveform_HandleTypeDef *pWaveform, waveform_orientation orientation,
  int16_t amp, int16_t len, int16_t x, int16_t y, uint32_t scale);
void waveform_start(waveform_HandleTypeDef waveform, uint32_t sample_rate);
void waveform_update(waveform_HandleTypeDef waveform, float *buffer, size_t buffer_size);
void waveform_draw(waveform_HandleTypeDef waveform, microGL_canvas *canvas);