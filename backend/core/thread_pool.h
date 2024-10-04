#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <pthread.h>
#include <stdlib.h>

typedef struct task_t {
    void (*function)(void *); // Function pointer for the task
    void *arg;                // Arguments for the function
    struct task_t *next;       // Next task in the queue
} task_t;

typedef struct task_queue_t {
    task_t *head;              // Head of the task queue
    task_t *tail;              // Tail of the task queue
    int count;                 // Number of tasks in the queue
} task_queue_t;

typedef struct thread_pool_t {
    pthread_t *threads;    // Array of worker threads
    task_queue_t *task_queue; // Queue of tasks for workers
    pthread_mutex_t lock;  // Mutex for synchronizing access to task queue
    pthread_cond_t notify; // Condition variable to notify workers
    int thread_count;      // Number of threads
    int shutdown;          // Flag to indicate if the pool is shutting down
} thread_pool_t;



// Function declarations go here

void *worker_thread(void *pool);
int thread_pool_add(thread_pool_t *tpool, void (*function)(void *), void *arg);
int thread_pool_destroy(thread_pool_t *tpool);
thread_pool_t *thread_pool_init(int num_threads);

#endif /* THREAD_POOL_H */
