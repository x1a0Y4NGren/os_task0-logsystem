#ifndef COMMON_H
#define COMMON_H

#define LOG_SOURCE_SIZE 32
#define LOG_MESSAGE_SIZE 256
#define LOG_CATEGORY_SIZE 32
#define LOG_BUFFER_CAPACITY 16

/* Basic log structure used across the whole project. */
typedef struct {
    int id;
    char source[LOG_SOURCE_SIZE];
    char message[LOG_MESSAGE_SIZE];
    char category[LOG_CATEGORY_SIZE];
    int score;
    int needs_alert;
} LogEntry;

#endif
