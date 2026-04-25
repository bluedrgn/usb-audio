#pragma once

#include "audio_player.h"

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define CLAMP(x,min,max) MAX((min),MIN((max),(x)))

AudioSample_q15_t AudioSample_cubic_interpolate(AudioSample_q15_t *s, int16_t x);