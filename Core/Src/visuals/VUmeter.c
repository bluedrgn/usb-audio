/*
 * UVmeter.c
 *
 *  Created on: Apr 10, 2026
 *      Author: bluedrgn
 */

#include "visuals/VUmeter.h"
#include "arm_math.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


#ifndef MIN
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef CLAMP
#define CLAMP(x, min, max) (MAX((min), MIN((max), (x))))
#endif


/** https://en.wikipedia.org/wiki/VU_meter */

static const float rise_time = 0.3;
static const float fall_time = 0.3;
static const float P_gain = 30.0;
static const float I_gain = 0.5;
static const float sensitivity_gain = M_2_SQRTPI;

static void update_phys(meter_instance_t *meter, float time);
static void reverseBresenhamsLine(microGL_canvas *canvas, float angle_rad, int16_t pivot_x, int16_t pivot_y, int16_t length);

void meter_init(
  meter_instance_t *meter,
  float zero_angle,
  float max_angle,
  int16_t pivot_x,
  int16_t pivot_y,
  int16_t length)
{
  meter->zero_angle = zero_angle;
  meter->precession = max_angle - zero_angle;
  meter->pivot_x = pivot_x;
  meter->pivot_y = pivot_y;
  meter->length = length;
}

void meter_start(meter_instance_t *meter, uint32_t sample_rate) {
  meter->rise_slope = (   PI / rise_time) / (float)sample_rate;
  meter->fall_slope = (-1.0f / fall_time) / (float)sample_rate;
  meter->peak = 0.0f;
  meter->value = 0.0f;
  meter->position = 0.0f;
  meter->inertia = 0.0f;
  meter->sample_rate = (float)sample_rate;
}

void meter_draw_needle(meter_instance_t *meter, microGL_canvas *canvas) {
  float rad;

  rad = meter->zero_angle + (meter->position * meter->precession);

  reverseBresenhamsLine(canvas, rad, meter->pivot_x, meter->pivot_y, meter->length);
}

void meter_update_VU_q15(meter_instance_t *meter, int16_t *buffer, size_t buffer_size) {
  float fbuf[buffer_size];

  arm_q15_to_float(buffer, fbuf, buffer_size);
  meter_update_VU(meter, fbuf, buffer_size);
}

void meter_update_VU(meter_instance_t *meter, float *buffer, size_t buffer_size) {
  float diff, acc, time;
  
  arm_scale_f32(buffer, sensitivity_gain, buffer, buffer_size);
  arm_abs_f32(buffer, buffer, buffer_size);

  acc = 0;
  for (size_t i = 0; i < buffer_size; i++) {
    diff = buffer[i] - meter->peak;

    if (diff >= 0) meter->peak += MIN(diff, meter->rise_slope);
    else meter->peak += MAX(diff, meter->fall_slope);

    acc += meter->peak;
  }

  meter->value = acc / buffer_size;

  time = (float)buffer_size / meter->sample_rate;
  update_phys(meter, time);
}

static void update_phys(meter_instance_t *meter, float time) {
  float error;

  error = meter->value - meter->position;

  meter->inertia += error;
  meter->position += error * P_gain * time
          + meter->inertia * I_gain * time;

  if (meter->position <= 0.0f) {
    meter->position = 0.0f;
    meter->inertia = 0.0f;
  }

  if (meter->position >= 1.0f) {
    meter->position = 1.0f;
    meter->inertia = 0.0f;
  }
}

static void reverseBresenhamsLine_vertical(microGL_canvas *canvas, float angle_rad, int16_t pivot_x, int16_t pivot_y, int16_t length) {
  int16_t endpoint_y;
  int16_t x, y, climb, slope;
  
  x = (pivot_x << 8) + 128;
  y = pivot_y;
  slope = -roundf(tanf(angle_rad - PI/2) * 256.f);
  climb =  roundf(sinf(angle_rad) * length);
  endpoint_y = pivot_y + climb;

  if (climb >= 0) {
    while (1) {
      microGL_set_pixel(canvas, x/256, y);
      if (++y > endpoint_y) return;
      x += slope;
    }
  }
  else {
    while (1) {
      microGL_set_pixel(canvas, x/256, y);
      if (--y < endpoint_y) return;
      x -= slope;
    }
  }
}

static void reverseBresenhamsLine_horizontal(microGL_canvas *canvas, float angle_rad, int16_t pivot_x, int16_t pivot_y, int16_t length) {
  int16_t endpoint_x;
  int16_t x, y, climb, slope;
  
  x = pivot_x;
  y = (pivot_y << 8) + 128;
  slope = roundf(tanf(angle_rad) * 256.f);
  climb = roundf(cosf(angle_rad) * length);
  endpoint_x = pivot_x + climb;

  if (climb >= 0) {
    while (1) {
      microGL_set_pixel(canvas, x, y/256);
      if (++x > endpoint_x) return;
      y += slope;
    }
  }
  else {
    while (1) {
      microGL_set_pixel(canvas, x, y/256);
      if (--x < endpoint_x) return;
      y -= slope;
    }
  }
}

static void reverseBresenhamsLine(microGL_canvas *canvas, float angle_rad, int16_t pivot_x, int16_t pivot_y, int16_t length) {
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
    reverseBresenhamsLine_horizontal(canvas, angle_rad, pivot_x, pivot_y, length);
    break;
  case 1:
  case 2:
  case 5:
  case 6:
    reverseBresenhamsLine_vertical(canvas, angle_rad, pivot_x, pivot_y, length);
    break;
  default:
    break;
  }
}
