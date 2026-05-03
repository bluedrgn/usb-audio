/*
 * bouncing_bars.c
 *
 *  Created on: Apr 26, 2026
 *      Author: bluedrgn
 */

#include "visuals/bouncing_bars.h"
#include "dsp/basic_math_functions.h"
#include "dsp/complex_math_functions.h"
#include "dsp/fast_math_functions.h"
#include "dsp/statistics_functions.h"
#include "dsp/support_functions.h"
#include "dsp/transform_functions.h"
#include "dsp/window_functions.h"
#include "main.h"
#include "microGL.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

static const float fall_time = 0.5f;

struct bouncing_bars_t{
  uint8_t size;
  int16_t spacing;
  int16_t width;
  int16_t length;
  int16_t origo_x;
  int16_t origo_y;
  uint16_t fft_size;
  uint16_t buf_remainder;
  uint32_t samplerate;
  float *window;
  float *sample_buf;
  float *bins;
  float *bars;
  float fall;
  arm_rfft_fast_instance_f32 FFT;
};

void bouncing_bars_init(bouncing_bars_HandleTypeDef *bbptr, uint8_t size, uint16_t spacing,
                        int16_t width, int16_t length, int16_t origo_x,
                        int16_t origo_y) {
  bouncing_bars_HandleTypeDef bb = malloc(sizeof(struct bouncing_bars_t));
  *bbptr = bb;
  bb->size = size;
  bb->spacing = spacing;
  bb->width = width;
  bb->length = length;
  bb->origo_x = origo_x;
  bb->origo_y = origo_y;
  bb->fft_size = 2U << size;
  bb->samplerate = 0;
  bb->window = malloc(bb->fft_size * sizeof(float));
  bb->sample_buf = malloc(bb->fft_size * sizeof(float));
  bb->bins = malloc((bb->fft_size / 2) * sizeof(float));
  bb->bars = malloc(bb->size * sizeof(float));

  if (!bb->window || !bb->sample_buf || !bb->bars) {
    Error_Handler();
  }

  // arm_hamming_f32(bb->window, bb->fft_size);
  arm_hanning_f32(bb->window, bb->fft_size);
  // arm_hft90d_f32(bb->window, bb->fft_size);
  arm_status status = arm_rfft_fast_init_f32(&bb->FFT, bb->fft_size);
  if (status != ARM_MATH_SUCCESS) {
    Error_Handler();
  }
}

void bouncing_bars_start(bouncing_bars_HandleTypeDef bb, uint32_t sample_rate) {
  bb->samplerate = sample_rate;
  bb->buf_remainder = 0;
  bb->fall = ((float)(1U << bb->size) / (float)sample_rate) / fall_time;
  arm_fill_f32(0.0f, bb->sample_buf, bb->fft_size);
  arm_fill_f32(0.0f, bb->bins, bb->fft_size / 2);
  arm_fill_f32(0.0f, bb->bars, bb->size);
}

void shift_buffer(bouncing_bars_HandleTypeDef bb, float *buffer, uint16_t buffer_size) {
  arm_copy_f32(bb->sample_buf + buffer_size, bb->sample_buf, bb->fft_size - buffer_size);
  arm_copy_f32(buffer, bb->sample_buf + (bb->fft_size - buffer_size), buffer_size);
}

void do_fft(bouncing_bars_HandleTypeDef bb) {
  float temp1[bb->fft_size];
  float temp2[bb->fft_size];
  arm_mult_f32(bb->sample_buf, bb->window, temp1, bb->fft_size);
  arm_rfft_fast_f32(&bb->FFT, temp1, temp2, 0);
  arm_cmplx_mag_f32(temp2, temp1, bb->fft_size / 2);
  arm_scale_f32(temp1, (float)M_SQRT2 / bb->fft_size, temp2, bb->fft_size / 2);

  for (uint16_t i = 0; i < (bb->fft_size / 2); i++) {
    bb->bins[i] = bb->bins[i] * (1.0f - bb->fall) + temp2[i] * bb->fall;
  }
  arm_offset_f32(bb->bars, -1.0f * bb->fall, bb->bars, bb->size);
  for (uint8_t i = 0; i < bb->size; i++) {
    float out;
    uint16_t ptr = (1U << i);
    arm_accumulate_f32(temp2 + ptr, ptr, &out);
    if (i == (bb->size - 1)) {
      out *= (float)M_SQRT2;
    }
    arm_sqrt_f32(out, &out);
    if (bb->bars[i] < out) {
      bb->bars[i] = out;
    }
  }
  arm_clip_f32(bb->bars, bb->bars, 0.0f, 1.0f, bb->size);
}

void bouncing_bars_update(bouncing_bars_HandleTypeDef bb, float *buffer, uint16_t buffer_size) {
  uint16_t fft_overlap_size = bb->fft_size / 2;
  if (bb->buf_remainder) {
    if(buffer_size >= bb->buf_remainder) {
      shift_buffer(bb, buffer, bb->buf_remainder);
      buffer += bb->buf_remainder;
      buffer_size -= bb->buf_remainder;
      bb->buf_remainder = 0;
      do_fft(bb);
    }
    else {
      shift_buffer(bb, buffer, buffer_size);
      bb->buf_remainder -= buffer_size;
      return;
    }
  }

  while (buffer_size >= fft_overlap_size) {
    shift_buffer(bb, buffer, fft_overlap_size);
    buffer += fft_overlap_size;
    buffer_size -= fft_overlap_size;
    do_fft(bb);
  }

  if (buffer_size) {
    shift_buffer(bb, buffer, buffer_size);
    bb->buf_remainder = fft_overlap_size - buffer_size;
  }
}


void bouncing_bars_draw(bouncing_bars_HandleTypeDef bb, microGL_canvas *canvas) {
  for (uint8_t i = 0; i < bb->size; i++) {
    int16_t barlen = roundf(bb->bars[i] * (float)bb->length);
    // if (barlen == 0) continue;
    int16_t x1 = bb->origo_x + i * (bb->width + bb->spacing);
    int16_t y1 = bb->origo_y;
    int16_t x2 = x1 + bb->width + ((bb->width >= 0) ? -1 : 1);
    int16_t y2 = y1 + barlen/* + ((barlen >= 0) ? -1 : 1)*/;
    microGL_draw_rectangle(canvas, x1, y1, x2 ,y2 ,1);
  }
}
