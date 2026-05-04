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

typedef struct VUmeter_t* VUmeter_HandleTypeDef; 

#ifdef PI
#undef PI
#endif
static const float PI = M_PI;

void meter_init(VUmeter_HandleTypeDef *pMeter, float zero_angle, float max_angle, int16_t pivot_x, int16_t pivot_y, int16_t length);
void meter_start(VUmeter_HandleTypeDef meter, uint32_t sample_rate);
void meter_draw_needle(VUmeter_HandleTypeDef meter, microGL_canvas *canvas);
void meter_update_VU(VUmeter_HandleTypeDef meter, float *buffer, size_t buffer_size);
void meter_update_VU_q15(VUmeter_HandleTypeDef meter, int16_t *buffer, size_t buffer_size);

