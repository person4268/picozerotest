#pragma once
#include "can.h"

#define FIFO_SIZE 4

typedef struct {
    struct can_msg buffer[FIFO_SIZE];
    int head;
    int tail;
    int count;
} fifo_t;

void fifo_init(fifo_t *fifo);
bool fifo_is_full(fifo_t *fifo);
bool fifo_is_empty(fifo_t *fifo);
bool fifo_enqueue(fifo_t *fifo, struct can_msg data);
bool fifo_dequeue(fifo_t *fifo, struct can_msg* data_out);
int fifo_size(fifo_t *fifo);