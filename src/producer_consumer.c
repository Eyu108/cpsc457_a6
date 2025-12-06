#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "../include/semaphore.h"
#include "../include/buffer.h"

// Global variables
circular_buffer_t buffer;
int items_per_producer = 100;      // Each producer creates 100 items
pthread_mutex_t item_counter_mutex;
int total_items_produced = 0;
int total_items_consumed = 0;
int expected_total_items = 0;

/**
 * Initialize circular buffer
 */
int buffer_init(circular_buffer_t *buffer, int size) {
    if (buffer == NULL || size <= 0) {
        return -1;
    }
    
    // Allocate data array
    buffer->data = (int *)malloc(size * sizeof(int));
    if (buffer->data == NULL) {
        return -1;
    }
    
    buffer->size = size;
    buffer->in = 0;
    buffer->out = 0;
    buffer->count = 0;
    
    // Initialize semaphores
    // empty counts available slots (initially all slots are empty)
    if (semaphore_init(&buffer->empty, size) != 0) {
        free(buffer->data);
        return -1;
    }
    
    // full counts occupied slots (initially no slots are full)
    if (semaphore_init(&buffer->full, 0) != 0) {
        semaphore_destroy(&buffer->empty);
        free(buffer->data);
        return -1;
    }
    
    // Initialize mutex for buffer access
    if (pthread_mutex_init(&buffer->mutex, NULL) != 0) {
        semaphore_destroy(&buffer->empty);
        semaphore_destroy(&buffer->full);
        free(buffer->data);
        return -1;
    }
    
    return 0;
}

/**
 * Destroy buffer and free resources
 */
int buffer_destroy(circular_buffer_t *buffer) {
    if (buffer == NULL) {
        return -1;
    }
    
    semaphore_destroy(&buffer->empty);
    semaphore_destroy(&buffer->full);
    pthread_mutex_destroy(&buffer->mutex);
    free(buffer->data);
    
    return 0;
}

/**
 * Produce an item into the buffer
 */
void buffer_produce(circular_buffer_t *buffer, int item) {
    // Wait for empty slot
    semaphore_wait(&buffer->empty);
    
    // Enter critical section
    pthread_mutex_lock(&buffer->mutex);
    
    // Add item to buffer
    buffer->data[buffer->in] = item;
    buffer->in = (buffer->in + 1) % buffer->size;
    buffer->count++;
    
    // Exit critical section
    pthread_mutex_unlock(&buffer->mutex);
    
    // Signal that buffer has one more full slot
    semaphore_signal(&buffer->full);
}

/**
 * Consume an item from the buffer
 */
int buffer_consume(circular_buffer_t *buffer) {
    // Wait for full slot
    semaphore_wait(&buffer->full);
    
    // Enter critical section
    pthread_mutex_lock(&buffer->mutex);
    
    // Remove item from buffer
    int item = buffer->data[buffer->out];
    buffer->out = (buffer->out + 1) % buffer->size;
    buffer->count--;
    
    // Exit critical section
    pthread_mutex_unlock(&buffer->mutex);
    
    // Signal that buffer has one more empty slot
    semaphore_signal(&buffer->empty);
    
    return item;
}

/**
 * Producer thread function
 * Produces unique items and puts them in the buffer
 */
void *producer(void *arg) {
    int producer_id = *(int *)arg;
    free(arg);
    
    // Each producer creates unique items
    int start_item = producer_id * items_per_producer;
    
    for (int i = 0; i < items_per_producer; i++) {
        int item = start_item + i;
        
        // Produce item
        buffer_produce(&buffer, item);
        
        // Update global counter
        pthread_mutex_lock(&item_counter_mutex);
        total_items_produced++;
        pthread_mutex_unlock(&item_counter_mutex);
    }
    
    return NULL;
}

/**
 * Consumer thread function
 * Consumes items from the buffer
 * Each consumer tries to consume a fair share of items
 */
void *consumer(void *arg) {
    free(arg); // Free the allocated ID (we don't need it)
    
    while (1) {
        // Atomically check and increment consumed counter
        pthread_mutex_lock(&item_counter_mutex);
        if (total_items_consumed >= expected_total_items) {
            // All items consumed, exit
            pthread_mutex_unlock(&item_counter_mutex);
            break;
        }
        // Reserve this item for consumption
        total_items_consumed++;
        pthread_mutex_unlock(&item_counter_mutex);
        
        // Now consume the reserved item
        buffer_consume(&buffer);
    }
    
    return NULL;
}

/**
 * Get current time in milliseconds
 */
long long get_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**
 * Run experiment with given number of producers and consumers
 * Returns throughput (time / items)
 */
double run_experiment(int num_producers, int num_consumers, int buffer_size) {
    // Reset counters
    total_items_produced = 0;
    total_items_consumed = 0;
    expected_total_items = num_producers * items_per_producer;
    
    // Initialize buffer
    if (buffer_init(&buffer, buffer_size) != 0) {
        fprintf(stderr, "Error: Failed to initialize buffer\n");
        return -1.0;
    }
    
    // Create producer and consumer threads
    pthread_t *producer_threads = (pthread_t *)malloc(num_producers * sizeof(pthread_t));
    pthread_t *consumer_threads = (pthread_t *)malloc(num_consumers * sizeof(pthread_t));
    
    if (producer_threads == NULL || consumer_threads == NULL) {
        fprintf(stderr, "Error: Failed to allocate thread arrays\n");
        return -1.0;
    }
    
    // Start timing
    long long start_time = get_time_ms();
    
    // Create producer threads
    for (int i = 0; i < num_producers; i++) {
        int *id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&producer_threads[i], NULL, producer, id);
    }
    
    // Create consumer threads
    for (int i = 0; i < num_consumers; i++) {
        int *id = (int *)malloc(sizeof(int));
        *id = i;
        pthread_create(&consumer_threads[i], NULL, consumer, id);
    }
    
    // Wait for all producers to finish
    for (int i = 0; i < num_producers; i++) {
        pthread_join(producer_threads[i], NULL);
    }
    
    // Wait for all consumers to finish
    for (int i = 0; i < num_consumers; i++) {
        pthread_join(consumer_threads[i], NULL);
    }
    
    // End timing
    long long end_time = get_time_ms();
    long long total_time_ms = end_time - start_time;
    
    // Calculate throughput (time per item in milliseconds)
    double throughput = (double)total_time_ms / (double)expected_total_items;
    
    // Cleanup
    buffer_destroy(&buffer);
    free(producer_threads);
    free(consumer_threads);
    
    return throughput;
}

/**
 * Experiment 1: Fixed producers (10), varying consumers (10-300)
 */
void experiment_1() {
    printf("\n=== Experiment 1: Fixed Producers (10), Varying Consumers ===\n");
    
    FILE *file = fopen("output_exp1.csv", "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open output_exp1.csv\n");
        return;
    }
    
    // Write CSV header
    fprintf(file, "producers,consumers,throughput\n");
    
    int num_producers = 10;
    int buffer_size = 100;
    
    // Vary consumers from 10 to 300 in increments of 10
    for (int num_consumers = 10; num_consumers <= 300; num_consumers += 10) {
        printf("Running: producers=%d, consumers=%d...\n", num_producers, num_consumers);
        fflush(stdout);
        
        double throughput = run_experiment(num_producers, num_consumers, buffer_size);
        
        if (throughput >= 0) {
            fprintf(file, "%d,%d,%f\n", num_producers, num_consumers, throughput);
            printf("  Throughput: %f ms/item\n", throughput);
        }
    }
    
    fclose(file);
    printf("Experiment 1 complete! Results saved to output_exp1.csv\n");
}

/**
 * Experiment 2: Varying producers (10-300), fixed consumers (10)
 */
void experiment_2() {
    printf("\n=== Experiment 2: Varying Producers, Fixed Consumers (10) ===\n");
    
    FILE *file = fopen("output_exp2.csv", "w");
    if (file == NULL) {
        fprintf(stderr, "Error: Failed to open output_exp2.csv\n");
        return;
    }
    
    // Write CSV header
    fprintf(file, "producers,consumers,throughput\n");
    
    int num_consumers = 10;
    int buffer_size = 100;
    
    // Vary producers from 10 to 300 in increments of 10
    for (int num_producers = 10; num_producers <= 300; num_producers += 10) {
        printf("Running: producers=%d, consumers=%d...\n", num_producers, num_consumers);
        fflush(stdout);
        
        double throughput = run_experiment(num_producers, num_consumers, buffer_size);
        
        if (throughput >= 0) {
            fprintf(file, "%d,%d,%f\n", num_producers, num_consumers, throughput);
            printf("  Throughput: %f ms/item\n", throughput);
        }
    }
    
    fclose(file);
    printf("Experiment 2 complete! Results saved to output_exp2.csv\n");
}

/**
 * Main function
 */
int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <fixed_producers|fixed_consumers>\n", argv[0]);
        return 1;
    }
    
    // Initialize item counter mutex
    if (pthread_mutex_init(&item_counter_mutex, NULL) != 0) {
        fprintf(stderr, "Error: Failed to initialize mutex\n");
        return 1;
    }
    
    // Run appropriate experiment
    if (strcmp(argv[1], "fixed_producers") == 0) {
        experiment_1();
    } else if (strcmp(argv[1], "fixed_consumers") == 0) {
        experiment_2();
    } else {
        fprintf(stderr, "Error: Invalid argument. Use 'fixed_producers' or 'fixed_consumers'\n");
        pthread_mutex_destroy(&item_counter_mutex);
        return 1;
    }
    
    // Cleanup
    pthread_mutex_destroy(&item_counter_mutex);
    
    printf("\nAll experiments complete!\n");
    return 0;
}