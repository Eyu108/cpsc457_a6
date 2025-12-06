#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "../include/semaphore.h"

// Global variables
int num_philosophers;           // Number of philosophers
semaphore_t *chopsticks;        // Array of semaphores (one per chopstick)

/**
 * Helper function to create random delay
 * @param max_time_us Maximum time to sleep in microseconds
 */
void nap(int max_time_us) {
    usleep(rand() % max_time_us);
}

/**
 * Philosopher thread function
 * Each philosopher tries to pick up RIGHT chopstick first, then LEFT
 * This ordering can lead to circular wait and deadlock
 * 
 * @param arg Philosopher ID (cast to int)
 */
void *philosopher(void *arg) {
    int id = *(int *)arg;
    free(arg); // Free the allocated ID
    
    // Calculate chopstick indices
    int right_chopstick = (id + 1) % num_philosophers;  // Right chopstick
    int left_chopstick = id;                             // Left chopstick
    
    // Seed random number generator for this thread
    unsigned int seed = time(NULL) + id;
    
    while (1) {
        // Think (delay before trying to eat)
        nap(1000);
        
        // === ENTRY SECTION (Acquire chopsticks) ===
        
        // Try to pick up RIGHT chopstick first
        printf("Philosopher %d wants RIGHT chopstick %d\n", id, right_chopstick);
        fflush(stdout);
        
        semaphore_wait(&chopsticks[right_chopstick]);
        printf("Philosopher %d picked RIGHT chopstick %d\n", id, right_chopstick);
        fflush(stdout);
        
        // Small delay between picking up chopsticks (increases chance of deadlock)
        nap(1000);
        
        // Try to pick up LEFT chopstick
        printf("Philosopher %d wants LEFT chopstick %d\n", id, left_chopstick);
        fflush(stdout);
        
        semaphore_wait(&chopsticks[left_chopstick]);
        printf("Philosopher %d picked LEFT chopstick %d\n", id, left_chopstick);
        fflush(stdout);
        
        // === CRITICAL SECTION (Eating) ===
        printf("Philosopher %d eating\n", id);
        fflush(stdout);
        nap(2000); // Eat
        
        printf("Philosopher %d finished eating\n", id);
        fflush(stdout);
        
        // === EXIT SECTION (Release chopsticks) ===
        
        // Put down LEFT chopstick
        semaphore_signal(&chopsticks[left_chopstick]);
        printf("Philosopher %d released LEFT chopstick %d\n", id, left_chopstick);
        fflush(stdout);
        
        // Put down RIGHT chopstick
        semaphore_signal(&chopsticks[right_chopstick]);
        printf("Philosopher %d released RIGHT chopstick %d\n", id, right_chopstick);
        fflush(stdout);
    }
    
    return NULL;
}

/**
 * Main function
 * Creates philosophers and chopsticks, runs until deadlock occurs
 */
int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_philosophers>\n", argv[0]);
        return 1;
    }
    
    // Parse number of philosophers
    num_philosophers = atoi(argv[1]);
    if (num_philosophers < 2) {
        fprintf(stderr, "Error: Number of philosophers must be at least 2\n");
        return 1;
    }
    
    printf("Starting Dining Philosophers (Deadlock Prone) with %d philosophers\n\n", 
           num_philosophers);
    fflush(stdout);
    
    // Allocate array of chopstick semaphores
    chopsticks = (semaphore_t *)malloc(num_philosophers * sizeof(semaphore_t));
    if (chopsticks == NULL) {
        fprintf(stderr, "Error: Failed to allocate chopsticks array\n");
        return 1;
    }
    
    // Initialize chopstick semaphores (each chopstick is initially available)
    for (int i = 0; i < num_philosophers; i++) {
        if (semaphore_init(&chopsticks[i], 1) != 0) {
            fprintf(stderr, "Error: Failed to initialize chopstick %d\n", i);
            return 1;
        }
    }
    
    // Create philosopher threads
    pthread_t *threads = (pthread_t *)malloc(num_philosophers * sizeof(pthread_t));
    if (threads == NULL) {
        fprintf(stderr, "Error: Failed to allocate threads array\n");
        return 1;
    }
    
    for (int i = 0; i < num_philosophers; i++) {
        int *id = (int *)malloc(sizeof(int));
        if (id == NULL) {
            fprintf(stderr, "Error: Failed to allocate philosopher ID\n");
            return 1;
        }
        *id = i;
        
        if (pthread_create(&threads[i], NULL, philosopher, id) != 0) {
            fprintf(stderr, "Error: Failed to create philosopher thread %d\n", i);
            free(id);
            return 1;
        }
    }
    
    // Wait for threads (will likely deadlock before this completes)
    for (int i = 0; i < num_philosophers; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Cleanup (likely never reached due to deadlock)
    for (int i = 0; i < num_philosophers; i++) {
        semaphore_destroy(&chopsticks[i]);
    }
    free(chopsticks);
    free(threads);
    
    return 0;
}