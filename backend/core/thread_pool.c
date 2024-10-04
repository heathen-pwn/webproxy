#include "thread_pool.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

void *worker_thread(void *pool) {
    thread_pool_t *tpool = (thread_pool_t *)pool;

    while (1) {
        pthread_mutex_lock(&(tpool->lock));

        // Wait for tasks to be available
        while (tpool->task_queue->count == 0 && !tpool->shutdown) {
            pthread_cond_wait(&(tpool->notify), &(tpool->lock));
        }

        if (tpool->shutdown) {
            pthread_mutex_unlock(&(tpool->lock));
            pthread_exit(NULL);
        }

        // Get the task from the queue
        task_t *task = tpool->task_queue->head;
        tpool->task_queue->head = task->next;
        tpool->task_queue->count--;

        pthread_mutex_unlock(&(tpool->lock));

        // Execute the task
        (*(task->function))(task->arg);
        free(task);
    }

    return NULL;
}
// Assign function a worker thread
int thread_pool_add(thread_pool_t *tpool, void (*function)(void *), void *arg) {
    task_t *new_task = (task_t *)malloc(sizeof(task_t));
    if (!new_task) return -1;

    new_task->function = function;
    new_task->arg = arg;
    new_task->next = NULL;

    pthread_mutex_lock(&(tpool->lock));

    if (tpool->task_queue->count == 0) {
        tpool->task_queue->head = new_task;
        tpool->task_queue->tail = new_task;
    } else {
        tpool->task_queue->tail->next = new_task;
        tpool->task_queue->tail = new_task;
    }
    tpool->task_queue->count++;

    // Signal one worker thread
    pthread_cond_signal(&(tpool->notify));
    pthread_mutex_unlock(&(tpool->lock));

    return 0;
}

int thread_pool_destroy(thread_pool_t *tpool) {
    pthread_mutex_lock(&(tpool->lock));
    tpool->shutdown = 1;
    pthread_cond_broadcast(&(tpool->notify));  // Wake up all threads

    pthread_mutex_unlock(&(tpool->lock));

    for (int i = 0; i < tpool->thread_count; i++) {
        pthread_join(tpool->threads[i], NULL);  // Wait for threads to finish
    }

    // Clean up
    free(tpool->threads);
    free(tpool->task_queue);
    return 0;
}

thread_pool_t *thread_pool_init(int num_threads) {
    // Allocate memory for the thread pool
    thread_pool_t *tpool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    if (!tpool) {
        fprintf(stderr, "Failed to allocate thread pool\n");
        return NULL;
    }

    // Allocate memory for the thread array
    tpool->threads = (pthread_t *)malloc(sizeof(pthread_t) * num_threads);
    if (!tpool->threads) {
        fprintf(stderr, "Failed to allocate thread array\n");
        free(tpool);
        return NULL;
    }

    // Allocate memory for the task queue
    tpool->task_queue = (task_queue_t *)malloc(sizeof(task_queue_t));
    if (!tpool->task_queue) {
        fprintf(stderr, "Failed to allocate task queue\n");
        free(tpool->threads);
        free(tpool);
        return NULL;
    }

    // Initialize the task queue
    tpool->task_queue->head = NULL;
    tpool->task_queue->tail = NULL;
    tpool->task_queue->count = 0;

    // Initialize synchronization primitives
    pthread_mutex_init(&(tpool->lock), NULL);
    pthread_cond_init(&(tpool->notify), NULL);

    // Set the number of threads and shutdown flag
    tpool->thread_count = num_threads;
    tpool->shutdown = 0;

    // Create worker threads
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&(tpool->threads[i]), NULL, worker_thread, (void *)tpool) != 0) {
            fprintf(stderr, "Error creating thread %d\n", i);
            thread_pool_destroy(tpool);
            return NULL;
        }
    }

    return tpool;
}
