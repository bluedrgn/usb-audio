/*
 * audio_player.h
 *
 *  Created on: Apr 10, 2026
 *      Author: bluedrgn
 */

#pragma once

#include <stm32f4xx_hal.h>
// #include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum __attribute__((__packed__)) {
  APLAYER_I2S_16BIT = 16,
  APLAYER_I2S_32BIT = 32,
}AudioPlayer_I2S_bitdepth;

typedef struct AudioPlayer_t* AudioPlayer_HandleTypeDef;

void audio_player_init(AudioPlayer_HandleTypeDef *player, I2S_HandleTypeDef* i2sDev, uint16_t bufferSize_ms, AudioPlayer_I2S_bitdepth bitDepth);
bool audio_player_is_playing(AudioPlayer_HandleTypeDef player);
void audio_player_start(AudioPlayer_HandleTypeDef player, uint32_t sample_rate);
void audio_player_stop(AudioPlayer_HandleTypeDef player);
void audio_player_enque_samples(AudioPlayer_HandleTypeDef player, float *buffer, size_t buffer_size);
/* !Call this function from the corresponding HAL_I2S_TxCpltCallback! */
void audio_player_sync(AudioPlayer_HandleTypeDef player);
void audio_player_set_volume(AudioPlayer_HandleTypeDef player, float volume_dB);
void audio_player_set_mute(AudioPlayer_HandleTypeDef player, uint8_t mute);
