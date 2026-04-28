#include "resample.h"
#include <stdint.h>


/* https://developer.arm.com/documentation/dai0033/latest/ */
/* The basic operations perfomed on two numbers x and y of fixed point q15 format returning the answer in q15 format */
// #define FMULL(x,y) (int32_t)(((int64_t)(x)*(int64_t)(y))>>15)
// #define FMUL(x,y) (((int32_t)(x)*(y))>>(15))


// static int16_t cubic_interpolate_q15(int16_t *v, int16_t x)
// {
//   int32_t a = (int32_t)v[0] - (int32_t)v[2];
//   int32_t b = 3 * a;
//   int32_t c = b + (int32_t)v[4];
//   int32_t d = c - (int32_t)v[-2];
//   int32_t e = FMULL(x, d);
//   int32_t f = 2 * (int32_t)v[-2];
//   int32_t g = 5 * (int32_t)v[0];
//   int32_t h = 4 * (int32_t)v[2];
//   int32_t i = f - g;
//   int32_t j = i + h;
//   int32_t k = j - (int32_t)v[4];
//   int32_t l = k + e;
//   int32_t m = FMULL(x, l);
//   int32_t n = (int32_t)v[2] - (int32_t)v[-2];
//   int32_t o = n + m;
//   int32_t p = FMULL(x, o);
//   int32_t q = p / 2;
//   int32_t r = (int32_t)v[0] + q;
//   return CLAMP(r, INT16_MIN, INT16_MAX);
// }


// AudioSample_q15_t AudioSample_cubic_interpolate_q15(AudioSample_q15_t *s, int16_t x)
// {
//   AudioSample_q15_t ret;
//   ret.L = cubic_interpolate_q15(&s->L, x);
//   ret.R = cubic_interpolate_q15(&s->R, x);
//   return ret;
// }


static float cubic_interpolate_f32(float *v, float x)
{
  float a = v[0] - v[2];
  float b = a * 3.0f;
  float c = b + v[4];
  float d = c - v[-2];
  float e = d * x;
  float f = v[-2] * 2.0f;
  float g = v[0] * 5.0f;
  float h = v[2] * 4.0f;
  float i = f - g;
  float j = i + h;
  float k = j - v[4];
  float l = k + e;
  float m = l * x;
  float n = v[2] - v[-2];
  float o = n + m;
  float p = o * x;
  float q = p * 0.5f;
  float r = v[0] + q;
  return r;
}

AudioSample_f32_t AudioSample_cubic_interpolate_f32(AudioSample_f32_t *s, float x)
{
  AudioSample_f32_t ret;
  ret.L = cubic_interpolate_f32(&s->L, x);
  ret.R = cubic_interpolate_f32(&s->R, x);
  return ret;
}

// void test()
// {
//   int32_t input[4] = {0, 0x7FFF, 0x7FFF, 0};
//   static int32_t output[256] __attribute__((used));

//   for(uint16_t x = 0; x < 256; x++)
//   {
//     output[x] = cubic_interpolate(input, x<<8);
//   }

//   (void)output;
//   asm("NOP");
// }

// void test() {
//   uint32_t input[256 + 4];
//   for (int i = 0; i < 256 + 4; i++) {
//     input[i] = (uint32_t)rand();
//   }

//   printf("%lu BEGIN\r\n", HAL_GetTick());
//   for (uint32_t cnt = 0; cnt < 1000000; cnt++) {
//     int i = (cnt & 0xFFFF) >> 8;
//     uint16_t x = cnt & 0xFFFF;
//     AudioSample_cubic_interpolate((AudioSample_q15_t *)&input[i] + 1, x);
//   }

//   printf("%lu END\r\n", HAL_GetTick());
// }

// void AudioSample_LowpassFilter(AudioSample_q15_t *stage, AudioSample_q15_t in)
// {
//   static const int32_t coeff_a = 679; /*0.0103634 ~80Hz*/
//   /* filter_output = filter_output + a * (new_sample - filter_output) */
//   stage->L = CLAMP(stage->L + FMUL(coeff_a, (int32_t)in.L - stage->L), INT16_MIN, INT16_MAX);
//   stage->R = CLAMP(stage->R + FMUL(coeff_a, (int32_t)in.R - stage->R), INT16_MIN, INT16_MAX);
// }

// AudioSample_q15_t AudioSample_BassBoost(AudioSample_q15_t in)
// {
//   static AudioSample_q15_t stage1, stage2;
//   AudioSample_q15_t out;
//   AudioSample_LowpassFilter(&stage1, in);
//   AudioSample_LowpassFilter(&stage2, stage1);
//   out.L = in.L + stage2.L;
//   out.R = in.R + stage2.R;
//   return out;  
// }

// #include <arm_math.h>
// AudioSample_q15_t AudioSample_BassBoost(AudioSample_q15_t in)
// {
//   static q63_t states[8] = {0,0,0,0,0,0,0,0};
//   /* https://www.earlevel.com/main/2021/09/02/biquad-calculator-v3/ */
//   /* coeffs: 0.9981385387143642 -1.9962770774287284 0.9981385387143642 1.9962497122012544 -0.9963044426562024 */
//   const q31_t coeffs[5] = {1071743095, -2143486190, 1071743095, 2143456807, -1069773750};
//   const arm_biquad_cas_df1_32x64_ins_q31 LPF_L = {1, states, coeffs, 1};
//   const arm_biquad_cas_df1_32x64_ins_q31 LPF_R = {1, states+4, coeffs, 1};
//   AudioSample_q15_t out;
//   q31_t in_L, in_R, out_L, out_R;
//   in_L = in.L << 15;
//   in_R = in.R << 15;
//   arm_biquad_cas_df1_32x64_q31(&LPF_L, &in_L, &out_L, 1);
//   arm_biquad_cas_df1_32x64_q31(&LPF_R, &in_R, &out_R, 1);
//   out.L = CLAMP(out_L>>16, INT16_MIN, INT16_MAX);
//   out.R = CLAMP(out_R>>16, INT16_MIN, INT16_MAX);
//   return out;
// }