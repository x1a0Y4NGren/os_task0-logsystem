#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "buffer.h"
#include "log_ai.h"
#include "output.h"

/* 程序运行时使用的默认参数。 */
#define DEFAULT_PRODUCER_COUNT 2
#define DEFAULT_CONSUMER_COUNT 1
#define DEFAULT_TOTAL_LOGS 20

/* 特殊日志编号：消费者读到这条记录时，说明可以退出了。 */
#define STOP_LOG_ID (-1)

/* 共享状态：由主线程、生产者线程、消费者线程共同访问。 */
typedef struct {
    LogBuffer *buffer;             /* 指向共享缓冲区 */
    int total_logs;                /* 计划总共生成多少条日志 */
    int producer_count;            /* 生产者线程数量 */
    int consumer_count;            /* 消费者线程数量 */
    int next_log_id;               /* 下一条要分配的日志编号 */
    int produced_count;            /* 已生产日志数 */
    int consumed_count;            /* 已消费日志数 */
    pthread_mutex_t state_mutex;   /* 保护共享计数和编号分配 */
    pthread_mutex_t output_mutex;  /* 保护输出文件写入 */
} SharedState;

/* 线程参数：每个线程都知道自己的编号、角色和共享状态。 */
typedef struct {
    int thread_id;
    const char *role;
    SharedState *shared;
} ThreadArg;

/* 解析命令行参数，并检查是否为合法整数。 */
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

/* 生成示例日志，供生产者线程写入缓冲区。 */
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

/* 生成停止标记，通知消费者线程结束。 */
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

/* 生产者线程：
 * 循环申请一个新的日志编号，生成日志，然后放入共享缓冲区。 */
static void *producer_worker(void *arg)
{
    ThreadArg *thread_arg = (ThreadArg *)arg;
    SharedState *shared = thread_arg->shared;
    int log_id;
    LogEntry entry;

    printf("[producer %d] started.\n", thread_arg->thread_id);

    while (1) {
        pthread_mutex_lock(&shared->state_mutex);

        /* 如果日志总数已经达到上限，当前生产者结束。 */
        if (shared->next_log_id > shared->total_logs) {
            pthread_mutex_unlock(&shared->state_mutex);
            break;
        }

        /* 为当前线程分配一个唯一日志编号，并更新统计信息。 */
        log_id = shared->next_log_id;
        shared->next_log_id++;
        shared->produced_count++;

        pthread_mutex_unlock(&shared->state_mutex);

        /* 生成示例日志并写入缓冲区。 */
        make_demo_log(&entry, log_id, thread_arg->thread_id);
        buffer_push(shared->buffer, &entry);
    }

    printf("[producer %d] finished.\n", thread_arg->thread_id);

    return 0;
}

/* 消费者线程：
 * 从缓冲区取日志，完成分析，再把结果写入输出文件。 */
static void *consumer_worker(void *arg)
{
    ThreadArg *thread_arg = (ThreadArg *)arg;
    SharedState *shared = thread_arg->shared;
    LogEntry entry;

    printf("[consumer %d] started.\n", thread_arg->thread_id);

    while (1) {
        buffer_pop(shared->buffer, &entry);

        /* 读到停止标记后，说明主线程要求消费者退出。 */
        if (entry.id == STOP_LOG_ID) {
            break;
        }

        /* 先分析日志，再写入结果文件。 */
        log_ai_analyze(&entry);

        /* 输出文件是共享资源，这里加锁避免多线程同时写入。 */
        pthread_mutex_lock(&shared->output_mutex);
        output_write_analysis(&entry);
        if (entry.needs_alert) {
            output_write_alert(&entry);
        }
        pthread_mutex_unlock(&shared->output_mutex);

        /* 更新消费统计。 */
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

    /* 让终端输出按行刷新，便于多线程运行时观察。 */
    setvbuf(stdout, 0, _IOLBF, 0);

    /* 解析命令行参数，允许用户调整缓冲区和线程数量。 */
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
    fflush(stdout);

    /* 初始化共享缓冲区。 */
    if (buffer_init(&buffer, buffer_capacity) != 0) {
        fprintf(stderr, "Failed to initialize log buffer.\n");
        return EXIT_FAILURE;
    }

    /* 初始化共享状态，供所有线程共同使用。 */
    shared.buffer = &buffer;
    shared.total_logs = total_logs;
    shared.producer_count = producer_count;
    shared.consumer_count = consumer_count;
    shared.next_log_id = 1;
    shared.produced_count = 0;
    shared.consumed_count = 0;

    pthread_mutex_init(&shared.state_mutex, 0);
    pthread_mutex_init(&shared.output_mutex, 0);

    /* 初始化日志分析模块和输出模块。 */
    log_ai_init();
    output_init();
    fflush(stdout);

    /* 为线程句柄和线程参数分配空间。 */
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
    fflush(stdout);

    /* 先创建消费者，让它们可以先进入等待状态。 */
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
    fflush(stdout);

    /* 再创建生产者，由生产者负责向缓冲区写日志。 */
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

    /* 主线程等待所有生产者结束。 */
    for (i = 0; i < created_producers; ++i) {
        pthread_join(producer_threads[i], 0);
    }

    /* 所有生产者结束后，写入停止标记，通知消费者退出。 */
    make_stop_log(&stop_entry);
    for (i = 0; i < created_consumers; ++i) {
        buffer_push(&buffer, &stop_entry);
    }

    /* 等待所有消费者结束。 */
    for (i = 0; i < created_consumers; ++i) {
        pthread_join(consumer_threads[i], 0);
    }

    printf("All threads finished.\n");
    printf("Produced logs: %d\n", shared.produced_count);
    printf("Consumed logs: %d\n", shared.consumed_count);
    fflush(stdout);

cleanup:
    /* 如果中途创建线程失败，仍然尽量完成收尾。 */
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

    /* 释放动态资源，并销毁互斥锁和缓冲区。 */
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
