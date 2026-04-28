#pragma once

#include <stm32f4xx_hal.h>
// #include <stddef.h>
#include <stdint.h>

#define AUDIOSAMPLE_TYPE AudioSample_q31_t

typedef struct
{
  float L;
  float R;
} AudioSample_f32_t;

typedef struct
{
  int16_t L;
  int16_t R;
} AudioSample_q15_t;

typedef struct
{
  int32_t L;
  int32_t R;
} AudioSample_q31_t;

/* Audio Commands enumeration */
typedef enum
{
  APLAYER_RESET,
  APLAYER_READY,
  APLAYER_START,
  APLAYER_PLAY,
  APLAYER_STOP,
} AudioPlaybackControlTypeDef;

typedef struct
{
  int16_t amount;
  int16_t sample_counter;
} AudioStretchControlTypeDef;

typedef struct
{
  // uint32_t alt_setting;
  I2S_HandleTypeDef* i2sDev;
  AudioPlaybackControlTypeDef playback;
  uint32_t current_sample_rate;
  uint16_t wr_ptr;
  AudioStretchControlTypeDef stretch;
  size_t buffer_size;
  AUDIOSAMPLE_TYPE *buffer;
  AudioSample_f32_t tmpbuf[4];
  uint16_t sync_unit;
  int16_t sync_goal;
  int16_t volume;
  float scaleFactor;
  uint8_t mute;
} AudioPlayer_HandleTypeDef;

void audio_player_init(AudioPlayer_HandleTypeDef *player, I2S_HandleTypeDef* i2sDev, AUDIOSAMPLE_TYPE *buffer, size_t buffer_size);
void audio_player_start(AudioPlayer_HandleTypeDef *player, uint32_t sample_rate);
void audio_player_stop(AudioPlayer_HandleTypeDef *player);
void audio_player_enque_samples(AudioPlayer_HandleTypeDef *player, AudioSample_f32_t *buffer, size_t buffer_size);
/* !Call this function from the corresponding HAL_I2S_TxCpltCallback! */
void audio_player_sync(AudioPlayer_HandleTypeDef *player);
void audio_player_set_volume(AudioPlayer_HandleTypeDef *player, int16_t volume);
void audio_player_set_mute(AudioPlayer_HandleTypeDef *player, uint8_t mute);
