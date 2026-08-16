#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <sys/reboot.h>

#include "bsod.h"
#include "vt_util.h"
#include "drm_util.h"
#include "render.h"
#include "font_util.h"
#include "qr.h"
#include "i18n.h"

/* 无法获取原 VT 时的降级恢复目标 */
#define DEFAULT_ORIGIN_VT 2
/* 临时剥离 DRM 权限用的空闲 VT */
#define TARGET_VT 6

/* 设计画布（等比例缩放基准） */
#define DESIGN_W 1920
#define DESIGN_H 1080

/* 进度百分比文字在设计画布上的位置与字号（现代模式；bsod_render 与动画共用） */
#define PROGRESS_X 290
#define PROGRESS_Y 570
#define PROGRESS_SIZE 32

/* 进度到 100% 后、执行重启/恢复前的延迟秒数 */
#define RESTART_DELAY_SECS 2

#define TOTAL_STEPS 7

/* 彩虹动画：每个进度节点拆成的帧数（约 10 秒内共 7*12=84 帧整屏重绘） */
#define RAINBOW_FRAMES_PER_NODE 12
/* 经典模式闪烁：闪烁半周期包含的帧数（约 0.5 秒） */
#define BLINK_HALF_PERIOD_FRAMES 6

/* ---- 经典 Windows 7 VGA 文本模式的行布局（行号基准，50 行时 *2） ---- */
#define CLASSIC_ROW_HEAD 0    /* 顶部标题两行 */
#define CLASSIC_ROW_CUSTOM 3  /* 自定义终止代码（可闪烁） */
#define CLASSIC_ROW_FIRST 6   /* If this is the first time ... */
#define CLASSIC_ROW_CHECK 10  /* Check to make sure ... */
#define CLASSIC_ROW_PROBLEMS 14 /* If problems continue ... */
#define CLASSIC_ROW_TECH 20   /* Technical information: */
#define CLASSIC_ROW_STOP 21   /* *** STOP: ... */
#define CLASSIC_ROW_COLLECT 22 /* Collecting data for crash dump ... */
#define CLASSIC_ROW_PROGRESS 23 /* 进度百分比专用行 */
#define CLASSIC_ROW_INIT 24   /* Initializing disk for crash dump ... */

/* 经典蓝屏的静态正文（与真实 Windows 7 文本蓝屏一致，仅英文） */
static const char *const classic_head_lines[] = {
    "A problem has been detected and Windows has been shut down to prevent damage",
    "to your computer.",
    NULL,
};
static const char *const classic_first_lines[] = {
    "If this is the first time you've seen this Stop error screen,",
    "restart your computer. If this screen appears again, follow",
    "these steps:",
    NULL,
};
static const char *const classic_check_lines[] = {
    "Check to make sure any new hardware or software is properly installed.",
    "If this is a new installation, ask your hardware or software manufacturer",
    "for any Windows updates you might need.",
    NULL,
};
static const char *const classic_problems_lines[] = {
    "If problems continue, disable or remove any newly installed hardware",
    "or software. Disable BIOS memory options such as caching or shadowing.",
    "If you need to use Safe Mode to remove or disable components, restart",
    "your computer, press F8 to select Advanced Startup Options, and then",
    "select Safe Mode.",
    NULL,
};

static void log_step(int step, const char *msg)
{
    printf("[%d/%d] %s\n", step, TOTAL_STEPS, msg);
}

/* 带 i18n 与格式化参数的步骤日志 */
static void log_stepf(int step, enum i18n_msg id, ...)
{
    char buf[256];
    va_list ap;

    va_start(ap, id);
    vsnprintf(buf, sizeof(buf), i18n_tr(id), ap);
    va_end(ap);
    log_step(step, buf);
}

/* 把画布坐标映射到屏幕坐标（等比例缩放并居中，保证完整显示） */
static void map_design(int sw, int sh, float *scale, int *ox, int *oy)
{
    *scale = (sw / (float)DESIGN_W) < (sh / (float)DESIGN_H)
                 ? (sw / (float)DESIGN_W)
                 : (sh / (float)DESIGN_H);
    *ox = (int)((sw - DESIGN_W * *scale) / 2.0f);
    *oy = (int)((sh - DESIGN_H * *scale) / 2.0f);
}

/* HSV -> 0x00RRGGBB（彩虹蓝屏背景/前景用，与 Windows 版预览算法一致） */
static uint32_t hsv_to_rgb(double hue, double saturation, double value)
{
    double chroma = value * saturation;
    double h = fmod(hue, 360.0) / 60.0;
    double x = chroma * (1.0 - fabs(fmod(h, 2.0) - 1.0));
    double r = 0.0, g = 0.0, b = 0.0;
    double m = value - chroma;
    uint32_t rr, gg, bb;

    if (h < 1.0)
    {
        r = chroma;
        g = x;
    }
    else if (h < 2.0)
    {
        r = x;
        g = chroma;
    }
    else if (h < 3.0)
    {
        g = chroma;
        b = x;
    }
    else if (h < 4.0)
    {
        g = x;
        b = chroma;
    }
    else if (h < 5.0)
    {
        r = x;
        b = chroma;
    }
    else
    {
        r = chroma;
        b = x;
    }

    rr = (uint32_t)((r + m) * 255.0);
    gg = (uint32_t)((g + m) * 255.0);
    bb = (uint32_t)((b + m) * 255.0);
    return (rr << 16) | (gg << 8) | bb;
}

/* 当前蓝屏主题（由 bsod_set_theme() 设置，未设置时为默认） */
static struct bsod_theme g_theme;
static int g_theme_ready = 0;

void bsod_set_theme(const struct bsod_theme *theme)
{
    if (theme)
    {
        g_theme = *theme;
        g_theme_ready = 1;
    }
}

/* 按当前语言选择字体族，保证蓝屏文字在对应文字系统下可渲染 */
static const char *font_family_for_lang(void)
{
    const char *code = i18n_lang_code();

    if (strcmp(code, "zh-TW") == 0)
        return "Noto Sans CJK TC";
    if (strcmp(code, "ja") == 0)
        return "Noto Sans CJK JP";
    if (strcmp(code, "ko") == 0)
        return "Noto Sans CJK KR";
    if (strcmp(code, "zh-CN") == 0)
        return "Noto Sans CJK SC";
    return "Noto Sans"; /* en：拉丁 */
}

/* 经典 VGA 文本模式使用等宽字体（更像老式字符屏） */
static const char *font_family_for_mode(const struct bsod_theme *t)
{
    return t->classic ? "DejaVu Sans Mono" : font_family_for_lang();
}

/* 在 (x,y) 处绘制文字；box 非零时先在该文字块后铺一层背景色。
 * （对应 Windows 版 CC 命令的文字背景色；box 与屏幕背景相同则不可见） */
static void draw_text_block(font_ctx *fc, render_ctx *r, int x, int y,
                            const char *text, unsigned int size,
                            uint32_t fg, uint32_t box)
{
    if (box)
    {
        int w = font_text_width(fc, text, size);
        int h = font_line_height(fc, size);
        render_fill_rect(r, x, y, w, h, box);
    }
    font_draw_text(fc, r, x, y, text, fg, size);
}

/* 渲染现代蓝屏内容（需字体已加载）。原因串用于终止代码、帮助链接与二维码。
 * bg/fg 为实际要用的颜色（静态即主题默认，彩虹时每帧传入当前色相）；
 * box 为文字背景色块（0 表示不绘制）。 */
static void bsod_render_modern(font_ctx *fc, render_ctx *r, int sw, int sh,
                               const char *reason, const struct bsod_theme *t,
                               uint32_t bg, uint32_t fg, uint32_t box)
{
    float SCALE;
    int OX, OY;
    char link[512];
    char stop[300];
    char pct0[32];
    int i;

    map_design(sw, sh, &SCALE, &OX, &OY);

    /* 帮助链接与终止代码（自定义终止代码优先，否则嵌入原因串） */
    snprintf(link, sizeof(link), "https://stackoverflow.com/search?q=%s", reason);
    if (t->custom_stop_code && t->stop_code[0])
        snprintf(stop, sizeof(stop), i18n_tr(MSG_RENDER_STOP), t->stop_code);
    else
        snprintf(stop, sizeof(stop), i18n_tr(MSG_RENDER_STOP), reason);

    /* 二维码：内容 = 帮助链接；模块色跟随背景色（彩虹时随色相变化） */
    {
        qr_matrix qr;
        qr_style style = {
            .fg = bg,
            .bg = 0xFFFFFFFF,
            .quiet = 2,
        };
        if (qr_generate_text(&qr, link, 1, 10) == 0)
        {
            qr_draw(r, &qr, OX + (int)(290 * SCALE), OY + (int)(650 * SCALE),
                    (int)(129 * SCALE), &style);
            qr_matrix_free(&qr);
        }
    }

    /* 文字 */
    draw_text_block(fc, r, OX + (int)(290 * SCALE), OY + (int)(214 * SCALE),
                    ":(", (unsigned int)(180 * SCALE), fg, box);
    draw_text_block(fc, r, OX + (int)(290 * SCALE), OY + (int)(448 * SCALE),
                    i18n_tr(MSG_RENDER_TITLE), (unsigned int)(32 * SCALE), fg, box);
    draw_text_block(fc, r, OX + (int)(290 * SCALE), OY + (int)(497 * SCALE),
                    i18n_tr(MSG_RENDER_SUB), (unsigned int)(32 * SCALE), fg, box);
    snprintf(pct0, sizeof(pct0), i18n_tr(MSG_RENDER_PERCENT), 0);
    draw_text_block(fc, r, OX + (int)(PROGRESS_X * SCALE),
                    OY + (int)(PROGRESS_Y * SCALE),
                    pct0, (unsigned int)(PROGRESS_SIZE * SCALE), fg, box);
    draw_text_block(fc, r, OX + (int)(455 * SCALE), OY + (int)(650 * SCALE),
                    i18n_tr(MSG_RENDER_VISIT), (unsigned int)(18 * SCALE), fg, box);
    draw_text_block(fc, r, OX + (int)(455 * SCALE), OY + (int)(685 * SCALE),
                    link, (unsigned int)(18 * SCALE), fg, box);
    draw_text_block(fc, r, OX + (int)(455 * SCALE), OY + (int)(732 * SCALE),
                    i18n_tr(MSG_RENDER_HELP), (unsigned int)(18 * SCALE), fg, box);
    draw_text_block(fc, r, OX + (int)(455 * SCALE), OY + (int)(767 * SCALE),
                    stop, (unsigned int)(18 * SCALE), fg, box);

    /* 自定义显示字符串（现代模式：设计画布坐标） */
    for (i = 0; i < t->string_count; i++)
    {
        const struct bsod_string_item *s = &t->strings[i];
        font_draw_text(fc, r, OX + (int)(s->x * SCALE), OY + (int)(s->y * SCALE),
                       s->text, s->color_set ? s->color : fg,
                       (unsigned int)(s->size * SCALE));
    }
}

/* 经典 VGA 文本模式：在 (col,row) 处绘制一行文字（左上对齐 + 垂直居中） */
static void classic_draw_line(font_ctx *fc, render_ctx *r, int cell_w, int cell_h,
                              unsigned int size, int col, int row,
                              const char *text, uint32_t color,
                              int frame, int blink)
{
    int x = col * cell_w;
    int y = row * cell_h + (cell_h - (int)font_line_height(fc, size)) / 2;

    if (blink && ((frame / BLINK_HALF_PERIOD_FRAMES) & 1))
        return; /* 闪烁关闭的半周期不绘制 */
    font_draw_text(fc, r, x, y, text, color, size);
}

/* 连续多行绘制（行号依次递增） */
static void classic_draw_lines(font_ctx *fc, render_ctx *r, int cell_w, int cell_h,
                               unsigned int size, int row0,
                               const char *const *lines, uint32_t color,
                               int frame, int blink)
{
    int i;

    for (i = 0; lines[i]; i++)
        classic_draw_line(fc, r, cell_w, cell_h, size, 0, row0 + i,
                          lines[i], color, frame, blink);
}

/* 渲染经典 Windows 7 VGA 文本模式蓝屏。fg 为实际文字颜色（彩虹时随帧变化）；
 * frame 为动画帧计数（用于闪烁）。背景色由调用方在 render_clear 中填充。
 * 自定义字符串 x=列 y=行。 */
static void bsod_render_classic(font_ctx *fc, render_ctx *r, int sw, int sh,
                                const char *reason, const struct bsod_theme *t,
                                uint32_t fg, int frame)
{
    int k = (t->classic_rows == 50) ? 2 : 1; /* 80x50 时行号稀疏一倍 */
    int cell_w = sw / t->classic_cols;
    int cell_h = sh / t->classic_rows;
    unsigned int size = (unsigned int)(cell_h * 4 / 5);
    const char *custom = (t->custom_stop_code && t->stop_code[0])
                             ? t->stop_code
                             : NULL;
    char stop[BSOD_STOP_CODE_MAX + 16];
    int i;

    /* 顶部标题 */
    classic_draw_lines(fc, r, cell_w, cell_h, size, CLASSIC_ROW_HEAD * k,
                       classic_head_lines, fg, frame, 0);

    /* 自定义终止代码：高亮白，blink 时闪烁 */
    if (custom)
        classic_draw_line(fc, r, cell_w, cell_h, size, 0, CLASSIC_ROW_CUSTOM * k,
                          custom, 0xFFFFFFFF, frame, t->blink);

    /* 排障说明 */
    classic_draw_lines(fc, r, cell_w, cell_h, size, CLASSIC_ROW_FIRST * k,
                       classic_first_lines, fg, frame, 0);
    classic_draw_lines(fc, r, cell_w, cell_h, size, CLASSIC_ROW_CHECK * k,
                       classic_check_lines, fg, frame, 0);
    classic_draw_lines(fc, r, cell_w, cell_h, size, CLASSIC_ROW_PROBLEMS * k,
                       classic_problems_lines, fg, frame, 0);

    /* 技术信息与终止代码 */
    classic_draw_line(fc, r, cell_w, cell_h, size, 0, CLASSIC_ROW_TECH * k,
                      "Technical information:", fg, frame, 0);
    if (custom)
        snprintf(stop, sizeof(stop), "*** STOP: %s", custom);
    else
        snprintf(stop, sizeof(stop), "*** STOP: %.70s", reason ? reason : "");
    classic_draw_line(fc, r, cell_w, cell_h, size, 0, CLASSIC_ROW_STOP * k,
                      stop, fg, frame, 0);

    /* 进度提示行 */
    classic_draw_line(fc, r, cell_w, cell_h, size, 0, CLASSIC_ROW_COLLECT * k,
                      "Collecting data for crash dump ...", fg, frame, 0);
    classic_draw_line(fc, r, cell_w, cell_h, size, 0, CLASSIC_ROW_INIT * k,
                      "Initializing disk for crash dump ...", fg, frame, 0);

    /* 自定义显示字符串（经典模式：x=列 y=行） */
    for (i = 0; i < t->string_count; i++)
    {
        const struct bsod_string_item *s = &t->strings[i];
        classic_draw_line(fc, r, cell_w, cell_h, size, s->x, s->y, s->text,
                          s->color_set ? s->color : fg, frame, 0);
    }
}

/* 进度动画（现代模式）：在 (tx,ty) 处以字号 fs 显示百分比，约 10 秒到 100%。
 * 非彩虹：只局部刷新进度文字；彩虹：整屏重绘，背景色相随进度推进循环。 */
static void animate_modern(font_ctx *fc, render_ctx *r, drm_bsod_ctx *drm,
                           int sw, int sh, const char *reason,
                           const struct bsod_theme *t,
                           int tx, int ty, unsigned int fs)
{
    /* 任意选取的百分比节点，最后到 100% */
    static const int nodes[] = {12, 27, 45, 63, 78, 90, 100};
    const int n = (int)(sizeof(nodes) / sizeof(nodes[0]));
    char buf[32];
    int i;

    if (!t->rainbow)
    {
        const int tw = font_text_width(fc, i18n_tr(MSG_RENDER_PERCENT_100), fs);

        for (i = 0; i < n; i++)
        {
            /* 局部刷新：先清掉旧文字（填回底色），再画新百分比 */
            render_fill_rect(r, tx, ty, tw, fs, t->bg);
            snprintf(buf, sizeof(buf), i18n_tr(MSG_RENDER_PERCENT), nodes[i]);
            font_draw_text(fc, r, tx, ty, buf, t->fg, fs);
            drm_flip(drm);        /* 重新推屏，显示最新一帧 */
            usleep(10000000 / n); /* 约 10 秒分摊到每个节点 */
        }
        return;
    }

    /* 彩虹模式：每个节点拆成若干帧，整屏以当前色相重绘 */
    {
        const int fpn = RAINBOW_FRAMES_PER_NODE;
        const useconds_t frame_us = (10000000 / n) / fpn;
        const int tw = font_text_width(fc, i18n_tr(MSG_RENDER_PERCENT_100), fs);
        const int th = font_line_height(fc, fs);
        int f;
        double hue = 0.0;

        for (i = 0; i < n; i++)
        {
            for (f = 0; f < fpn; f++)
            {
                uint32_t bg = hsv_to_rgb(hue, 1.0, 1.0);

                render_clear(r, bg);
                bsod_render_modern(fc, r, sw, sh, reason, t, bg, t->fg, 0);
                /* bsod_render_modern 已在进度处画 "0%"，先清掉该区域再画当前百分比，避免重叠 */
                render_fill_rect(r, tx, ty, tw, th, bg);
                snprintf(buf, sizeof(buf), i18n_tr(MSG_RENDER_PERCENT), nodes[i]);
                font_draw_text(fc, r, tx, ty, buf, t->fg, fs);
                drm_flip(drm);
                usleep(frame_us);
                hue = fmod(hue + t->rainbow_speed * frame_us / 1000000.0, 360.0);
            }
        }
    }
}

/* 进度动画（经典模式）：进度百分比显示在 CLASSIC_ROW_PROGRESS 行。
 * 非彩虹且不闪烁：只局部刷新进度行；否则整屏重绘（闪烁/彩虹）。 */
static void animate_classic(font_ctx *fc, render_ctx *r, drm_bsod_ctx *drm,
                            int sw, int sh, const char *reason,
                            const struct bsod_theme *t,
                            int cell_w, int cell_h, unsigned int size)
{
    static const int nodes[] = {12, 27, 45, 63, 78, 90, 100};
    const int n = (int)(sizeof(nodes) / sizeof(nodes[0]));
    const int progress_row = CLASSIC_ROW_PROGRESS * ((t->classic_rows == 50) ? 2 : 1);
    char buf[32];
    int i;

    if (!t->rainbow && !t->blink)
    {
        for (i = 0; i < n; i++)
        {
            render_fill_rect(r, 0, progress_row * cell_h,
                             t->classic_cols * cell_w, cell_h, t->bg);
            snprintf(buf, sizeof(buf), i18n_tr(MSG_RENDER_PERCENT), nodes[i]);
            classic_draw_line(fc, r, cell_w, cell_h, size, 0, progress_row,
                              buf, t->fg, 0, 0);
            drm_flip(drm);
            usleep(10000000 / n);
        }
        return;
    }

    /* 彩虹或闪烁：整屏重绘 */
    {
        const int fpn = RAINBOW_FRAMES_PER_NODE;
        const useconds_t frame_us = (10000000 / n) / fpn;
        int f;
        int frame = 0;
        double hue = 0.0;

        for (i = 0; i < n; i++)
        {
            for (f = 0; f < fpn; f++, frame++)
            {
                uint32_t bg = t->bg;
                uint32_t fg = t->fg;

                if (t->rainbow)
                {
                    /* 经典彩虹前景用暗补色，与 Windows 版预览一致 */
                    bg = hsv_to_rgb(hue, 1.0, 1.0);
                    fg = hsv_to_rgb(fmod(hue + 180.0, 360.0), 1.0, 0.15);
                }
                render_clear(r, bg);
                bsod_render_classic(fc, r, sw, sh, reason, t, fg, frame);
                snprintf(buf, sizeof(buf), i18n_tr(MSG_RENDER_PERCENT), nodes[i]);
                classic_draw_line(fc, r, cell_w, cell_h, size, 0, progress_row,
                                  buf, fg, frame, 0);
                drm_flip(drm);
                usleep(frame_us);
                hue = fmod(hue + t->rainbow_speed * frame_us / 1000000.0, 360.0);
            }
        }
    }
}

static enum bsod_exit_mode g_exit_mode = BSOD_EXIT_REBOOT;

void bsod_set_exit_mode(enum bsod_exit_mode mode)
{
    g_exit_mode = mode;
}

int bsod_show(const char *reason)
{
    drm_bsod_ctx drm;
    font_ctx fc = {0};
    int tty_fd = -1;
    int origin_vt = -1;
    int ret = -1;

    if (!g_theme_ready)
    {
        bsod_theme_default(&g_theme);
        g_theme_ready = 1;
    }

    if (!reason)
        reason = "";

    drm_bsod_ctx_init(&drm);

    /* 1. 获取当前活动 VT 编号 */
    log_stepf(1, MSG_STEP_OPEN_TTY);
    tty_fd = vt_open_tty();
    if (tty_fd < 0)
    {
        goto cleanup;
    }

    origin_vt = vt_get_active(tty_fd);
    if (origin_vt < 0)
    {
        origin_vt = DEFAULT_ORIGIN_VT; /* 降级默认值 */
    }
    else
    {
        printf(i18n_tr(MSG_DESKTOP_ON_TTY), origin_vt);
    }

    /* 2. 切到空闲 VT，让图形界面挂起并释放 DRM 权限 */
    if (origin_vt != TARGET_VT)
    {
        log_stepf(2, MSG_STEP_SWITCH_VT, TARGET_VT);
        if (vt_switch_to(tty_fd, TARGET_VT) < 0)
        {
            goto cleanup;
        }
    }

    /* 3. 打开 DRM 设备并抢占 Master 控制权（NULL = 自动探测 card0..card7） */
    log_stepf(3, MSG_STEP_OPEN_DRM);
    if (drm_open_device(&drm, NULL) < 0)
    {
        goto cleanup;
    }
    log_stepf(4, MSG_STEP_SET_MASTER);

    /* 4. 获取物理分辨率与显示资源 */
    if (drm_find_connector(&drm) < 0)
    {
        goto cleanup;
    }
    log_stepf(5, MSG_STEP_RESOLUTION, drm.width, drm.height);

    /* 5. 创建 dumb framebuffer 并渲染蓝屏底色 */
    log_stepf(6, MSG_STEP_FILL_FB);
    if (drm_create_fb(&drm) < 0)
    {
        goto cleanup;
    }

    /* 5b. 渲染演示内容：清屏 / 任意位置写字 / 二维码 / 经典文本模式 */
    {
        render_ctx rctx;
        int SCREEN_W = (int)drm.width;
        int SCREEN_H = (int)drm.height;
        int font_ok = 0;

        render_init(&rctx, drm.map_ptr, drm.width, drm.height, drm.pitch);

        /* 清屏：整屏填充为主题背景色 */
        render_clear(&rctx, g_theme.bg);

        /* 绘制蓝屏内容（需字体加载） */
        if (font_init(&fc) == 0 &&
            font_load(&fc, font_family_for_mode(&g_theme), 48) == 0)
        {
            font_ok = 1;
            if (g_theme.classic)
                bsod_render_classic(&fc, &rctx, SCREEN_W, SCREEN_H, reason,
                                    &g_theme, g_theme.fg, 0);
            else
                bsod_render_modern(&fc, &rctx, SCREEN_W, SCREEN_H, reason,
                                   &g_theme, g_theme.bg, g_theme.fg,
                                   g_theme.text_bg != g_theme.bg ? g_theme.text_bg : 0);
        }

        /* 6. 推送到屏幕；即使失败也保持原始流程：继续进度动画并重启/恢复 */
        log_stepf(7, MSG_STEP_PUSH_DONE);
        drm_set_mode(&drm);

        /* 进度动画：10 秒内若干百分比节点递增，最后到 100% */
        if (font_ok)
        {
            if (g_theme.classic)
            {
                int cell_w = SCREEN_W / g_theme.classic_cols;
                int cell_h = SCREEN_H / g_theme.classic_rows;
                unsigned int size = (unsigned int)(cell_h * 4 / 5);

                animate_classic(&fc, &rctx, &drm, SCREEN_W, SCREEN_H, reason,
                                &g_theme, cell_w, cell_h, size);
            }
            else
            {
                float SCALE;
                int OX, OY;

                map_design(SCREEN_W, SCREEN_H, &SCALE, &OX, &OY);
                animate_modern(&fc, &rctx, &drm, SCREEN_W, SCREEN_H, reason,
                               &g_theme,
                               OX + (int)(PROGRESS_X * SCALE),
                               OY + (int)(PROGRESS_Y * SCALE),
                               (unsigned int)(PROGRESS_SIZE * SCALE));
            }
        }

        font_cleanup(&fc);
    }

    /* 进度到 100% 后延迟 2 秒，再按模式执行（默认重启系统） */
    printf(i18n_tr(MSG_AFTER_DELAY), RESTART_DELAY_SECS,
           i18n_tr(g_exit_mode == BSOD_EXIT_REBOOT ? MSG_ACTION_REBOOT
                                                   : MSG_ACTION_RESTORE));
    sleep(RESTART_DELAY_SECS);

    if (g_exit_mode == BSOD_EXIT_REBOOT)
    {
        /* 刷新磁盘缓冲后重启系统（成功后不会返回） */
        sync();
        if (reboot(RB_AUTOBOOT) == 0)
            return 0;
        perror(i18n_tr(MSG_REBOOT_FAILED));
    }

    /* 恢复模式（或 reboot 失败回退）：还原显示并退出 */
    drm_restore(&drm);

    ret = 0;

cleanup:
    printf("%s", i18n_tr(MSG_CLEANUP));
    drm_cleanup(&drm);

    /* 7. 切回原本的 VT，恢复桌面 Display Server 的控制权 */
    if (tty_fd >= 0)
    {
        if (origin_vt > 0 && origin_vt != TARGET_VT)
        {
            printf(i18n_tr(MSG_SWITCH_BACK), origin_vt);
            vt_switch_to(tty_fd, origin_vt);
        }
        close(tty_fd);
    }

    printf("%s", i18n_tr(MSG_RESTORED));
    return ret;
}
