#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include <pthread.h>

/**
 * Node structure for FIFO queue of waiting threads
 * Each node stores a thread ID and pointer to next node
 */
typedef struct queue_node {
    pthread_t thread_id;           // Thread ID of waiting thread
    struct queue_node *next;       // Pointer to next node in queue
} queue_node_t;

/**
 * Custom counting semaphore structure
 * Uses monitor pattern with mutex and condition variable
 * Maintains FIFO queue for waiting threads
 */
typedef struct {
    int count;                     // Semaphore counter value
    pthread_mutex_t mutex;         // Mutex for mutual exclusion (monitor lock)
    pthread_cond_t cond;           // Condition variable for blocking/unblocking threads
    queue_node_t *queue_head;      // Head of FIFO waiting queue
    queue_node_t *queue_tail;      // Tail of FIFO waiting queue
} semaphore_t;

/**
 * Initialize a semaphore with given initial count
 * @param sem Pointer to semaphore structure
 * @param initial_count Initial value for semaphore counter
 * @return 0 on success, -1 on failure
 */
int semaphore_init(semaphore_t *sem, int initial_count);

/**
 * Destroy a semaphore and free resources
 * @param sem Pointer to semaphore structure
 * @return 0 on success, -1 on failure
 */
int semaphore_destroy(semaphore_t *sem);

/**
 * Wait operation on semaphore (P operation / down)
 * Decrements counter if > 0, otherwise blocks thread in FIFO queue
 * @param sem Pointer to semaphore structure
 */
void semaphore_wait(semaphore_t *sem);

/**
 * Signal operation on semaphore (V operation / up)
 * Wakes up first waiting thread in queue, or increments counter if no waiting threads
 * @param sem Pointer to semaphore structure
 */
void semaphore_signal(semaphore_t *sem);

#endif // SEMAPHORE_H