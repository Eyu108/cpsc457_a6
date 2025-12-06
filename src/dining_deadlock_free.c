#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "../include/semaphore.h"

// Global variables
int num_philosophers;           // Number of philosophers
int num_meals;                  // Number of meals each philosopher should eat
semaphore_t *chopsticks;        // Array of semaphores (one per chopstick)
pthread_mutex_t print_mutex;    // Mutex for synchronized printing
int current_meal = 1;           // Current meal number being displayed
int *meals_eaten;               // Array to track meals eaten by each philosopher

/**
 * Helper function to create random delay
 * @param max_time_us Maximum time to sleep in microseconds
 */
void nap(int max_time_us) {
    usleep(rand() % max_time_us);
}

/**
 * Philosopher thread function - Odd-Even Solution
 * 
 * ODD philosophers (1, 3, 5, ...):  Pick LEFT chopstick first, then RIGHT
 * EVEN philosophers (0, 2, 4, ...): Pick RIGHT chopstick first, then LEFT
 * 
 * This asymmetry breaks the circular wait condition and prevents deadlock
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
    
    // Eat the specified number of meals
    for (int meal = 0; meal < num_meals; meal++) {
        // Think (delay before trying to eat)
        nap(100);
        
        // === ENTRY SECTION (Acquire chopsticks) ===
        
        if (id % 2 == 0) {
            // EVEN philosopher: Pick RIGHT first, then LEFT
            
            pthread_mutex_lock(&print_mutex);
            printf("Phil %d acquired chopstick %d\n", id, right_chopstick);
            fflush(stdout);
            pthread_mutex_unlock(&print_mutex);
            semaphore_wait(&chopsticks[right_chopstick]);
            
            nap(50); // Small delay
            
            pthread_mutex_lock(&print_mutex);
            printf("Phil %d acquired chopstick %d\n", id, left_chopstick);
            fflush(stdout);
            pthread_mutex_unlock(&print_mutex);
            semaphore_wait(&chopsticks[left_chopstick]);
            
        } else {
            // ODD philosopher: Pick LEFT first, then RIGHT
            
            pthread_mutex_lock(&print_mutex);
            printf("Phil %d acquired chopstick %d\n", id, left_chopstick);
            fflush(stdout);
            pthread_mutex_unlock(&print_mutex);
            semaphore_wait(&chopsticks[left_chopstick]);
            
            nap(50); // Small delay
            
            pthread_mutex_lock(&print_mutex);
            printf("Phil %d acquired chopstick %d\n", id, right_chopstick);
            fflush(stdout);
            pthread_mutex_unlock(&print_mutex);
            semaphore_wait(&chopsticks[right_chopstick]);
        }
        
        // === CRITICAL SECTION (Eating) ===
        pthread_mutex_lock(&print_mutex);
        printf("Phil %d eating\n", id);
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
        
        nap(100); // Eat
        
        pthread_mutex_lock(&print_mutex);
        printf("Phil %d finished eating\n", id);
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
        
        // Update meals eaten counter
        meals_eaten[id]++;
        
        // === EXIT SECTION (Release chopsticks) ===
        
        // Release both chopsticks
        semaphore_signal(&chopsticks[left_chopstick]);
        semaphore_signal(&chopsticks[right_chopstick]);
        
        pthread_mutex_lock(&print_mutex);
        printf("Phil %d released chopsticks %d and %d\n", 
               id, left_chopstick, right_chopstick);
        fflush(stdout);
        pthread_mutex_unlock(&print_mutex);
    }
    
    return NULL;
}

/**
 * Main function
 * Creates philosophers and chopsticks, runs deadlock-free simulation
 */
int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_philosophers>\n", argv[0]);
        return 1;
    }
    
    // Parse number of philosophers
    num_philosophers = atoi(argv[1]);
    if (num_philosophers < 5) {
        fprintf(stderr, "Error: Number of philosophers must be at least 5\n");
        return 1;
    }
    
    // Set number of meals (100 as per assignment requirement)
    num_meals = 100;
    
    printf("Starting Dining Philosophers (Deadlock Free - Odd/Even) with %d philosophers\n", 
           num_philosophers);
    printf("Each philosopher will eat %d meals\n\n", num_meals);
    fflush(stdout);
    
    // Initialize print mutex for synchronized output
    if (pthread_mutex_init(&print_mutex, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize print mutex\n");
        return 1;
    }
    
    // Allocate array of chopstick semaphores
    chopsticks = (semaphore_t *)malloc(num_philosophers * sizeof(semaphore_t));
    if (chopsticks == NULL) {
        fprintf(stderr, "Error: Failed to allocate chopsticks array\n");
        return 1;
    }
    
    // Allocate array to track meals eaten
    meals_eaten = (int *)calloc(num_philosophers, sizeof(int));
    if (meals_eaten == NULL) {
        fprintf(stderr, "Error: Failed to allocate meals_eaten array\n");
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
    
    // Record start time
    struct timespec start_time, end_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    
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
    
    // Wait for all philosophers to finish eating
    for (int i = 0; i < num_philosophers; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Record end time
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    
    // Calculate execution time
    double execution_time = (end_time.tv_sec - start_time.tv_sec) + 
                           (end_time.tv_nsec - start_time.tv_nsec) / 1000000000.0;
    
    // Print summary
    printf("\n=== Simulation Complete ===\n");
    printf("Total philosophers: %d\n", num_philosophers);
    printf("Meals per philosopher: %d\n", num_meals);
    printf("Total meals served: %d\n", num_philosophers * num_meals);
    printf("Execution time: %.2f seconds\n", execution_time);
    printf("No deadlock occurred!\n");
    
    // Verify all meals were eaten
    int total_meals = 0;
    for (int i = 0; i < num_philosophers; i++) {
        total_meals += meals_eaten[i];
    }
    printf("Meals counted: %d\n", total_meals);
    
    // Cleanup
    for (int i = 0; i < num_philosophers; i++) {
        semaphore_destroy(&chopsticks[i]);
    }
    pthread_mutex_destroy(&print_mutex);
    free(chopsticks);
    free(threads);
    free(meals_eaten);
    
    return 0;
}