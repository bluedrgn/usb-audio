#include "main.h"
#include "audio_player.h"
#include "resample.h"
#include "stm32f4xx_hal_i2s.h"
#include <stddef.h>
#ifdef DEBUG
#include <stdio.h>
#endif /*DEBUG*/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
// #include <arm_math.h>


void __play(AudioPlayer_HandleTypeDef *player);
void __stop(AudioPlayer_HandleTypeDef *player);
void __update_VU(AudioPlayer_HandleTypeDef *player, AudioSample_q15_t *buffer, size_t buffer_size);

void audio_player_init(AudioPlayer_HandleTypeDef *player, I2S_HandleTypeDef* i2sDev, AudioSample_q15_t *buffer, size_t buffer_size) {
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
    player->playback = APLAYER_START;
  }
}

void audio_player_stop(AudioPlayer_HandleTypeDef *player) {
  if (player->playback == APLAYER_PLAY)
  {
    player->playback = APLAYER_STOP;
  }
}

void audio_player_enque_samples(AudioPlayer_HandleTypeDef *player, AudioSample_q15_t *buffer, size_t buffer_size) {
  AudioSample_q15_t tmpbuff[buffer_size+4];

  if (player == NULL || buffer == NULL || buffer_size == 0)
    Error_Handler();

  memcpy(&tmpbuff[0], player->tmpbuf, 4 * sizeof(AudioSample_q15_t));
  memcpy(&tmpbuff[4], buffer, buffer_size * sizeof(AudioSample_q15_t));
  memcpy(player->tmpbuf, &buffer[buffer_size-4], 4 * sizeof(AudioSample_q15_t));

  #ifdef DEBUG
  // printf("  Enqueue %p %u\r\n", buff, size);
  #endif /*DEBUG*/

  if ((player->playback == APLAYER_START) && (player->wr_ptr >= (player->buffer_size * 2 / 3)))
  {
    __play(player);
  }

  for (size_t rd = 1; rd <= buffer_size; rd++) {
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

    player->buffer[player->wr_ptr] = AudioSample_cubic_interpolate(&tmpbuff[rd], player->stretch.sample_counter);

    player->wr_ptr++;
    player->wr_ptr %= player->buffer_size;
  }

  // __update_VU(player, buffer, buffer_size);
}

void __play(AudioPlayer_HandleTypeDef *player) {
  player->i2sDev->Init.AudioFreq = player->current_sample_rate;
  HAL_I2S_Init(player->i2sDev);
  HAL_I2S_Transmit_DMA(player->i2sDev, (uint16_t*)player->buffer, player->buffer_size*2);
  player->playback = APLAYER_PLAY;
}

void __stop(AudioPlayer_HandleTypeDef *player) {
  HAL_I2S_DMAStop(player->i2sDev);
  player->playback = APLAYER_RESET;
}


/* !Call this function from the corresponding HAL_I2S_TxCpltCallback! */
void audio_player_sync(AudioPlayer_HandleTypeDef *player) {
  /* read pointer should be 0 at this point */
  if (player == NULL) {
    return;
  }

  if (player->playback == APLAYER_STOP) {
    __stop(player);
    #ifdef DEBUG
    // HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
    #endif
    return;
  }

  player->offset = player->wr_ptr - player->sync_goal;
  #ifdef DEBUG
//   HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  printf("%08lX Sync %+5d ", HAL_GetTick(), player->offset);
  #endif /*DEBUG*/

  if (player->offset <= 0) {
    player->stretch.amount = (player->offset-(player->sync_unit-1))/player->sync_unit;
  }
  else {
    player->stretch.amount = (player->offset+(player->sync_unit-1))/player->sync_unit;
  }

  #ifdef DEBUG
  printf(" %+5d\r", player->stretch.amount);
  if (player->playback == APLAYER_RESET) {
    putchar('\n');
  } else {
    fflush(stdout);
  }
  #endif /*DEBUG*/
}