#include "arm_math_types.h"
#include "dsp/support_functions.h"
#include "main.h"
#include "audio_player.h"
#include "resample.h"
#include "stm32f4xx_hal_i2s.h"
#include "arm_math.h"
#include <math.h>
#include <stddef.h>
#ifdef DEBUG
#include <stdio.h>
#endif /*DEBUG*/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static const size_t chs = 2;  /* Stereo! */

void __play(AudioPlayer_HandleTypeDef *player);
void __stop(AudioPlayer_HandleTypeDef *player);

void audio_player_init(AudioPlayer_HandleTypeDef *player, I2S_HandleTypeDef* i2sDev, AUDIOSAMPLE_TYPE *buffer, size_t buffer_size) {
  player->i2sDev = i2sDev;
  player->buffer = buffer;
  player->buffer_size = buffer_size;
  player->sync_goal = buffer_size * 2 / 3;
  player->playback = APLAYER_RESET;
}

void audio_player_start(AudioPlayer_HandleTypeDef *player, uint32_t sample_rate) {
  if (player->playback == APLAYER_RESET)
  {
    memset(player->tmpbuf, 0, sizeof(player->tmpbuf));
    player->wr_ptr = 0;
    player->playback = APLAYER_READY;
  }

  if (player->playback == APLAYER_READY)
  {
    player->current_sample_rate = sample_rate;
    player->sync_unit = sample_rate / 1000;
    player->stretch.amount = 0;
    player->stretch.sample_counter = 0;
    arm_fill_f32(0.0f, (float*)player->tmpbuf, 4);
    player->playback = APLAYER_START;
  }
}

void audio_player_stop(AudioPlayer_HandleTypeDef *player) {
  if (player->playback == APLAYER_PLAY)
  {
    player->playback = APLAYER_STOP;
  }
}

void audio_player_enque_samples(AudioPlayer_HandleTypeDef *player, AudioSample_f32_t *buffer, size_t buffer_size) {
  AudioSample_f32_t tmp_in[buffer_size+4];

  #ifdef DEBUG
  // printf("  Enqueue %p %u\r\n", buff, size);
  if (player == NULL || buffer == NULL || buffer_size == 0)
    Error_Handler();
  #endif /*DEBUG*/

  /* caching the last 4 samples from the end of every chunk */
  arm_copy_f32((float*)player->tmpbuf, (float*)&tmp_in[0], 4 * chs);
  arm_scale_f32((float*)buffer, player->scaleFactor, (float*)&tmp_in[4], buffer_size * chs);
  // arm_copy_f32((float*)buffer, (float*)&tmp_in[4].L, buffer_size * chs);
  arm_copy_f32((float*)&tmp_in[buffer_size], (float*)player->tmpbuf, 4 * chs);


  if ((player->playback == APLAYER_START) && (player->wr_ptr >= (player->buffer_size * 2 / 3)))
  {
    __play(player);
  }

  for (size_t rd = 1; rd <= buffer_size; rd++) {
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
    if (sizeof(AUDIOSAMPLE_TYPE) == 4) {
      arm_float_to_q15((float*)&sample_in, (q15_t*)&player->buffer[player->wr_ptr], chs);
    }
    else {
      AUDIOSAMPLE_TYPE *sample_out = &player->buffer[player->wr_ptr];
      arm_float_to_q31((float*)&sample_in, (q31_t*)sample_out, chs);
      /* https://community.st.com/t5/stm32-mcus-products/stm32f7-i2s-dma-byte-order-issue/td-p/382535 */
      /* https://stackoverflow.com/questions/48541818/swap-halfwords-efficently */
      sample_out->L = (sample_out->L << 16) | (sample_out->L >> 16);
      sample_out->R = (sample_out->R << 16) | (sample_out->R >> 16);
    }
    player->wr_ptr++;
    player->wr_ptr %= player->buffer_size;
  }
}

void __play(AudioPlayer_HandleTypeDef *player) {
  player->i2sDev->Init.AudioFreq = player->current_sample_rate;
  HAL_I2S_Init(player->i2sDev);
  HAL_I2S_Transmit_DMA(player->i2sDev, (uint16_t*)player->buffer, player->buffer_size * chs);
  player->playback = APLAYER_PLAY;
}

void __stop(AudioPlayer_HandleTypeDef *player) {
  HAL_I2S_DMAStop(player->i2sDev);
  player->playback = APLAYER_RESET;
}


/* !Call this function from the corresponding HAL_I2S_TxCpltCallback! */
void audio_player_sync(AudioPlayer_HandleTypeDef *player) {
  int16_t offset;
  /* read pointer should be 0 at this point */
  if (player == NULL) {
    return;
  }

  if (player->playback == APLAYER_STOP) {
    __stop(player);
    // #ifdef DEBUG
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

  // #ifdef DEBUG
  // printf("%08lX Sync %+5d %+5d\r", HAL_GetTick(), offset, player->stretch.amount);
  // if (player->playback == APLAYER_RESET) {
  //   putchar('\n');
  // } else {
  //   fflush(stdout);
  // }
  // #endif /*DEBUG*/
}

void audio_player_set_volume(AudioPlayer_HandleTypeDef *player, int16_t volume) {
  if (player == NULL) {
    return;
  }
  player->volume = volume;
  if (volume == INT16_MIN)
    player->scaleFactor = 0.0f;
  else
    player->scaleFactor = powf(10.0f, (player->volume / 256.0f) / 20.0f);

  #ifdef DEBUG
  printf("Volume set to %+.3f dB (%.3f %%)\r\n", (player->volume / 256.0f), (player->scaleFactor * 100.0f));
  #endif
}

void audio_player_set_mute(AudioPlayer_HandleTypeDef *player, uint8_t mute) {
  if (player == NULL) {
    return;
  }
  // TODO: mute not used yet
  player->mute = mute;
}

