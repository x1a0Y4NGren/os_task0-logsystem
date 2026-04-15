#include <stdio.h>

#include "log_ai.h"

/* 初始化日志分析模块。
 * 当前版本只输出一条提示，说明模块已经准备好。 */
void log_ai_init(void)
{
    printf("Log analysis module initialized.\n");
}

/* 对日志进行分析。
 * 当前课程作业版本采用简化实现：
 * 统一写入基础分类、评分和告警标记，
 * 先把生产者-消费者主流程和文件输出流程跑通。 */
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
