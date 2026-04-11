#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "buffer.h"
#include "log_ai.h"
#include "output.h"

#define DEFAULT_PRODUCER_COUNT 2
#define DEFAULT_CONSUMER_COUNT 1
#define DEFAULT_TOTAL_LOGS 20
#define STOP_LOG_ID (-1)

typedef struct {
    LogBuffer *buffer;
    int total_logs;
    int producer_count;
    int consumer_count;
    int next_log_id;
    int produced_count;
    int consumed_count;
    pthread_mutex_t state_mutex;
    pthread_mutex_t output_mutex;
} SharedState;

typedef struct {
    int thread_id;
    const char *role;
    SharedState *shared;
} ThreadArg;

static int parse_int_arg(const char *text, int min_value, int *result)
{
    char *end_ptr = 0;
    long value;

    if (text == 0 || result == 0) {
        return -1;
    }

    value = strtol(text, &end_ptr, 10);
    if (*text == '\0' || *end_ptr != '\0') {
        return -1;
    }

    if (value < min_value || value > INT_MAX) {
        return -1;
    }

    *result = (int)value;
    return 0;
}

static void print_usage(const char *program_name)
{
    printf("Usage: %s [buffer_capacity] [producer_count] [consumer_count] [total_logs]\n",
           program_name);
    printf("Example: %s 16 2 1 50\n", program_name);
}

static void make_demo_log(LogEntry *entry, int log_id, int producer_id)
{
    static const char *messages[] = {
        "User login request received",
        "Disk usage warning detected",
        "Database response time increased",
        "API request finished successfully",
        "Service error code reported"
    };
    int message_index;

    if (entry == 0) {
        return;
    }

    message_index = (log_id - 1) % (int)(sizeof(messages) / sizeof(messages[0]));

    entry->id = log_id;
    snprintf(entry->source, sizeof(entry->source), "producer-%d", producer_id);
    snprintf(entry->message,
             sizeof(entry->message),
             "%s (log-%d)",
             messages[message_index],
             log_id);
    snprintf(entry->category, sizeof(entry->category), "PENDING");
    entry->score = 0;
    entry->needs_alert = 0;
}

static void make_stop_log(LogEntry *entry)
{
    if (entry == 0) {
        return;
    }

    entry->id = STOP_LOG_ID;
    snprintf(entry->source, sizeof(entry->source), "system");
    snprintf(entry->message, sizeof(entry->message), "STOP");
    snprintf(entry->category, sizeof(entry->category), "CONTROL");
    entry->score = 0;
    entry->needs_alert = 0;
}

static void *producer_worker(void *arg)
{
    ThreadArg *thread_arg = (ThreadArg *)arg;
    SharedState *shared = thread_arg->shared;
    int log_id;
    LogEntry entry;

    printf("[producer %d] started.\n", thread_arg->thread_id);

    while (1) {
        pthread_mutex_lock(&shared->state_mutex);

        if (shared->next_log_id > shared->total_logs) {
            pthread_mutex_unlock(&shared->state_mutex);
            break;
        }

        log_id = shared->next_log_id;
        shared->next_log_id++;
        shared->produced_count++;

        pthread_mutex_unlock(&shared->state_mutex);

        make_demo_log(&entry, log_id, thread_arg->thread_id);
        buffer_push(shared->buffer, &entry);
    }

    printf("[producer %d] finished.\n", thread_arg->thread_id);

    return 0;
}

static void *consumer_worker(void *arg)
{
    ThreadArg *thread_arg = (ThreadArg *)arg;
    SharedState *shared = thread_arg->shared;
    LogEntry entry;

    printf("[consumer %d] started.\n", thread_arg->thread_id);

    while (1) {
        buffer_pop(shared->buffer, &entry);

        if (entry.id == STOP_LOG_ID) {
            break;
        }

        log_ai_analyze(&entry);

        pthread_mutex_lock(&shared->output_mutex);
        output_write_analysis(&entry);
        if (entry.needs_alert) {
            output_write_alert(&entry);
        }
        pthread_mutex_unlock(&shared->output_mutex);

        pthread_mutex_lock(&shared->state_mutex);
        shared->consumed_count++;
        pthread_mutex_unlock(&shared->state_mutex);
    }

    printf("[consumer %d] finished.\n", thread_arg->thread_id);

    return 0;
}

int main(int argc, char *argv[])
{
    LogBuffer buffer;
    SharedState shared;
    pthread_t *producer_threads = 0;
    pthread_t *consumer_threads = 0;
    ThreadArg *producer_args = 0;
    ThreadArg *consumer_args = 0;
    LogEntry stop_entry;
    int buffer_capacity = LOG_BUFFER_CAPACITY;
    int producer_count = DEFAULT_PRODUCER_COUNT;
    int consumer_count = DEFAULT_CONSUMER_COUNT;
    int total_logs = DEFAULT_TOTAL_LOGS;
    int created_producers = 0;
    int created_consumers = 0;
    int i;

    if (argc > 5) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc >= 2 && parse_int_arg(argv[1], 1, &buffer_capacity) != 0) {
        fprintf(stderr, "Invalid buffer_capacity.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc >= 3 && parse_int_arg(argv[2], 1, &producer_count) != 0) {
        fprintf(stderr, "Invalid producer_count.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc >= 4 && parse_int_arg(argv[3], 1, &consumer_count) != 0) {
        fprintf(stderr, "Invalid consumer_count.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (argc >= 5 && parse_int_arg(argv[4], 0, &total_logs) != 0) {
        fprintf(stderr, "Invalid total_logs.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    printf("========================================\n");
    printf("Smart Log Processing System\n");
    printf("OS course assignment: producer-consumer initial version\n");
    printf("========================================\n");
    printf("buffer_capacity = %d\n", buffer_capacity);
    printf("producer_count  = %d\n", producer_count);
    printf("consumer_count  = %d\n", consumer_count);
    printf("total_logs      = %d\n", total_logs);

    if (buffer_init(&buffer, buffer_capacity) != 0) {
        fprintf(stderr, "Failed to initialize log buffer.\n");
        return EXIT_FAILURE;
    }

    shared.buffer = &buffer;
    shared.total_logs = total_logs;
    shared.producer_count = producer_count;
    shared.consumer_count = consumer_count;
    shared.next_log_id = 1;
    shared.produced_count = 0;
    shared.consumed_count = 0;

    pthread_mutex_init(&shared.state_mutex, 0);
    pthread_mutex_init(&shared.output_mutex, 0);

    log_ai_init();
    output_init();

    producer_threads = (pthread_t *)calloc((size_t)producer_count, sizeof(pthread_t));
    consumer_threads = (pthread_t *)calloc((size_t)consumer_count, sizeof(pthread_t));
    producer_args = (ThreadArg *)calloc((size_t)producer_count, sizeof(ThreadArg));
    consumer_args = (ThreadArg *)calloc((size_t)consumer_count, sizeof(ThreadArg));

    if (producer_threads == 0 || consumer_threads == 0 ||
        producer_args == 0 || consumer_args == 0) {
        fprintf(stderr, "Failed to allocate thread resources.\n");
        free(producer_threads);
        free(consumer_threads);
        free(producer_args);
        free(consumer_args);
        output_close();
        pthread_mutex_destroy(&shared.state_mutex);
        pthread_mutex_destroy(&shared.output_mutex);
        buffer_destroy(&buffer);
        return EXIT_FAILURE;
    }

    printf("System modules initialized.\n");
    printf("Creating consumer threads.\n");

    for (i = 0; i < consumer_count; ++i) {
        consumer_args[i].thread_id = i + 1;
        consumer_args[i].role = "consumer";
        consumer_args[i].shared = &shared;

        if (pthread_create(&consumer_threads[i], 0, consumer_worker, &consumer_args[i]) != 0) {
            fprintf(stderr, "Failed to create consumer thread %d.\n", i + 1);
            goto cleanup;
        }

        created_consumers++;
    }

    printf("Creating producer threads.\n");

    for (i = 0; i < producer_count; ++i) {
        producer_args[i].thread_id = i + 1;
        producer_args[i].role = "producer";
        producer_args[i].shared = &shared;

        if (pthread_create(&producer_threads[i], 0, producer_worker, &producer_args[i]) != 0) {
            fprintf(stderr, "Failed to create producer thread %d.\n", i + 1);
            goto cleanup;
        }

        created_producers++;
    }

    for (i = 0; i < created_producers; ++i) {
        pthread_join(producer_threads[i], 0);
    }

    make_stop_log(&stop_entry);
    for (i = 0; i < created_consumers; ++i) {
        buffer_push(&buffer, &stop_entry);
    }

    for (i = 0; i < created_consumers; ++i) {
        pthread_join(consumer_threads[i], 0);
    }

    printf("All threads finished.\n");
    printf("Produced logs: %d\n", shared.produced_count);
    printf("Consumed logs: %d\n", shared.consumed_count);

cleanup:
    if (created_producers != producer_count || created_consumers != consumer_count) {
        make_stop_log(&stop_entry);

        for (i = 0; i < created_producers; ++i) {
            pthread_join(producer_threads[i], 0);
        }

        for (i = 0; i < created_consumers; ++i) {
            buffer_push(&buffer, &stop_entry);
        }

        for (i = 0; i < created_consumers; ++i) {
            pthread_join(consumer_threads[i], 0);
        }
    }

    free(producer_threads);
    free(consumer_threads);
    free(producer_args);
    free(consumer_args);
    output_close();
    pthread_mutex_destroy(&shared.state_mutex);
    pthread_mutex_destroy(&shared.output_mutex);
    buffer_destroy(&buffer);

    if (created_producers != producer_count || created_consumers != consumer_count) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
