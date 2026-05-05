/*
 * ringbuf.h
 *
 *  Created on: May 4, 2026
 *      Author: bluedrgn
 */

#pragma once

#include <stdint.h>
#include <stddef.h>

typedef struct ringbuf_t* ringbuf_Handle;

typedef enum {
RINGBUF_OK,
RINGBUF_MEM_ERR,
RINGBUF_PARAM_ERR,
RINGBUF_SIZE_ERR,
} ringbuf_status_t;


ringbuf_status_t ringbuf_init(ringbuf_Handle *ringbuf, size_t size);
ringbuf_status_t ringbuf_deinit(ringbuf_Handle *ringbuf);
ringbuf_status_t ringbuf_enqueue(ringbuf_Handle ringbuf, void *data, size_t size);
ringbuf_status_t ringbuf_dequeue(ringbuf_Handle ringbuf, void *data, size_t size);
ringbuf_status_t ringbuf_avaliable(ringbuf_Handle ringbuf, size_t *size);
ringbuf_status_t ringbuf_space(ringbuf_Handle ringbuf, size_t *size);
ringbuf_status_t ringbuf_reset(ringbuf_Handle ringbuf);
