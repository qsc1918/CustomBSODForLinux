#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bsod_theme.h"

/* 默认值：经典 Windows 10 蓝屏 #0078D7，文字白色（与 bsod.c 原默认一致） */
#define DEFAULT_BG 0x000078D7U
#define DEFAULT_FG 0xFFFFFFFFU
#define DEFAULT_RAINBOW_SPEED 100.0 /* 度/秒，约 3.6 秒一个完整色相循环 */

void bsod_theme_default(struct bsod_theme *t)
{
    memset(t, 0, sizeof(*t));
    t->bg = DEFAULT_BG;
    t->fg = DEFAULT_FG;
    t->text_bg = DEFAULT_BG; /* 默认与背景相同 => 不在文字后绘制色块 */
    t->rainbow_speed = DEFAULT_RAINBOW_SPEED;
    t->classic_cols = 80;
    t->classic_rows = 25;
    t->string_count = 0;
}

/* 去除首尾空白 */
static char *trim(char *s)
{
    char *end;

    while (*s && isspace((unsigned char)*s))
        s++;
    end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        *--end = '\0';
    return s;
}

int bsod_parse_color(const char *s, uint32_t *out)
{
    unsigned long v;
    char *end = NULL;

    if (!s || !out)
        return -1;
    while (*s == ' ' || *s == '\t')
        s++;
    if (s[0] == '#')
        s++;
    else if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        s += 2;

    if (strlen(s) != 6)
        return -1;
    v = strtoul(s, &end, 16);
    if (end != s + 6)
        return -1;
    *out = (uint32_t)(v & 0xFFFFFF);
    return 0;
}

int bsod_theme_add_string(struct bsod_theme *t, const char *spec)
{
    char buf[512];
    char *save = NULL;
    char *p;
    struct bsod_string_item item;
    uint32_t color;
    const char *fields[5];
    int nf = 0;

    if (!t || !spec)
        return -1;
    if (t->string_count >= BSOD_MAX_STRINGS)
        return -1;
    if (strlen(spec) >= sizeof(buf))
        return -1;

    memset(&item, 0, sizeof(item));
    item.color_set = false;
    item.size = 32;

    strcpy(buf, spec);
    /* 拆分 '|' 分隔的字段 */
    for (p = strtok_r(buf, "|", &save); p && nf < 5; p = strtok_r(NULL, "|", &save))
        fields[nf++] = p;
    if (nf < 4)
        return -1; /* 至少需要 text|x|y|size */

    if (strlen(fields[0]) >= sizeof(item.text))
        return -1;
    strcpy(item.text, fields[0]);
    item.x = atoi(fields[1]);
    item.y = atoi(fields[2]);
    item.size = (unsigned)atoi(fields[3]);
    if (item.size == 0)
        item.size = 32;

    if (nf >= 5 && bsod_parse_color(fields[4], &color) == 0)
    {
        item.color = color;
        item.color_set = true;
    }

    t->strings[t->string_count] = item;
    t->string_count++;
    return 0;
}

/* 布尔值解析：true/false/1/0/yes/no/on/off */
static int parse_bool(const char *s, bool *out)
{
    if (!s || !out)
        return -1;
    if (strcasecmp(s, "true") == 0 || strcmp(s, "1") == 0 ||
        strcasecmp(s, "yes") == 0 || strcasecmp(s, "on") == 0)
    {
        *out = true;
        return 0;
    }
    if (strcasecmp(s, "false") == 0 || strcmp(s, "0") == 0 ||
        strcasecmp(s, "no") == 0 || strcasecmp(s, "off") == 0)
    {
        *out = false;
        return 0;
    }
    return -1;
}

int bsod_theme_load_file(struct bsod_theme *t, const char *path)
{
    FILE *fp;
    char line[1024];
    int in_strings = 0;
    int line_no = 0;

    if (!t || !path)
        return -1;
    fp = fopen(path, "r");
    if (!fp)
        return -1;

    while (fgets(line, sizeof(line), fp))
    {
        char *p = trim(line);
        char *eq;

        line_no++;
        if (*p == '\0' || *p == '#' || *p == ';')
            continue;

        if (*p == '[')
        {
            in_strings = (strcasecmp(p, "[strings]") == 0);
            continue;
        }

        eq = strchr(p, '=');
        if (!eq)
        {
            fprintf(stderr, "[theme] %s:%d 忽略无法解析的行: %s\n", path, line_no, p);
            continue;
        }
        *eq = '\0';
        {
            char *key = trim(p);
            char *val = trim(eq + 1);

            if (in_strings)
            {
                /* 段内任意键都当作一个字符串条目 */
                if (bsod_theme_add_string(t, val) != 0)
                    fprintf(stderr, "[theme] %s:%d 无效字符串条目: %s\n",
                            path, line_no, val);
                continue;
            }

            if (strcasecmp(key, "background") == 0 || strcasecmp(key, "bg") == 0)
            {
                if (bsod_parse_color(val, &t->bg) != 0)
                    fprintf(stderr, "[theme] %s:%d 无效颜色: %s\n", path, line_no, val);
            }
            else if (strcasecmp(key, "foreground") == 0 || strcasecmp(key, "fg") == 0)
            {
                if (bsod_parse_color(val, &t->fg) != 0)
                    fprintf(stderr, "[theme] %s:%d 无效颜色: %s\n", path, line_no, val);
            }
            else if (strcasecmp(key, "text_background") == 0 ||
                     strcasecmp(key, "text_bg") == 0)
            {
                if (bsod_parse_color(val, &t->text_bg) != 0)
                    fprintf(stderr, "[theme] %s:%d 无效颜色: %s\n", path, line_no, val);
            }
            else if (strcasecmp(key, "stop_code") == 0)
            {
                if (strlen(val) >= sizeof(t->stop_code))
                    val[sizeof(t->stop_code) - 1] = '\0';
                strcpy(t->stop_code, val);
                t->custom_stop_code = true;
            }
            else if (strcasecmp(key, "rainbow") == 0)
            {
                if (parse_bool(val, &t->rainbow) != 0)
                    fprintf(stderr, "[theme] %s:%d 无效布尔值: %s\n", path, line_no, val);
            }
            else if (strcasecmp(key, "rainbow_speed") == 0)
            {
                t->rainbow_speed = atof(val);
                if (t->rainbow_speed <= 0)
                    t->rainbow_speed = DEFAULT_RAINBOW_SPEED;
            }
            else if (strcasecmp(key, "vga") == 0 || strcasecmp(key, "classic") == 0)
            {
                if (parse_bool(val, &t->classic) != 0)
                    fprintf(stderr, "[theme] %s:%d 无效布尔值: %s\n", path, line_no, val);
            }
            else if (strcasecmp(key, "vga_cols") == 0)
            {
                int n = atoi(val);
                if (n >= 40 && n <= 200)
                    t->classic_cols = n;
            }
            else if (strcasecmp(key, "vga_rows") == 0)
            {
                int n = atoi(val);
                if (n == 25 || n == 50)
                    t->classic_rows = n;
                else
                    fprintf(stderr, "[theme] %s:%d vga_rows 仅支持 25 或 50\n",
                            path, line_no);
            }
            else if (strcasecmp(key, "vga_blink") == 0 || strcasecmp(key, "blink") == 0)
            {
                if (parse_bool(val, &t->blink) != 0)
                    fprintf(stderr, "[theme] %s:%d 无效布尔值: %s\n", path, line_no, val);
            }
            else
            {
                fprintf(stderr, "[theme] %s:%d 未知键: %s\n", path, line_no, key);
            }
        }
    }

    fclose(fp);
    return 0;
}

int bsod_theme_parse_arg(struct bsod_theme *t, int argc, char **argv, int *i)
{
    const char *arg = argv[*i];
    const char *val;

    if (strcmp(arg, "--bg") == 0 || strcmp(arg, "--background") == 0)
    {
        if (*i + 1 >= argc)
            return -1;
        val = argv[++(*i)];
        if (bsod_parse_color(val, &t->bg) != 0)
            return -1;
        return 1;
    }
    if (strcmp(arg, "--fg") == 0 || strcmp(arg, "--foreground") == 0)
    {
        if (*i + 1 >= argc)
            return -1;
        val = argv[++(*i)];
        if (bsod_parse_color(val, &t->fg) != 0)
            return -1;
        return 1;
    }
    if (strcmp(arg, "--text-bg") == 0)
    {
        if (*i + 1 >= argc)
            return -1;
        val = argv[++(*i)];
        if (bsod_parse_color(val, &t->text_bg) != 0)
            return -1;
        return 1;
    }
    if (strcmp(arg, "--stop") == 0 || strcmp(arg, "--stop-code") == 0)
    {
        if (*i + 1 >= argc)
            return -1;
        val = argv[++(*i)];
        if (strlen(val) >= sizeof(t->stop_code))
            return -1;
        strcpy(t->stop_code, val);
        t->custom_stop_code = true;
        return 1;
    }
    if (strcmp(arg, "--string") == 0)
    {
        if (*i + 1 >= argc)
            return -1;
        val = argv[++(*i)];
        if (bsod_theme_add_string(t, val) != 0)
            return -1;
        return 1;
    }
    if (strcmp(arg, "--rainbow") == 0)
    {
        t->rainbow = true;
        return 1;
    }
    if (strcmp(arg, "--rainbow-speed") == 0)
    {
        if (*i + 1 >= argc)
            return -1;
        val = argv[++(*i)];
        t->rainbow_speed = atof(val);
        if (t->rainbow_speed <= 0)
            t->rainbow_speed = DEFAULT_RAINBOW_SPEED;
        return 1;
    }
    if (strcmp(arg, "--vga") == 0 || strcmp(arg, "--classic") == 0)
    {
        t->classic = true;
        return 1;
    }
    if (strcmp(arg, "--vga-cols") == 0)
    {
        int n;
        if (*i + 1 >= argc)
            return -1;
        val = argv[++(*i)];
        n = atoi(val);
        if (n < 40 || n > 200)
            return -1;
        t->classic_cols = n;
        return 1;
    }
    if (strcmp(arg, "--vga-rows") == 0)
    {
        int n;
        if (*i + 1 >= argc)
            return -1;
        val = argv[++(*i)];
        n = atoi(val);
        if (n != 25 && n != 50)
            return -1;
        t->classic_rows = n;
        return 1;
    }
    if (strcmp(arg, "--blink") == 0)
    {
        t->blink = true;
        return 1;
    }

    return 0;
}
