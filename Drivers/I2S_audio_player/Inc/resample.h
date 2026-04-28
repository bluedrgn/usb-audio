#pragma once

#include "audio_player.h"

#ifndef MIN
    #define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
    #define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef CLAMP
    #define CLAMP(x,min,max) MAX((min),MIN((max),(x)))
#endif
// AudioSample_q15_t AudioSample_cubic_interpolate_q15(AudioSample_q15_t *s, int16_t x);
AudioSample_f32_t AudioSample_cubic_interpolate_f32(AudioSample_f32_t *s, float x);