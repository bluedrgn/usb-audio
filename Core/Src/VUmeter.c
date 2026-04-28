/*
 * VUmeter.c
 *
 *  Created on: Jul 11, 2024
 *      Author: balazs
 */

#include "VUmeter.h"
#include "arm_math.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

// #include "arm_math.h"
#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef CLAMP
#define CLAMP(x, min, max) (MAX((min), MIN((max), (x))))
#endif

#ifdef PI
#undef PI
#endif
static const float PI = M_PI;

/** https://en.wikipedia.org/wiki/VU_meter */

static const float framerate = 1000.0;  /**< Expected frequency of calling meter_update_VU. */
static const float rise_time = 0.3;
static const float fall_time = 0.3;
static const float P_gain = 30.0;
static const float I_gain = 0.5;
static const float sensitivity_gain = 1.156324;


static void reverseBresenhamsLine(microGL_Framebuffer_t *screen, float angle_rad, int16_t pivot_x, int16_t pivot_y, int16_t length);

static void update_phys(meter_instance_t *meter) {
  float error;

  error = meter->value - meter->position;

  meter->inertia += error;
  meter->position += error * (P_gain / framerate)
          + meter->inertia * (I_gain / framerate);

  if (meter->position <= 0.0f) {
    meter->position = 0.0f;
    meter->inertia = 0.0f;
  }

  if (meter->position >= 1.0f) {
    meter->position = 1.0f;
    meter->inertia = 0.0f;
  }
}

void meter_init(
  meter_instance_t *meter,
  float zero_angle,
  float max_angle,
  uint32_t sample_rate,
  int16_t pivot_x,
  int16_t pivot_y,
  int16_t length)
{
  meter->zero_angle = zero_angle;
  meter->precession = max_angle - zero_angle;
  meter->sample_rate = sample_rate;
  meter->length = length;
  meter->pivot_x = pivot_x;
  meter->pivot_y = pivot_y;
  meter->position = 0;
  meter->inertia = 0;
  meter->peak = 0;
  meter->value = 0;
  #ifdef VU_LOWPASS
  meter->state[0] = 0;
  meter->state[1] = 0;
  meter->state[2] = 0;
  meter->state[3] = 0;
  #endif
  meter->rise_slope = ( 2.0f / rise_time) / meter->sample_rate;
  meter->fall_slope = (-1.0f / fall_time) / meter->sample_rate;
}

void meter_draw_needle(microGL_Framebuffer_t *screen, meter_instance_t *meter) {
  float rad;

  rad = meter->zero_angle + (meter->position * meter->precession);

  reverseBresenhamsLine(screen, rad, meter->pivot_x, meter->pivot_y, meter->length);
}

void meter_update_VU_q15(meter_instance_t *meter, int16_t *buffer, size_t buffer_size) {
  float fbuf[buffer_size];

  arm_q15_to_float(buffer, fbuf, buffer_size);
  meter_update_VU(meter, fbuf, buffer_size);
}

void meter_update_VU(meter_instance_t *meter, float *buffer, size_t buffer_size) {
  #ifdef VU_LOWPASS
  const float coeffs[5] = {2.051734e-02, 2.051734e-02, 0, 9.589653e-01, 0}; //6.6667Hz @ 1KHz
  arm_biquad_casd_df1_inst_f32 LPF = {1, meter->state, coeffs};
  #endif
  float diff;
  float acc;
  
  arm_scale_f32(buffer, sensitivity_gain, buffer, buffer_size);
  arm_abs_f32(buffer, buffer, buffer_size);

  acc = 0;
  for (size_t i = 0; i < buffer_size; i++) {
    diff = buffer[i] - meter->peak;

    if (diff >= 0) meter->peak += MIN(diff, meter->rise_slope);
    else meter->peak += MAX(diff, meter->fall_slope);

    acc += meter->peak;
  }

  #ifdef VU_LOWPASS
  float out = acc / buffer_size;
  arm_biquad_cascade_df1_f32(&LPF, &out, &meter->value, 1);
  #else
  meter->value = acc / buffer_size;
  #endif
  update_phys(meter);
}

static void reverseBresenhamsLine_vertical(microGL_Framebuffer_t *screen, float angle_rad, int16_t pivot_x, int16_t pivot_y, int16_t length) {
  int16_t endpoint_y;
  int16_t x, y, climb, slope;
  
  x = (pivot_x << 8) + 128;
  y = pivot_y;
  slope = -roundf(tanf(angle_rad - PI/2) * 256.f);
  climb =  roundf(sinf(angle_rad) * length);
  endpoint_y = pivot_y + climb;

  if (climb >= 0) {
    while (1) {
      microGL_set_pixel(screen, x/256, y);
      if (++y > endpoint_y) return;
      x += slope;
    }
  }
  else {
    while (1) {
      microGL_set_pixel(screen, x/256, y);
      if (--y < endpoint_y) return;
      x -= slope;
    }
  }
}

static void reverseBresenhamsLine_horizontal(microGL_Framebuffer_t *screen, float angle_rad, int16_t pivot_x, int16_t pivot_y, int16_t length) {
  int16_t endpoint_x;
  int16_t x, y, climb, slope;
  
  x = pivot_x;
  y = (pivot_y << 8) + 128;
  slope = roundf(tanf(angle_rad) * 256.f);
  climb = roundf(cosf(angle_rad) * length);
  endpoint_x = pivot_x + climb;

  if (climb >= 0) {
    while (1) {
      microGL_set_pixel(screen, x, y/256);
      if (++x > endpoint_x) return;
      y += slope;
    }
  }
  else {
    while (1) {
      microGL_set_pixel(screen, x, y/256);
      if (--x < endpoint_x) return;
      y -= slope;
    }
  }
}

static void reverseBresenhamsLine(microGL_Framebuffer_t *screen, float angle_rad, int16_t pivot_x, int16_t pivot_y, int16_t length) {
  while (angle_rad < 0) {
    angle_rad += 2*PI;
  } 
  while (angle_rad >= 2*PI) {
    angle_rad -= 2*PI;
  }
  
  uint8_t quarter = (uint8_t)floorf(angle_rad/(PI/4));

  switch (quarter) {
  case 0:
  case 3:
  case 4:
  case 7:
    reverseBresenhamsLine_horizontal(screen, angle_rad, pivot_x, pivot_y, length);
    break;
  case 1:
  case 2:
  case 5:
  case 6:
    reverseBresenhamsLine_vertical(screen, angle_rad, pivot_x, pivot_y, length);
    break;
  default:
    break;
  }
}
