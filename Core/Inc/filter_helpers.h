/*
 * filter_helpers.h
 *
 *  Created on: Apr 26, 2026
 *      Author: bluedrgn
 */

#pragma once

void first_order_lowpass(float *pCoeffs, float cutoff, float samplerate);
void second_order_lowpass(float *pCoeffs, float cutoff, float samplerate, float Q);
void first_order_highpass(float *pCoeffs, float cutoff, float samplerate);
void second_order_highpass(float *pCoeffs, float cutoff, float samplerate, float Q);
