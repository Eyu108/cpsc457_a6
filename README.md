# CPSC 457 Assignment 6: POSIX Threads, Semaphores, and Deadlocks

## Overview
This assignment implements solutions to classic concurrency problems using custom semaphores built with POSIX threads, mutexes, and condition variables.

## Project Structure
```
cpsc457_a6/
├── include/
│   ├── semaphore.h          # Custom semaphore header
│   └── buffer.h             # Circular buffer header
├── src/
│   ├── semaphore.c          # Custom semaphore implementation
│   ├── dining_deadlock.c    # Dining philosophers (deadlock-prone)
│   ├── dining_deadlock_free.c  # Dining philosophers (deadlock-free)
│   └── producer_consumer.c  # Producer-consumer problem
├── obj/                     # Object files (generated)
├── Makefile                 # Build configuration
└── README.md               # This file
```

## Components

### 1. Custom Semaphore
- Monitor-based implementation using `pthread_mutex` and `pthread_cond`
- FIFO queue for waiting threads
- Thread-safe `wait()` and `signal()` operations

### 2. Dining Philosophers (Deadlock-Prone)
- 5 philosophers
- Right-then-left chopstick pickup order
- Demonstrates circular wait and deadlock conditions

### 3. Dining Philosophers (Deadlock-Free)
- 100 philosophers, 100 meals each
- Odd-even strategy: odd philosophers pick left first, even pick right first
- Breaks circular wait condition, preventing deadlock

### 4. Producer-Consumer Problem
- Circular buffer with semaphore synchronization
- Two throughput experiments:
  - Experiment 1: 10 producers, 10-300 consumers
  - Experiment 2: 10-300 producers, 10 consumers
- Generates CSV files with throughput data

## Compilation

Compile all programs:
```bash
make
```

Clean build artifacts:
```bash
make clean
```

## Usage

### Dining Philosophers (Deadlock-Prone)
```bash
./dining_deadlock <number_of_philosophers>
```
Example:
```bash
./dining_deadlock 5
```

### Dining Philosophers (Deadlock-Free)
```bash
./dining_deadlock_free <number_of_philosophers>
```
Example:
```bash
./dining_deadlock_free 100
```

### Producer-Consumer Experiments
```bash
./producer_consumer fixed_producers    # Experiment 1
./producer_consumer fixed_consumers    # Experiment 2
```

## Output Files
- `output_exp1.csv` - Throughput data for Experiment 1 (fixed producers)
- `output_exp2.csv` - Throughput data for Experiment 2 (fixed consumers)


## Key Features
- ✅ Custom semaphore with FIFO queue
- ✅ Monitor pattern using mutex + condition variables
- ✅ Race-condition free synchronization
- ✅ Deadlock prevention through odd-even strategy
- ✅ Performance measurement and CSV output
- ✅ Clean, well-commented code


## Author
Student: Eli
Course: CPSC 457 - Fall 2025
Assignment: 6
Due: December 5, 2025