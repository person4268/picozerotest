#include <stdbool.h>
#include "fifo.h"
// yeah this is chatgpt but meh, it's just a simple fifo impl i didnt wanna bother with


// Initialize FIFO
void fifo_init(fifo_t *fifo) {
    fifo->head = 0;
    fifo->tail = 0;
    fifo->count = 0;
}

// Check if FIFO is full
bool fifo_is_full(fifo_t *fifo) {
    return fifo->count == FIFO_SIZE;
}

// Check if FIFO is empty
bool fifo_is_empty(fifo_t *fifo) {
    return fifo->count == 0;
}

// Add element to FIFO (from ISR)
bool fifo_enqueue(fifo_t *fifo, struct can_msg data) {
    if (fifo_is_full(fifo)) {
        return false;  // FIFO is full, cannot enqueue
    }

    fifo->buffer[fifo->tail] = data;
    fifo->tail = (fifo->tail + 1) % FIFO_SIZE;
    fifo->count++;
    return true;
}

// Remove element from FIFO (from main loop or ISR)
bool fifo_dequeue(fifo_t *fifo, struct can_msg* data) {
    if (fifo_is_empty(fifo)) {
        return false;  // FIFO is empty, cannot dequeue
    }

    *data = fifo->buffer[fifo->head];
    fifo->head = (fifo->head + 1) % FIFO_SIZE;
    fifo->count--;
    return true;
}

int fifo_size(fifo_t *fifo) {
    return fifo->count;
}