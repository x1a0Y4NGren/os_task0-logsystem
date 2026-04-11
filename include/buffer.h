#ifndef BUFFER_H
#define BUFFER_H

#include <pthread.h>

#include "common.h"

/* A simple circular buffer for the producer-consumer model. */
typedef struct {
    LogEntry *entries;
    int capacity;
    int head;
    int tail;
    int count;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} LogBuffer;

int buffer_init(LogBuffer *buffer, int capacity);
void buffer_destroy(LogBuffer *buffer);
void buffer_push(LogBuffer *buffer, const LogEntry *entry);
void buffer_pop(LogBuffer *buffer, LogEntry *entry);

#endif
