#include <stdio.h>
#include <string.h>
#include <time.h>
#include "output.h"
static FILE *analysis_file = 0;
static FILE *alert_file = 0;
/* ── 补充：运行计数器，用于 output_close() 末尾汇总统计 ── */
static long total_entries = 0;
static long total_alerts  = 0;
static long level_count[4] = {0, 0, 0, 0};   /* 下标 = needs_alert 级别 */
/* ── 补充：按类别统计 ── */
#define CAT_COUNT 7
static const char *cat_names[CAT_COUNT] = {
    "CRITICAL", "ERROR", "SECURITY", "WARNING", "INFO", "DEBUG", "UNCLASSIFIED"
};
static long cat_count[CAT_COUNT];
/* ── 补充：获取当前时间字符串，格式 "YYYY-MM-DD HH:MM:SS" ── */
static void current_timestamp(char *buf, size_t buf_size)
{
    time_t     now     = time(0);
    struct tm *tm_info = localtime(&now);
    if (tm_info != 0)
        strftime(buf, buf_size, "%Y-%m-%d %H:%M:%S", tm_info);
    else
        snprintf(buf, buf_size, "0000-00-00 00:00:00");
}
/* ── 补充：将 needs_alert 数值映射为可读标签 ── */
static const char *alert_label(int level)
{
    switch (level) {
        case 1:  return "LEVEL-1 [ELEVATED]";
        case 2:  return "LEVEL-2 [HIGH]";
        case 3:  return "LEVEL-3 [CRITICAL]";
        default: return "UNKNOWN";
    }
}
/* ── 补充：在 cat_names[] 中查找类别下标，未找到返回末位 ── */
static int find_cat(const char *category)
{
    int i;
    for (i = 0; i < CAT_COUNT - 1; i++)
        if (strcmp(category, cat_names[i]) == 0) return i;
    return CAT_COUNT - 1;
}
/* ================================================================== */
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
    /* ── 补充：重置所有计数器 ── */
    total_entries = 0;
    total_alerts  = 0;
    memset(level_count, 0, sizeof(level_count));
    memset(cat_count,   0, sizeof(cat_count));
}
/* ================================================================== */
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
    /* ── 补充：更新条目计数和类别统计 ── */
    total_entries++;
    cat_count[find_cat(entry->category)]++;
}
/* ================================================================== */
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
    /* ── 补充：在原始单行记录之后追加结构化详情 ── */
    {
        char ts[20];
        current_timestamp(ts, sizeof(ts));
        fprintf(alert_file,
                "  [%s]  %s\n"
                "  Category : %s\n"
                "  Score    : %d/100\n"
                "  ----------------------------------------\n",
                ts,
                alert_label(entry->needs_alert),
                entry->category,
                entry->score);
        fflush(alert_file);
    }
    /* ── 补充：更新告警计数 ── */
    total_alerts++;
    if (entry->needs_alert >= 0 && entry->needs_alert <= 3)
        level_count[entry->needs_alert]++;
}
/* ================================================================== */
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
    /* ── 补充：将汇总统计打印到终端，供实验报告记录 ── */
    {
        int i;
        char ts[20];
        current_timestamp(ts, sizeof(ts));
        printf("[output] Session closed at %s\n", ts);
        printf("[output] Total entries processed : %ld\n", total_entries);
        printf("[output] Total alerts raised     : %ld\n", total_alerts);
        printf("[output]   LEVEL-1 [ELEVATED]   : %ld\n", level_count[1]);
        printf("[output]   LEVEL-2 [HIGH]       : %ld\n", level_count[2]);
        printf("[output]   LEVEL-3 [CRITICAL]   : %ld\n", level_count[3]);
        printf("[output] Category breakdown:\n");
        for (i = 0; i < CAT_COUNT; i++)
            if (cat_count[i] > 0)
                printf("[output]   %-14s : %ld\n", cat_names[i], cat_count[i]);
    }
}