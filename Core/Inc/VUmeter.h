/*
 * UVmeter.h
 *
 *  Created on: Jul 11, 2024
 *      Author: balaz
 */

#ifndef INC_VUMETER_H_
#define INC_VUMETER_H_

#include "microGL.h"
#include "audio_player.h"
#include <stdint.h>

// #define VU_LOWPASS

typedef struct {
  float zero_angle;
  float precession;
  uint32_t sample_rate;
  int16_t length;
  int16_t pivot_x;
  int16_t pivot_y;
  float position;
  float inertia;
  float peak;
  float value;
  #ifdef VU_LOWPASS
  float state[4];
  #endif
  float rise_slope;
  float fall_slope;
} meter_instance_t;

void meter_init(meter_instance_t *meter, float zero_angle, float max_angle, uint32_t sample_rate, int16_t pivot_x, int16_t pivot_y, int16_t length);
void meter_draw_needle(microGL_Framebuffer_t *screen, meter_instance_t *meter);
void meter_update_VU(meter_instance_t *meter, float *buffer, size_t buffer_size);
void meter_update_VU_q15(meter_instance_t *meter, int16_t *buffer, size_t buffer_size);

#endif /* INC_VUMETER_H_ */
