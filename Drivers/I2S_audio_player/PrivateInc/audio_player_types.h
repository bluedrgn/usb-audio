/*
 * audio_player_types.h
 *
 *  Created on: Apr 10, 2026
 *      Author: bluedrgn
 */

#pragma once

typedef struct
{
  float L;
  float R;
} AudioSample_f32_t;

typedef struct {
  int16_t L;
  int16_t R;
} AudioSample_q15_t;

typedef struct {
  int32_t L;
  int32_t R;
} AudioSample_q31_t;

// typedef union {
//   AudioSample_q15_t q15;
//   AudioSample_q31_t q31;
// } AudioSample_t;
