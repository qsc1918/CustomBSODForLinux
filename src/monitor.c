#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <systemd/sd-journal.h>

#include "monitor.h"
#include "bsod.h"
#include "i18n.h"

#define REASON_MAX 512

/* 取 journal 字段的值：返回指向 "FIELD=" 之后内容的指针，长度存 *out_len */
static const char *jfield(sd_journal *j, const char *name, size_t *out_len)
{
    const void *data;
    size_t len;
    size_t prefix = strlen(name) + 1;

    if (sd_journal_get_data(j, name, &data, &len) < 0)
        return NULL;
    if (len <= prefix)
        return NULL;
    *out_len = len - prefix;
    return (const char *)data + prefix;
}

/* 大小写不敏感子串查找 */
static int contains(const char *haystack, const char *needle)
{
    size_t n = strlen(needle);

    for (; *haystack; haystack++)
    {
        if (strncasecmp(haystack, needle, n) == 0)
            return 1;
    }
    return 0;
}

/* 把日志条目内容拷贝到 buf（带长度上限），返回是否非空 */
static void copy_to(char *buf, size_t bufsz, const char *s, size_t len)
{
    if (len >= bufsz)
        len = bufsz - 1;
    memcpy(buf, s, len);
    buf[len] = '\0';
}

/* 判断当前日志条目是否为需要触发蓝屏的错误；是则把一行原因写入 reason */
static int entry_to_reason(sd_journal *j, char *reason, size_t reason_sz)
{
    const char *val;
    size_t len;
    char msg[REASON_MAX] = "";
    char ident[128] = "";
    int prio = 7; /* LOG_NOTICE */

    if ((val = jfield(j, "PRIORITY", &len)) != NULL && len > 0)
        prio = atoi(val);

    if ((val = jfield(j, "MESSAGE", &len)) != NULL)
        copy_to(msg, sizeof(msg), val, len);

    /* 用标识符/进程名/服务名作为来源 */
    if ((val = jfield(j, "SYSLOG_IDENTIFIER", &len)) != NULL && len > 0)
        copy_to(ident, sizeof(ident), val, len);
    else if ((val = jfield(j, "_COMM", &len)) != NULL && len > 0)
        copy_to(ident, sizeof(ident), val, len);
    else if ((val = jfield(j, "UNIT", &len)) != NULL && len > 0)
        copy_to(ident, sizeof(ident), val, len);
    else
        snprintf(ident, sizeof(ident), "日志");

    /* 排除本进程自身产生的日志 */
    if ((val = jfield(j, "_PID", &len)) != NULL && len > 0)
    {
        if (atoi(val) == (int)getpid())
            return 0;
    }

    if (msg[0] == '\0')
        return 0;

    /* 判定：优先级 <= err(3)，或消息含失败特征 */
    if (prio > 3 &&
        !contains(msg, "failed") &&
        !contains(msg, "error") &&
        !contains(msg, "result: fail") &&
        !contains(msg, "main process exited") &&
        !contains(msg, "failed to start"))
    {
        return 0;
    }

    snprintf(reason, reason_sz, "%s: %s", ident, msg);
    return 1;
}

/* 定位到日志尾部：seek_tail 后需 previous 把游标移到最后一个条目，
 * 这样后续 sd_journal_next 才能发现之后新写入的条目。 */
static void seek_end(sd_journal *j)
{
    sd_journal_seek_tail(j);
    sd_journal_previous(j);
}

int monitor_run(int dry_run)
{
    sd_journal *j = NULL;
    int r;

    r = sd_journal_open(&j, SD_JOURNAL_LOCAL_ONLY);
    if (r < 0)
    {
        fprintf(stderr, i18n_tr(MSG_MON_OPEN_FAIL), strerror(-r));
        return -1;
    }

    /* 只关注之后新产生的日志 */
    seek_end(j);

    printf(i18n_tr(MSG_MON_START),
           i18n_tr(dry_run ? MSG_MON_DRYRUN : MSG_MON_SHOW_ACT));

    for (;;)
    {
        int k;

        /* 读取从当前位置开始的所有新条目 */
        while ((k = sd_journal_next(j)) > 0)
        {
            char reason[REASON_MAX];

            if (!entry_to_reason(j, reason, sizeof(reason)))
                continue;

            printf(i18n_tr(MSG_MON_DETECTED), reason);
            if (!dry_run)
            {
                printf("%s", i18n_tr(MSG_MON_SHOWING));
                bsod_show(reason); /* 重启：不会返回；恢复模式：返回后退出 */
                printf("%s", i18n_tr(MSG_MON_END));
                sd_journal_close(j);
                return 0;
            }
        }
        if (k < 0)
        {
            fprintf(stderr, i18n_tr(MSG_MON_NEXT_FAIL), strerror(-k));
            break;
        }

        /* 暂无新日志，等待 */
        r = sd_journal_wait(j, (uint64_t)-1);
        if (r < 0)
        {
            fprintf(stderr, i18n_tr(MSG_MON_WAIT_FAIL), strerror(-r));
            break;
        }
        /* journal 文件轮转/重建：游标失效，重新定位到尾部 */
        if (r == SD_JOURNAL_INVALIDATE)
            seek_end(j);
    }

    sd_journal_close(j);
    return 0;
}
