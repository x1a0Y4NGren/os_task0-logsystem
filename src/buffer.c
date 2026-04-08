#include "buffer.h"

void buffer_init(LogBuffer *buffer)
{
    if (buffer == 0) {
        return;
    }

    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;

    pthread_mutex_init(&buffer->mutex, 0);
    pthread_cond_init(&buffer->not_empty, 0);
    pthread_cond_init(&buffer->not_full, 0);
}

void buffer_destroy(LogBuffer *buffer)
{
    if (buffer == 0) {
        return;
    }

    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);
}

void buffer_push(LogBuffer *buffer, const LogEntry *entry)
{
    if (buffer == 0 || entry == 0) {
        return;
    }

    pthread_mutex_lock(&buffer->mutex);

    while (buffer->count == LOG_BUFFER_CAPACITY) {
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    }

    buffer->entries[buffer->tail] = *entry;
    buffer->tail = (buffer->tail + 1) % LOG_BUFFER_CAPACITY;
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
    buffer->head = (buffer->head + 1) % LOG_BUFFER_CAPACITY;
    buffer->count--;

    pthread_cond_signal(&buffer->not_full);
    pthread_mutex_unlock(&buffer->mutex);
}
