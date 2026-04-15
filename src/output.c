#include <stdio.h>

#include "output.h"

static FILE *analysis_file = 0;
static FILE *alert_file = 0;

/* 初始化输出模块：
 * 创建 analysis.csv 和 alerts.log，
 * 并写入表头或初始化信息。 */
void output_init(void)
{
    analysis_file = fopen("analysis.csv", "w");
    if (analysis_file != 0) {
        fprintf(analysis_file, "id,source,category,score,needs_alert,message\n");
        fflush(analysis_file);
    }

    alert_file = fopen("alerts.log", "w");
    if (alert_file != 0) {
        fprintf(alert_file, "Alert output initialized.\n");
        fflush(alert_file);
    }
}

/* 将每条日志的分析结果写入 analysis.csv。 */
void output_write_analysis(const LogEntry *entry)
{
    if (analysis_file == 0 || entry == 0) {
        return;
    }

    fprintf(analysis_file,
            "%d,%s,%s,%d,%d,%s\n",
            entry->id,
            entry->source,
            entry->category,
            entry->score,
            entry->needs_alert,
            entry->message);
    fflush(analysis_file);
}

/* 将告警信息写入 alerts.log。 */
void output_write_alert(const LogEntry *entry)
{
    if (alert_file == 0 || entry == 0) {
        return;
    }

    fprintf(alert_file,
            "ALERT: id=%d source=%s score=%d message=%s\n",
            entry->id,
            entry->source,
            entry->score,
            entry->message);
    fflush(alert_file);
}

/* 关闭输出文件，释放文件资源。 */
void output_close(void)
{
    if (analysis_file != 0) {
        fclose(analysis_file);
        analysis_file = 0;
    }

    if (alert_file != 0) {
        fclose(alert_file);
        alert_file = 0;
    }
}
