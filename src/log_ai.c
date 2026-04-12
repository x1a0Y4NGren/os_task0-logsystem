#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "log_ai.h"

/* ------------------------------------------------------------------ */
/* 分类规则表                                                          */
/* ------------------------------------------------------------------ */

typedef struct {
    const char *keyword;    /* 小写关键词                              */
    const char *category;   /* 分类标签                                */
    int         base_score; /* 基础严重度分值                          */
} ClassifyRule;

static const ClassifyRule rules[] = {
    /* CRITICAL */
    { "panic",        "CRITICAL",  95 },
    { "fatal",        "CRITICAL",  95 },
    { "critical",     "CRITICAL",  90 },
    { "oops",         "CRITICAL",  88 },
    { "kernel bug",   "CRITICAL",  88 },
    /* ERROR */
    { "segfault",     "ERROR",     80 },
    { "segmentation", "ERROR",     80 },
    { "corrupt",      "ERROR",     82 },
    { "overflow",     "ERROR",     75 },
    { "underflow",    "ERROR",     72 },
    { "abort",        "ERROR",     78 },
    { "exception",    "ERROR",     72 },
    { "error",        "ERROR",     75 },
    { "failed",       "ERROR",     70 },
    { "failure",      "ERROR",     70 },
    { "denied",       "ERROR",     65 },
    { "refused",      "ERROR",     65 },
    { "timeout",      "ERROR",     68 },
    { "invalid",      "ERROR",     60 },
    /* SECURITY */
    { "intrusion",    "SECURITY",  92 },
    { "attack",       "SECURITY",  90 },
    { "exploit",      "SECURITY",  92 },
    { "injection",    "SECURITY",  88 },
    { "unauthorized", "SECURITY",  80 },
    { "forbidden",    "SECURITY",  78 },
    { "privilege",    "SECURITY",  75 },
    { "auth",         "SECURITY",  70 },
    /* WARNING */
    { "disk full",    "WARNING",   58 },
    { "low memory",   "WARNING",   55 },
    { "warn",         "WARNING",   45 },
    { "high",         "WARNING",   42 },
    { "retry",        "WARNING",   40 },
    { "slow",         "WARNING",   38 },
    { "deprecated",   "WARNING",   35 },
    /* INFO / DEBUG */
    { "debug",        "DEBUG",     10 },
    { "trace",        "DEBUG",      5 },
    { "info",         "INFO",      15 },
    { "start",        "INFO",      10 },
    { "stop",         "INFO",      10 },
    { "connect",      "INFO",      12 },
    { "disconnect",   "INFO",      12 },
};

#define RULE_COUNT       ((int)(sizeof(rules) / sizeof(rules[0])))
#define ALERT_THRESHOLD  60
#define ERROR_CODE_BONUS 10

/* ------------------------------------------------------------------ */
/* 内部辅助函数                                                        */
/* ------------------------------------------------------------------ */

/* 将 src 转小写后存入 dst，不修改原始消息 */
static void str_to_lower(char *dst, const char *src, size_t dst_size)
{
    size_t i;
    for (i = 0; i + 1 < dst_size && src[i] != '\0'; i++)
        dst[i] = (char)tolower((unsigned char)src[i]);
    dst[i] = '\0';
}

/* 检测消息中是否含具体数字错误码（0x… 或十进制整数 ≥ 100） */
static int has_error_code(const char *msg)
{
    const char *p = msg;
    while (*p != '\0') {
        if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X') &&
            isxdigit((unsigned char)p[2]))
            return 1;
        if (isdigit((unsigned char)*p) &&
            (p == msg || !isdigit((unsigned char)*(p - 1)))) {
            long val = 0;
            const char *q = p;
            while (isdigit((unsigned char)*q))
                val = val * 10 + (*q++ - '0');
            if (val >= 100) return 1;
        }
        p++;
    }
    return 0;
}

/* ================================================================== */
/* 公开接口                                                            */
/* ================================================================== */

void log_ai_init(void)
{
    printf("Log analysis module initialized.\n");
}

void log_ai_analyze(LogEntry *entry)
{
    if (entry == 0) {
        return;
    }

    /* ── Step 1：关键词分类 ──
     * 将消息转为小写副本后按优先级扫描规则表，首条命中即生效。
     * 无命中时保持原始占位值 UNCLASSIFIED，分值为 0。            */
    {
        char lower_msg[LOG_MESSAGE_SIZE];
        int  i, matched = 0;

        str_to_lower(lower_msg, entry->message, sizeof(lower_msg));

        for (i = 0; i < RULE_COUNT; i++) {
            if (strstr(lower_msg, rules[i].keyword) != 0) {
                snprintf(entry->category, sizeof(entry->category),
                         "%s", rules[i].category);
                entry->score = rules[i].base_score;
                matched = 1;
                break;
            }
        }

        if (!matched) {
            snprintf(entry->category, sizeof(entry->category), "UNCLASSIFIED");
            entry->score = 0;
        }
    }

    /* ── Step 2：评分细化 ──
     * 消息含具体错误码（0x… 或 ≥100 的十进制整数）时额外加分，
     * 反映该条日志更具可操作性；最终分值钳位至 [0, 100]。        */
    if (has_error_code(entry->message))
        entry->score += ERROR_CODE_BONUS;
    if (entry->score > 100) entry->score = 100;
    if (entry->score <   0) entry->score =   0;

    /* ── Step 3：三级告警决策 ──
     *   score < 60          → needs_alert = 0  无告警
     *   60 ≤ score < 75     → needs_alert = 1  LEVEL-1 [ELEVATED]
     *   75 ≤ score < 90     → needs_alert = 2  LEVEL-2 [HIGH]
     *   score ≥ 90          → needs_alert = 3  LEVEL-3 [CRITICAL]  */
    if (entry->score >= 90)
        entry->needs_alert = 3;
    else if (entry->score >= 75)
        entry->needs_alert = 2;
    else if (entry->score >= ALERT_THRESHOLD)
        entry->needs_alert = 1;
    else
        entry->needs_alert = 0;
}
