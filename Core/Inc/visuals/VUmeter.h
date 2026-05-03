/*
 * UVmeter.h
 *
 *  Created on: Apr 10, 2026
 *      Author: bluedrgn
 */

#pragma once

#include "microGL.h"
#include "audio_player.h"
#include <stdint.h>
#include <math.h>


#ifdef PI
#undef PI
#endif
static const float PI = M_PI;

typedef struct {
  float zero_angle;
  float precession;
  int16_t pivot_x;
  int16_t pivot_y;
  int16_t length;
  float rise_slope;
  float fall_slope;
  float peak;
  float value;
  float position;
  float inertia;
  float sample_rate;
} meter_instance_t;

void meter_init(meter_instance_t *meter, float zero_angle, float max_angle, int16_t pivot_x, int16_t pivot_y, int16_t length);
void meter_start(meter_instance_t *meter, uint32_t sample_rate);
void meter_draw_needle(meter_instance_t *meter, microGL_canvas *canvas);
void meter_update_VU(meter_instance_t *meter, float *buffer, size_t buffer_size);
void meter_update_VU_q15(meter_instance_t *meter, int16_t *buffer, size_t buffer_size);

