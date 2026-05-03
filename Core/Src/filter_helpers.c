/*
 * filter_helpers.c
 *
 *  Created on: Apr 26, 2026
 *      Author: bluedrgn
 */

/** https://www.st.com/resource/en/application_note/an2874-bqd-filter-design-equations-stmicroelectronics.pdf */

#include <math.h>

void first_order_lowpass(float *pCoeffs, float cutoff, float samplerate) {
  float theta, K, alpha;
  
  /** The normalized cut-off frequency. */
  theta = 2.0f * (float)M_PI * (cutoff / samplerate);
  K = tanf(theta / 2.0f);
  alpha = 1.0f + K;

  pCoeffs[0] = K / alpha;
  pCoeffs[1] = K / alpha;
  pCoeffs[2] = 0.0f;
  pCoeffs[3] = (1.0f - K) / alpha;
  pCoeffs[4] = 0.0f;
}

void second_order_lowpass(float *pCoeffs, float cutoff, float samplerate, float Q) {
  float theta, K, W, alpha;
  
  /** The normalized cut-off frequency. */
  theta = 2.0f * (float)M_PI * (cutoff / samplerate);
  K = tanf(theta / 2.0f);
  W = powf(K, 2.0f);
  alpha = 1.0f + K / Q + W;

  pCoeffs[0] = W / alpha;
  pCoeffs[1] = 2 * pCoeffs[0];
  pCoeffs[2] = pCoeffs[0];
  pCoeffs[3] = -2.0f * (W - 1.0f) / alpha;
  pCoeffs[4] = (-1.0f + K / Q - W) / alpha;
}

void first_order_highpass(float *pCoeffs, float cutoff, float samplerate) {
  float theta, K, alpha;
  
  /** The normalized cut-off frequency. */
  theta = 2.0f * (float)M_PI * (cutoff / samplerate);
  K = tanf(theta / 2.0f);
  alpha = 1.0f + K;

  pCoeffs[0] = 1.0f / alpha;
  pCoeffs[1] = -1.0f / alpha;
  pCoeffs[2] = 0.0f;
  pCoeffs[3] = (1.0f - K) / alpha;
  pCoeffs[4] = 0.0f;
}

void second_order_highpass(float *pCoeffs, float cutoff, float samplerate, float Q) {
  float theta, K, W, alpha;
  
  /** The normalized cut-off frequency. */
  theta = 2.0f * (float)M_PI * (cutoff / samplerate);
  K = tanf(theta / 2.0f);
  W = powf(K, 2.0f);
  alpha = 1.0f + K / Q + W;

  pCoeffs[0] = 1.0f / alpha;
  pCoeffs[1] = -2.0f / alpha;
  pCoeffs[2] = pCoeffs[0];
  pCoeffs[3] = -2.0f * (W - 1.0f) / alpha;
  pCoeffs[4] = (-1.0f + K / Q - W) / alpha;
}
