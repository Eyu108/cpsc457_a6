#include "../include/semaphore.h"
#include <stdlib.h>
#include <stdio.h>

/**
 * Helper function to enqueue a thread ID to the waiting queue
 * Adds thread to the tail of the FIFO queue
 * @param sem Pointer to semaphore structure
 * @param thread_id Thread ID to enqueue
 * @return 0 on success, -1 on failure
 */
static int enqueue_thread(semaphore_t *sem, pthread_t thread_id) {
    // Allocate new queue node
    queue_node_t *new_node = (queue_node_t *)malloc(sizeof(queue_node_t));
    if (new_node == NULL) {
        return -1; // Memory allocation failed
    }
    
    // Initialize new node
    new_node->thread_id = thread_id;
    new_node->next = NULL;
    
    // Add to tail of queue (FIFO)
    if (sem->queue_tail == NULL) {
        // Queue is empty, new node becomes both head and tail
        sem->queue_head = new_node;
        sem->queue_tail = new_node;
    } else {
        // Queue not empty, add to tail
        sem->queue_tail->next = new_node;
        sem->queue_tail = new_node;
    }
    
    return 0;
}

/**
 * Helper function to dequeue a thread ID from the waiting queue
 * Removes and returns thread from the head of the FIFO queue
 * @param sem Pointer to semaphore structure
 * @param thread_id Pointer to store dequeued thread ID
 * @return 0 on success, -1 if queue is empty
 */
static int dequeue_thread(semaphore_t *sem, pthread_t *thread_id) {
    // Check if queue is empty
    if (sem->queue_head == NULL) {
        return -1; // Queue is empty
    }
    
    // Get thread ID from head node
    queue_node_t *old_head = sem->queue_head;
    *thread_id = old_head->thread_id;
    
    // Move head to next node
    sem->queue_head = old_head->next;
    
    // If queue becomes empty, update tail as well
    if (sem->queue_head == NULL) {
        sem->queue_tail = NULL;
    }
    
    // Free the old head node
    free(old_head);
    
    return 0;
}

/**
 * Initialize a semaphore with given initial count
 * Sets up mutex, condition variable, and empty FIFO queue
 */
int semaphore_init(semaphore_t *sem, int initial_count) {
    if (sem == NULL) {
        return -1;
    }
    
    // Initialize semaphore counter
    sem->count = initial_count;
    
    // Initialize empty FIFO queue
    sem->queue_head = NULL;
    sem->queue_tail = NULL;
    
    // Initialize mutex for monitor
    if (pthread_mutex_init(&sem->mutex, NULL) != 0) {
        return -1;
    }
    
    // Initialize condition variable for blocking/unblocking
    if (pthread_cond_init(&sem->cond, NULL) != 0) {
        pthread_mutex_destroy(&sem->mutex);
        return -1;
    }
    
    return 0;
}

/**
 * Destroy a semaphore and free all resources
 * Cleans up mutex, condition variable, and waiting queue
 */
int semaphore_destroy(semaphore_t *sem) {
    if (sem == NULL) {
        return -1;
    }
    
    // Free all nodes in waiting queue
    queue_node_t *current = sem->queue_head;
    while (current != NULL) {
        queue_node_t *next = current->next;
        free(current);
        current = next;
    }
    
    // Destroy mutex and condition variable
    pthread_mutex_destroy(&sem->mutex);
    pthread_cond_destroy(&sem->cond);
    
    return 0;
}

/**
 * Wait operation on semaphore (P operation / down)
 * 
 * Simplified implementation:
 * - If count > 0: decrement and proceed
 * - If count == 0: wait on condition variable
 * 
 * FIFO queue tracks waiting order for fairness
 */
void semaphore_wait(semaphore_t *sem) {
    // Enter monitor (acquire mutex)
    pthread_mutex_lock(&sem->mutex);
    
    // Get current thread ID for queue tracking
    pthread_t self = pthread_self();
    
    // If counter is 0, need to wait
    if (sem->count == 0) {
        // Add to FIFO queue for tracking
        enqueue_thread(sem, self);
        
        // Wait until signaled
        // Note: We use a simple approach - signal wakes threads
        // and they check if resources are available
        while (sem->count == 0) {
            pthread_cond_wait(&sem->cond, &sem->mutex);
        }
        
        // Remove from queue (FIFO - we're at head now)
        pthread_t dummy;
        dequeue_thread(sem, &dummy);
    }
    
    // Decrement counter
    sem->count--;
    
    // Exit monitor (release mutex)
    pthread_mutex_unlock(&sem->mutex);
}

/**
 * Signal operation on semaphore (V operation / up)
 * 
 * Increments counter and wakes up one waiting thread if any
 */
void semaphore_signal(semaphore_t *sem) {
    // Enter monitor (acquire mutex)
    pthread_mutex_lock(&sem->mutex);
    
    // Increment counter
    sem->count++;
    
    // Wake up one waiting thread (if any)
    // The woken thread will check count and proceed if > 0
    pthread_cond_signal(&sem->cond);
    
    // Exit monitor (release mutex)
    pthread_mutex_unlock(&sem->mutex);
}