#include "lab.h"
#include <pthread.h>
#include <stdio.h>

struct queue {
    void **buffer;
    int capacity;
    int size;
    int front;
    int rear;
    bool shutdown;
    pthread_mutex_t mutex;
    pthread_cond_t not_full;
    pthread_cond_t not_empty;
};

static bool print_shutdown_message = false;

queue_t queue_init(int capacity) {
    queue_t q = (queue_t)malloc(sizeof(struct queue));
    q->buffer = (void **)malloc(capacity * sizeof(void *));
    q->capacity = capacity;
    q->size = 0;
    q->front = 0;
    q->rear = -1;
    q->shutdown = false;
    pthread_mutex_init(&q->mutex, NULL);
    pthread_cond_init(&q->not_full, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    return q;
}

void queue_destroy(queue_t q) {
    pthread_mutex_lock(&q->mutex);
    q->shutdown = true;
    pthread_cond_broadcast(&q->not_full);
    pthread_cond_broadcast(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);

    free(q->buffer);
    pthread_mutex_destroy(&q->mutex);
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
    free(q);
}

void enqueue(queue_t q, void *data) {
    pthread_mutex_lock(&q->mutex);
    while (q->size == q->capacity && !q->shutdown) {
        pthread_cond_wait(&q->not_full, &q->mutex);
    }
    if (q->shutdown) {
        pthread_mutex_unlock(&q->mutex);
        return;
    }
    q->rear = (q->rear + 1) % q->capacity;
    q->buffer[q->rear] = data;
    q->size++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->mutex);
}

void *dequeue(queue_t q) {
    pthread_mutex_lock(&q->mutex);
    while (q->size == 0 && !q->shutdown) {
        pthread_cond_wait(&q->not_empty, &q->mutex);
    }
    if (q->shutdown && q->size == 0) {
        pthread_mutex_unlock(&q->mutex);
        return NULL;
    }
    void *data = q->buffer[q->front];
    q->front = (q->front + 1) % q->capacity;
    q->size--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->mutex);
    return data;
}

void queue_shutdown(queue_t q) {
    pthread_mutex_lock(&q->mutex);
    if (!q->shutdown) {
        q->shutdown = true;
        pthread_cond_broadcast(&q->not_full);
        pthread_cond_broadcast(&q->not_empty);
        if (print_shutdown_message) {
            printf("Shutdown requested!\n");
        }
    }
    pthread_mutex_unlock(&q->mutex);
}

bool is_empty(queue_t q) {
    pthread_mutex_lock(&q->mutex);
    bool empty = q->size == 0;
    pthread_mutex_unlock(&q->mutex);
    return empty;
}

bool is_shutdown(queue_t q) {
    pthread_mutex_lock(&q->mutex);
    bool shutdown = q->shutdown;
    pthread_mutex_unlock(&q->mutex);
    return shutdown;
}

void set_print_shutdown_message(bool value) {
    print_shutdown_message = value;
}
