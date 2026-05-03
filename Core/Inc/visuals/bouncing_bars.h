/*
 * bouncing_bars.h
 *
 *  Created on: Apr 26, 2026
 *      Author: bluedrgn
 */

#pragma once

#include "dsp/transform_functions.h"
#include "microGL.h"
#include <stdint.h>
#include <sys/types.h>


typedef struct bouncing_bars_t* bouncing_bars_HandleTypeDef;

void bouncing_bars_init(bouncing_bars_HandleTypeDef *bars, uint8_t size, uint16_t spacing,
                        int16_t width, int16_t length, int16_t origo_x,
                        int16_t origo_y);
void bouncing_bars_start(bouncing_bars_HandleTypeDef bars, uint32_t sample_rate);
void bouncing_bars_update(bouncing_bars_HandleTypeDef bars, float *buffer, uint16_t buffer_size);
void bouncing_bars_draw(bouncing_bars_HandleTypeDef bars, microGL_canvas *canvas);