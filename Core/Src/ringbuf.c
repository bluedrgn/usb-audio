/*
 * ringbuf.c
 *
 *  Created on: Apr 10, 2026
 *      Author: bluedrgn
 */

#include "ringbuf.h"
#include "main.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct ringbuf_t {
uint8_t *buf;
volatile size_t size;
volatile size_t head;
volatile size_t tail;
};

ringbuf_status_t ringbuf_init(ringbuf_Handle *rb_ptr, size_t size) {
  if (!rb_ptr) return RINGBUF_PARAM_ERR;
  if (!size) return RINGBUF_SIZE_ERR;
  ringbuf_Handle rb = malloc(sizeof(struct ringbuf_t));
  if (!rb) return RINGBUF_MEM_ERR;
  *rb_ptr = rb;
  rb->buf = malloc(size);
  if (!rb->buf) return RINGBUF_MEM_ERR;
  rb->size = size;
  rb->head = 0;
  rb->tail = 0;
  return RINGBUF_OK;
}

ringbuf_status_t ringbuf_deinit(ringbuf_Handle *rb_ptr) {
  if (!rb_ptr) return RINGBUF_PARAM_ERR;
  if (!(*rb_ptr)->buf) RINGBUF_MEM_ERR;
  free((*rb_ptr)->buf);
  free(*rb_ptr);
  *rb_ptr = NULL;
  return RINGBUF_OK;
}

ringbuf_status_t ringbuf_enqueue(ringbuf_Handle rb, void *data, size_t data_size) {
  if (!rb || !data) return RINGBUF_PARAM_ERR;
  if (!data_size) return RINGBUF_OK;
  size_t space;
  ringbuf_status_t ret;
  ret = ringbuf_space(rb, &space);
  if (ret != RINGBUF_OK) return ret;
  if (data_size > space) return RINGBUF_SIZE_ERR;
  if (rb->head + data_size >= rb->size) {
    size_t chunk = rb->size - rb->head;
    memcpy(&rb->buf[rb->head], data, chunk);
    rb->head = 0;
    data_size -= chunk;
    data += chunk;
  }
  memcpy(&rb->buf[rb->head], data, data_size);
  rb->head = (rb->head + data_size) % rb->size;
  return RINGBUF_OK;
}

ringbuf_status_t ringbuf_dequeue(ringbuf_Handle rb, void *data, size_t read_size) {
  if (!rb || !data) return RINGBUF_PARAM_ERR;
  if (!read_size) return RINGBUF_OK;
  size_t avaliable;
  ringbuf_status_t ret;
  ret = ringbuf_avaliable(rb, &avaliable);
  if (ret != RINGBUF_OK) return ret;
  if (read_size > avaliable) return RINGBUF_SIZE_ERR;
  if (rb->tail + read_size >= rb->size) {
    size_t chunk = rb->size - rb->tail;
    memcpy(data, &rb->buf[rb->tail], chunk);
    rb->tail = 0;
    read_size -= chunk;
    data += chunk;
  }
  memcpy(data, &rb->buf[rb->tail], read_size);
  rb->tail = (rb->tail + read_size) % rb->size;
  return RINGBUF_OK;
}

ringbuf_status_t ringbuf_avaliable(ringbuf_Handle rb, size_t *size) {
  if (!rb) return RINGBUF_PARAM_ERR;
  if (rb->head >= rb->tail)
    *size = rb->head - rb->tail;
  else
    *size = rb->size - (rb->tail - rb->head);
  return RINGBUF_OK;
}

ringbuf_status_t ringbuf_space(ringbuf_Handle rb, size_t *size) {
  if (!rb) return RINGBUF_PARAM_ERR;
  if (rb->head < rb->tail)
    *size = rb->tail - rb->head - 1U;
  else
    *size = rb->size - (rb->head - rb->tail) - 1U;
  return RINGBUF_OK;
}

ringbuf_status_t ringbuf_reset(ringbuf_Handle rb) {
  if (!rb) return RINGBUF_PARAM_ERR;
  rb->head = 0;
  rb->tail = 0;
  return RINGBUF_OK;
}
