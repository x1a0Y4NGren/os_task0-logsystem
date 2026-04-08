#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffer.h"
#include "log_ai.h"
#include "output.h"

#define PRODUCER_COUNT 2

typedef struct {
    int thread_id;
    const char *role;
} ThreadArg;

static void *producer_worker(void *arg)
{
    ThreadArg *thread_arg = (ThreadArg *)arg;

    printf("[%s %d] placeholder thread started.\n",
           thread_arg->role,
           thread_arg->thread_id);
    printf("[%s %d] real log generation will be added later.\n",
           thread_arg->role,
           thread_arg->thread_id);

    return 0;
}

static void *consumer_worker(void *arg)
{
    ThreadArg *thread_arg = (ThreadArg *)arg;
    LogEntry sample_entry = {0};

    sample_entry.id = 1;
    snprintf(sample_entry.source, sizeof(sample_entry.source), "demo");
    snprintf(sample_entry.message,
             sizeof(sample_entry.message),
             "placeholder log message");

    log_ai_analyze(&sample_entry);
    output_write_analysis(&sample_entry);

    if (sample_entry.needs_alert) {
        output_write_alert(&sample_entry);
    }

    printf("[%s %d] placeholder thread started.\n",
           thread_arg->role,
           thread_arg->thread_id);
    printf("[%s %d] real log consumption will be added later.\n",
           thread_arg->role,
           thread_arg->thread_id);

    return 0;
}

int main(void)
{
    LogBuffer buffer;
    pthread_t producers[PRODUCER_COUNT];
    pthread_t consumer;
    ThreadArg producer_args[PRODUCER_COUNT];
    ThreadArg consumer_arg = {1, "consumer"};
    int i;

    printf("========================================\n");
    printf("Smart Log Processing System (Skeleton)\n");
    printf("OS course assignment: producer-consumer demo\n");
    printf("========================================\n");

    buffer_init(&buffer);
    log_ai_init();
    output_init();

    printf("System modules initialized.\n");
    printf("Creating 2 producer threads and 1 consumer thread.\n");

    for (i = 0; i < PRODUCER_COUNT; ++i) {
        producer_args[i].thread_id = i + 1;
        producer_args[i].role = "producer";

        if (pthread_create(&producers[i], 0, producer_worker, &producer_args[i]) != 0) {
            fprintf(stderr, "Failed to create producer thread %d.\n", i + 1);
            output_close();
            buffer_destroy(&buffer);
            return EXIT_FAILURE;
        }
    }

    if (pthread_create(&consumer, 0, consumer_worker, &consumer_arg) != 0) {
        fprintf(stderr, "Failed to create consumer thread.\n");

        for (i = 0; i < PRODUCER_COUNT; ++i) {
            pthread_join(producers[i], 0);
        }

        output_close();
        buffer_destroy(&buffer);
        return EXIT_FAILURE;
    }

    for (i = 0; i < PRODUCER_COUNT; ++i) {
        pthread_join(producers[i], 0);
    }
    pthread_join(consumer, 0);

    printf("Skeleton run finished.\n");
    printf("You can extend the real producer-consumer logic next.\n");

    output_close();
    buffer_destroy(&buffer);

    return 0;
}
