/*
 * waveform.h
 *
 *  Created on: Apr 26, 2026
 *      Author: bluedrgn
 */

#pragma once

#include "microGL.h"

typedef struct waveform_t* waveform_HandleTypeDef;

waveform_init(waveform_HandleTypeDef waveform);
waveform_update(waveform_HandleTypeDef waveform);
waveform_draw(waveform_HandleTypeDef waveform);