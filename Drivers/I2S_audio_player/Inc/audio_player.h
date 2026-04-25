#pragma once

#include <stm32f4xx_hal.h>
// #include <stddef.h>
#include <stdint.h>

typedef struct
{
  int16_t L;
  int16_t R;
} AudioSample_q15_t;

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
  int16_t offset;
  uint16_t wr_ptr;
  AudioStretchControlTypeDef stretch;
  size_t buffer_size;
  AudioSample_q15_t *buffer;
  AudioSample_q15_t tmpbuf[4];
  uint16_t sync_unit;
  int16_t sync_goal;
} AudioPlayer_HandleTypeDef;

void audio_player_init(AudioPlayer_HandleTypeDef *player, I2S_HandleTypeDef* i2sDev, AudioSample_q15_t *buffer, size_t buffer_size);
void audio_player_start(AudioPlayer_HandleTypeDef *player, uint32_t sample_rate);
void audio_player_stop(AudioPlayer_HandleTypeDef *player);
void audio_player_enque_samples(AudioPlayer_HandleTypeDef *player, AudioSample_q15_t *buffer, size_t buffer_size);
/* !Call this function from the corresponding HAL_I2S_TxCpltCallback! */
void audio_player_sync(AudioPlayer_HandleTypeDef *player);