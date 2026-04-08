#include <stdio.h>

#include "log_ai.h"

void log_ai_init(void)
{
    printf("Log analysis module initialized.\n");
}

void log_ai_analyze(LogEntry *entry)
{
    if (entry == 0) {
        return;
    }

    /* Placeholder result: real classification logic will be added later. */
    snprintf(entry->category, sizeof(entry->category), "UNCLASSIFIED");
    entry->score = 0;
    entry->needs_alert = 0;
}
