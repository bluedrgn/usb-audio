/*
 * waveform.c
 *
 *  Created on: Apr 26, 2026
 *      Author: bluedrgn
 */

#include "visuals/waveform.h"
#include "microGL.h"
#include <math.h>
#include <stddef.h>
#include <stdint.h>


void waveform_init(waveform_TypeDef *wf, waveform_orientation orient,
  int16_t amp, int16_t len, int16_t x, int16_t y, uint32_t scale,
  waveform_minmax_t *buffer) {
  
  wf->orientation = orient;
  wf->amplitude = amp;
  wf->length = len;
  wf->origo_x = x;
  wf->origo_y = y;
  wf->scale = scale;
  wf->buffer = buffer;
  wf->w_ptr = 0;
  wf->scale_remainder = 0;
}

void waveform_update(waveform_TypeDef *wf, float *buf, size_t size) {
  for(; size > 0; size--, buf++) {
    int16_t sample = roundf(*buf * wf->amplitude);

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

void waveform_draw(waveform_TypeDef *wf, microGL_canvas *canvas) {
  uint16_t r_ptr;

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
