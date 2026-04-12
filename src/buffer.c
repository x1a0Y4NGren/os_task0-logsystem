#include <stdlib.h>
#include <string.h>

#include "buffer.h"


int buffer_init(LogBuffer *buffer, int capacity)
{
    if (buffer == NULL || capacity <= 0) {
        return -1;
    }

    buffer->entries = (LogEntry *)calloc((size_t)capacity, sizeof(LogEntry));
    if (buffer->entries == NULL) {
        return -1;
    }

    buffer->capacity = capacity;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;

    pthread_mutex_init(&buffer->mutex, NULL);
    pthread_cond_init(&buffer->not_empty, NULL);
    pthread_cond_init(&buffer->not_full, NULL);

    return 0;
}

void buffer_destroy(LogBuffer *buffer)
{
    if (buffer == NULL) {
        return;
    }

    if (buffer->entries != NULL) {
        free(buffer->entries);
        buffer->entries = NULL;
    }

    pthread_mutex_destroy(&buffer->mutex);
    pthread_cond_destroy(&buffer->not_empty);
    pthread_cond_destroy(&buffer->not_full);

    buffer->capacity = 0;
    buffer->head = 0;
    buffer->tail = 0;
    buffer->count = 0;
}

void buffer_push(LogBuffer *buffer, const LogEntry *entry)
{
    if (buffer == NULL || entry == NULL) {
        return;
    }

    pthread_mutex_lock(&buffer->mutex);

    while (buffer->count >= buffer->capacity) {
        pthread_cond_wait(&buffer->not_full, &buffer->mutex);
    }

    memcpy(&buffer->entries[buffer->tail], entry, sizeof(LogEntry));
    
    buffer->tail = (buffer->tail + 1) % buffer->capacity;
    buffer->count++;

    pthread_cond_signal(&buffer->not_empty);

    pthread_mutex_unlock(&buffer->mutex);
}

void buffer_pop(LogBuffer *buffer, LogEntry *entry)
{
    if (buffer == NULL || entry == NULL) {
        return;
    }

    pthread_mutex_lock(&buffer->mutex);

    while (buffer->count <= 0) {
        pthread_cond_wait(&buffer->not_empty, &buffer->mutex);
    }

    memcpy(entry, &buffer->entries[buffer->head], sizeof(LogEntry));
    
    buffer->head = (buffer->head + 1) % buffer->capacity;
    buffer->count--;

    pthread_cond_signal(&buffer->not_full);

    pthread_mutex_unlock(&buffer->mutex);
}
