/*
 * audio_player.c
 *
 *  Created on: Apr 10, 2026
 *      Author: bluedrgn
 */

#include "arm_math_types.h"
#include "dsp/basic_math_functions.h"
#include "dsp/support_functions.h"
#include "main.h"
#include "audio_player.h"
#include "resample.h"
#include "audio_player_types.h"
#include "stm32f4xx_hal_i2s.h"
#include <math.h>
#include <stddef.h>
#ifdef DEBUG_PRINT
#include <stdio.h>
#endif /*DEBUG*/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


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

struct AudioPlayer_t
{
  I2S_HandleTypeDef* i2sDev;
  AudioPlaybackControlTypeDef playback;
  AudioPlayer_I2S_bitdepth bitdepth;
  uint32_t current_sample_rate;
  uint16_t wr_ptr;
  AudioStretchControlTypeDef stretch;
  uint16_t buffer_size_ms;
  uint16_t buffer_size_samples;
  uint8_t *buffer;
  AudioSample_f32_t tmpbuf[4];
  uint16_t sync_unit;
  uint16_t sync_goal;
  float volume;
  float scaleFactor;
  uint8_t mute;
};

static const size_t chs = 2;  /* Stereo! */

void __play(AudioPlayer_HandleTypeDef player);
void __stop(AudioPlayer_HandleTypeDef player);

void audio_player_init(AudioPlayer_HandleTypeDef *player_ptr, I2S_HandleTypeDef* i2sDev, uint16_t buffer_size_ms, AudioPlayer_I2S_bitdepth bitDepth) {
  if (!player_ptr || !i2sDev) Error_Handler();
  AudioPlayer_HandleTypeDef player = malloc(sizeof(struct AudioPlayer_t));
  if (!player) Error_Handler();
  *player_ptr = player;
  player->i2sDev = i2sDev;
  player->buffer = NULL;
  player->buffer_size_ms = buffer_size_ms;
  player->bitdepth = bitDepth;
  player->playback = APLAYER_RESET;
}

bool audio_player_is_playing(AudioPlayer_HandleTypeDef player) {
  switch (player->playback) {
  // case APLAYER_START:
  case APLAYER_PLAY:
    return true;
  default:
    return false;
  }
}

void audio_player_start(AudioPlayer_HandleTypeDef player, uint32_t sample_rate) {
  if (player->playback == APLAYER_RESET)
  {
    memset(player->tmpbuf, 0, sizeof(player->tmpbuf));
    player->wr_ptr = 0;
    player->playback = APLAYER_READY;
  }

  if (player->playback == APLAYER_READY)
  {
    player->buffer_size_samples = (sample_rate * player->buffer_size_ms) / 1000U;
    player->sync_goal = player->buffer_size_samples * 2 / 3;
    if (player->buffer != NULL) free(player->buffer);
    switch (player->bitdepth) {
    case APLAYER_I2S_16BIT:
      player->buffer = malloc(player->buffer_size_samples * sizeof(AudioSample_q15_t));
      break;
    case APLAYER_I2S_32BIT:
      player->buffer = malloc(player->buffer_size_samples * sizeof(AudioSample_q31_t));
      break;
    }
    if (player->buffer == NULL) Error_Handler();
    player->current_sample_rate = sample_rate;
    player->sync_unit = sample_rate / 1000U;
    player->stretch.amount = 0;
    player->stretch.sample_counter = 0;
    arm_fill_f32(0.0f, (float*)player->tmpbuf, 4);
    player->playback = APLAYER_START;
  }
}

void audio_player_stop(AudioPlayer_HandleTypeDef player) {
  if (player->playback == APLAYER_PLAY)
  {
    player->playback = APLAYER_STOP;
  }
}

void audio_player_enque_samples(AudioPlayer_HandleTypeDef player, float *buf_f32, size_t buffer_size) {
  if (!player || !buf_f32 || !buffer_size) Error_Handler();

  AudioSample_f32_t tmp_in[(buffer_size / chs) + 4];

  #ifdef DEBUG_PRINT
  // printf("  Enqueue %p %u\r\n", buff, size);
  #endif

  /* caching the last 4 samples from the end of every chunk */
  arm_copy_f32((float*)player->tmpbuf, (float*)&tmp_in[0], 4 * chs);
  arm_scale_f32(buf_f32, player->scaleFactor, (float*)&tmp_in[4], buffer_size);
  // arm_copy_f32((float*)buffer, (float*)&tmp_in[4].L, buffer_size * chs);
  arm_copy_f32((float*)&tmp_in[buffer_size / chs], (float*)player->tmpbuf, 4 * chs);


  for (size_t rd = 1; rd <= (buffer_size / chs); rd++) {
    AudioSample_f32_t sample_in;
    float subsample_shift_amount;

    player->stretch.sample_counter += player->stretch.amount;
    if (player->stretch.sample_counter < 0) {
      player->stretch.sample_counter &= 0x7FFF;
      if (player->stretch.amount >= 0) {
        rd++;
      }
      else {
        rd--;
      }
    }

    subsample_shift_amount = player->stretch.sample_counter / 32768.0f;
    sample_in = AudioSample_cubic_interpolate_f32(&tmp_in[rd], subsample_shift_amount);
    // sample_out = tmp_in[rd];
    switch (player->bitdepth) {
    case APLAYER_I2S_16BIT:
      arm_float_to_q15((float *)&sample_in, (q15_t *)&((AudioSample_q15_t*)player->buffer)[player->wr_ptr], chs);
      break;
    case APLAYER_I2S_32BIT:
      AudioSample_q31_t *sample_out = &((AudioSample_q31_t*)player->buffer)[player->wr_ptr];
      arm_float_to_q31((float *)&sample_in, (q31_t *)sample_out, chs);
      /* https://community.st.com/t5/stm32-mcus-products/stm32f7-i2s-dma-byte-order-issue/td-p/382535 */
      /* https://stackoverflow.com/questions/48541818/swap-halfwords-efficently */
      sample_out->L = (sample_out->L << 16) | (sample_out->L >> 16);
      sample_out->R = (sample_out->R << 16) | (sample_out->R >> 16);
      break;
    }
    player->wr_ptr++;
    player->wr_ptr %= player->buffer_size_samples;
  }

  if ((player->playback == APLAYER_START) && (player->wr_ptr >= player->sync_goal)) {
    __play(player);
  }
}

void __play(AudioPlayer_HandleTypeDef player) {
  if (!player->buffer) Error_Handler();

  switch (player->bitdepth) {
  case APLAYER_I2S_16BIT:
    player->i2sDev->Init.DataFormat = I2S_DATAFORMAT_16B;
    break;
  case APLAYER_I2S_32BIT:
    player->i2sDev->Init.DataFormat = I2S_DATAFORMAT_32B;
    break;
  }
  player->i2sDev->Init.AudioFreq = player->current_sample_rate;
  HAL_I2S_Init(player->i2sDev);
  HAL_I2S_Transmit_DMA(player->i2sDev, (uint16_t*)player->buffer, player->buffer_size_samples * chs);
  player->playback = APLAYER_PLAY;
}

void __stop(AudioPlayer_HandleTypeDef player) {
  HAL_I2S_DMAStop(player->i2sDev);
  if (player->buffer != NULL) free(player->buffer);
  player->buffer = NULL;
  player->playback = APLAYER_RESET;
}


/* !Call this function from the corresponding HAL_I2S_TxCpltCallback! */
void audio_player_sync(AudioPlayer_HandleTypeDef player) {
  int16_t offset;
  /* read pointer should be 0 at this point */
  if (player == NULL) {
    return;
  }

  if (player->playback == APLAYER_STOP) {
    __stop(player);
    // #ifdef DEBUG_PRINT
    // HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    // #endif
    return;
  }

  offset = player->wr_ptr - player->sync_goal;
  if (offset < 0) {
    player->stretch.amount = (offset-(player->sync_unit-1))/player->sync_unit;
  }
  else {
    player->stretch.amount = (offset+(player->sync_unit-1))/player->sync_unit;
  }

  // #ifdef DEBUG_PRINT
  // printf("%08lX Sync %+5d %+5d\r", HAL_GetTick(), offset, player->stretch.amount);
  // if (player->playback == APLAYER_RESET) {
  //   putchar('\n');
  // } else {
  //   fflush(stdout);
  // }
  // #endif /*DEBUG*/
}

void audio_player_set_volume(AudioPlayer_HandleTypeDef player, float volume_dB) {
  if (player == NULL) {
    return;
  }
  player->volume = volume_dB;
  player->scaleFactor = powf(10.0f, volume_dB / 20.0f);

  #ifdef DEBUG_PRINT
  printf("Volume set to %+.3f dB (%.3f %%)\r\n", volume_dB, (player->scaleFactor * 100.0f));
  #endif
}

void audio_player_set_mute(AudioPlayer_HandleTypeDef player, uint8_t mute) {
  if (player == NULL) {
    return;
  }
  // TODO: mute not used yet
  player->mute = mute;
}

