/*
 * waveform.c
 *
 *  Created on: Apr 26, 2026
 *      Author: bluedrgn
 */

#include "visuals/waveform.h"
#include "filter_helpers.h"
#include "dsp/filtering_functions.h"
#include "dsp/support_functions.h"
#include "main.h"
#include "microGL.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>


typedef struct {
  int16_t max;
  int16_t min;
} waveform_minmax_t;

struct waveform_t {
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
  float LPF_pState[4];
  float LPF_pCoeffs[5];
};


void waveform_init(waveform_HandleTypeDef *wfptr, waveform_orientation orient,
  int16_t amp, int16_t len, int16_t x, int16_t y, uint32_t scale) {
  if (!wfptr || !len) Error_Handler();
  waveform_HandleTypeDef wf = malloc(sizeof(struct waveform_t));
  if (!wf) Error_Handler();
  *wfptr = wf;
  wf->orientation = orient;
  wf->amplitude = amp;
  wf->length = len;
  wf->origo_x = x;
  wf->origo_y = y;
  wf->scale = scale;
  wf->buffer = malloc(abs(len) * sizeof(waveform_minmax_t));
  wf->w_ptr = 0;
  wf->scale_remainder = 0;
  if (!wf->buffer) Error_Handler();
}

void waveform_start(waveform_HandleTypeDef wf, uint32_t sample_rate) {
  if (!wf) Error_Handler();
  arm_fill_q31(0, (int32_t*)wf->buffer, abs(wf->length));
  arm_fill_f32(0.0f, wf->LPF_pState, 4);
  second_order_lowpass(wf->LPF_pCoeffs, (float)sample_rate / (float)wf->scale, (float)sample_rate, M_SQRT1_2);
}

void waveform_update(waveform_HandleTypeDef wf, float *buf, size_t size) {
  float input[size];

  if (!wf) Error_Handler();

  arm_biquad_casd_df1_inst_f32 LPF = {.numStages = 1, .pCoeffs = wf->LPF_pCoeffs, .pState = wf->LPF_pState};
  arm_biquad_cascade_df1_f32(&LPF, buf, input, size);

  for(size_t i = 0; i < size; i++) {
    int16_t sample = roundf(input[i] * wf->amplitude);

    if (wf->scale_remainder == 0) {
      wf->w_ptr++;
      wf->w_ptr %= wf->length;
      wf->buffer[wf->w_ptr] = (waveform_minmax_t){.max = sample, .min = sample};
    }
    else {
      if (wf->buffer[wf->w_ptr].max < sample)
        wf->buffer[wf->w_ptr].max = sample;
      else if (wf->buffer[wf->w_ptr].min > sample)
        wf->buffer[wf->w_ptr].min = sample;
    }

    wf->scale_remainder++;
    wf->scale_remainder %= wf->scale;
  }
}

void waveform_draw(waveform_HandleTypeDef wf, microGL_canvas *canvas) {
  uint16_t r_ptr;

  if (!wf) Error_Handler();

  r_ptr = wf->w_ptr + 1;

  if (wf->orientation == WAVEFORM_HORIZONTAL) {
    for (int16_t x = wf->origo_x; x != (wf->origo_x + wf->length); x += (wf->length >= 0) ? 1 : -1) {
      r_ptr %= wf->length;
      microGL_draw_vertical_line(canvas, x, wf->origo_y + wf->buffer[r_ptr].min, wf->origo_y + wf->buffer[r_ptr].max);
      // int16_t y = wf->origo_y + (wf->buffer[r_ptr].min + wf->buffer[r_ptr].max);
      // microGL_set_pixel(canvas, x, y);
      r_ptr++;
    }
  }
  else {
    for (int16_t y = wf->origo_y; y != (wf->origo_y + wf->length); y += (wf->length >= 0) ? 1 : -1) {
      r_ptr %= wf->length;
      microGL_draw_horizontal_line(canvas, wf->origo_x + wf->buffer[r_ptr].min, wf->origo_x + wf->buffer[r_ptr].max, y);
      // int16_t x = wf->origo_x + (wf->buffer[r_ptr].min + wf->buffer[r_ptr].max);
      // microGL_set_pixel(canvas, x, y);
      r_ptr++;
    }
  }
}
