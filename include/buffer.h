#ifndef BUFFER_H
#define BUFFER_H

#include "../include/semaphore.h"
#include <pthread.h>

/**
 * Circular buffer structure for producer-consumer problem
 * Uses semaphores for synchronization and mutex for mutual exclusion
 */
typedef struct {
    int *data;                      // Array to store produced items
    int size;                       // Maximum capacity of buffer
    int in;                         // Index for next producer write
    int out;                        // Index for next consumer read
    int count;                      // Current number of items in buffer
    
    semaphore_t empty;              // Counts empty slots (initially = size)
    semaphore_t full;               // Counts full slots (initially = 0)
    pthread_mutex_t mutex;          // Mutex for mutual exclusion on buffer access
} circular_buffer_t;

/**
 * Initialize a circular buffer with given size
 * @param buffer Pointer to buffer structure
 * @param size Maximum capacity of buffer
 * @return 0 on success, -1 on failure
 */
int buffer_init(circular_buffer_t *buffer, int size);

/**
 * Destroy a buffer and free resources
 * @param buffer Pointer to buffer structure
 * @return 0 on success, -1 on failure
 */
int buffer_destroy(circular_buffer_t *buffer);

/**
 * Produce (insert) an item into the buffer
 * Blocks if buffer is full
 * @param buffer Pointer to buffer structure
 * @param item Item to produce
 */
void buffer_produce(circular_buffer_t *buffer, int item);

/**
 * Consume (remove) an item from the buffer
 * Blocks if buffer is empty
 * @param buffer Pointer to buffer structure
 * @return Consumed item
 */
int buffer_consume(circular_buffer_t *buffer);

#endif // BUFFER_H