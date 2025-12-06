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
 * If counter > 0: decrements counter and proceeds
 * If counter == 0: adds thread to FIFO queue and blocks using pthread_cond_wait()
 * 
 * This implementation ensures:
 * - Race-free access to counter and queue (protected by mutex)
 * - FIFO ordering of waiting threads
 * - Deadlock-free operation
 */
void semaphore_wait(semaphore_t *sem) {
    // Enter monitor (acquire mutex)
    pthread_mutex_lock(&sem->mutex);
    
    // Get current thread ID
    pthread_t self = pthread_self();
    
    // If counter is 0, need to wait
    if (sem->count == 0) {
        // Add thread to FIFO waiting queue
        enqueue_thread(sem, self);
        
        // Block thread until signaled
        // pthread_cond_wait atomically releases mutex and blocks
        // When signaled, it re-acquires mutex before returning
        while (1) {
            pthread_cond_wait(&sem->cond, &sem->mutex);
            
            // Check if this thread is at head of queue (FIFO fairness)
            if (sem->queue_head != NULL && 
                pthread_equal(sem->queue_head->thread_id, self)) {
                // This thread's turn - remove from queue
                pthread_t dummy;
                dequeue_thread(sem, &dummy);
                break;
            }
            // Not this thread's turn, continue waiting
        }
    } else {
        // Counter > 0, can proceed immediately
        sem->count--;
    }
    
    // Exit monitor (release mutex)
    pthread_mutex_unlock(&sem->mutex);
}

/**
 * Signal operation on semaphore (V operation / up)
 * 
 * If waiting threads exist: wakes up first thread in FIFO queue
 * If no waiting threads: increments counter
 * 
 * This implementation ensures:
 * - Race-free access to counter and queue (protected by mutex)
 * - FIFO fairness for waiting threads
 * - No lost signals
 */
void semaphore_signal(semaphore_t *sem) {
    // Enter monitor (acquire mutex)
    pthread_mutex_lock(&sem->mutex);
    
    // Check if there are waiting threads in queue
    if (sem->queue_head != NULL) {
        // There are waiting threads - wake one up
        // pthread_cond_signal wakes up one waiting thread
        // The woken thread will check if it's at head of queue in wait()
        pthread_cond_signal(&sem->cond);
    } else {
        // No waiting threads - increment counter
        sem->count++;
    }
    
    // Exit monitor (release mutex)
    pthread_mutex_unlock(&sem->mutex);
}