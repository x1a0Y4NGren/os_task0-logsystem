#include <stdlib.h>

#include "buffer.h"

int buffer_init(LogBuffer *buffer, int capacity)
{
    if (buffer == 0 || capacity <= 0) {
        return -1;
    }

    buffer->entries = (LogEntry *)malloc(sizeof(LogEntry) * capacity);
    if (buffer->entries == 0) {
        return -1;
    }

    buffer->capacity = capacity;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;

    pthread_mutex_init(&buffer->mutex, 0);
    pthread_cond_init(&buffer->not_empty, 0);
    pthread_cond_init(&buffer->not_full, 0);

    return 0;
}

void buffer_destroy(LogBuffer *buffer)
{
    if (buffer == 0) {
        return;
    }

    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);

    free(buffer->entries);
    buffer->entries = 0;
    buffer->capacity = 0;
}

void buffer_push(LogBuffer *buffer, const LogEntry *entry)
{
    if (buffer == 0 || entry == 0) {
        return;
    }

    pthread_mutex_lock(&buffer->mutex);

    while (buffer->count == buffer->capacity) {
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    }

    buffer->entries[buffer->tail] = *entry;
    buffer->tail = (buffer->tail + 1) % buffer->capacity;
    buffer->count++;

    pthread_cond_signal(&buffer->not_empty);
    pthread_mutex_unlock(&buffer->mutex);
}

void buffer_pop(LogBuffer *buffer, LogEntry *entry)
{
    if (buffer == 0 || entry == 0) {
        return;
    }

    pthread_mutex_lock(&buffer->mutex);

    while (buffer->count == 0) {
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }

    *entry = buffer->entries[buffer->head];
    buffer->head = (buffer->head + 1) % buffer->capacity;
    buffer->count--;

    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);
}
