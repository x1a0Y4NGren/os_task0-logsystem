#ifndef OUTPUT_H
#define OUTPUT_H

#include "common.h"

void output_init(void);
void output_write_analysis(const LogEntry *entry);
void output_write_alert(const LogEntry *entry);
void output_close(void);

#endif
