#ifndef BSOD_THEME_H
#define BSOD_THEME_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @file bsod_theme.h
 * @brief 蓝屏外观主题（移植自 CustomBSOD 的自定义项）
 *
 * 提供 CustomBSOD 可自定义功能在 Linux 上的对应实现：
 *  - 背景颜色 / 前景文字颜色 / 文字背景色（对应 Windows 版 CR / CC 命令）
 *  - 自定义终止代码 Stop Code（对应 SP 命令）
 *  - 自定义显示字符串（对应 DS 命令）
 *  - 彩虹蓝屏（对应 RD / R7 命令）
 *  - 经典 Windows 7 VGA 文本模式 80x25 / 80x50、闪烁、彩虹（对应 VGA 模式）
 *
 * 配置来源：默认值 -> 配置文件（/etc/bsod.conf、~/.config/bsod/bsod.conf，
 * 或 --config 指定）-> 命令行选项（优先级最高）。
 */

#define BSOD_STRING_TEXT_MAX 256
#define BSOD_MAX_STRINGS 64
#define BSOD_STOP_CODE_MAX 160

/**
 * @brief 自定义显示字符串条目
 *
 * 现代（非 classic）模式下 x/y 为 1920x1080 设计画布坐标（像素），size 为字号；
 * 经典 VGA 文本模式下 x=列、y=行（0 起始的单元格），size 忽略（用单元格字号）。
 */
struct bsod_string_item
{
    char text[BSOD_STRING_TEXT_MAX];
    int x;          /*!< 现代：x 像素；经典：列号 */
    int y;          /*!< 现代：y 像素；经典：行号 */
    unsigned size;  /*!< 现代：字号；经典：忽略 */
    uint32_t color; /*!< 文字颜色 0x00RRGGBB */
    bool color_set; /*!< 是否显式指定颜色（否则使用前景色） */
};

/** @brief 蓝屏外观主题 */
struct bsod_theme
{
    uint32_t bg;     /*!< 屏幕背景颜色 0x00RRGGBB */
    uint32_t fg;     /*!< 前景/文字颜色 0x00RRGGBB */
    uint32_t text_bg; /*!< 文字背景色（现代模式：在文字后绘制色块；默认 = bg 即不绘制） */

    char stop_code[BSOD_STOP_CODE_MAX]; /*!< 自定义终止代码文本 */
    bool custom_stop_code;              /*!< 是否启用自定义终止代码 */

    bool rainbow;        /*!< 彩虹蓝屏：背景色相持续循环变化 */
    double rainbow_speed; /*!< 色相变化速度（度/秒） */

    bool classic;      /*!< 经典 Windows 7 VGA 文本模式蓝屏 */
    int classic_cols;  /*!< 列数（默认 80） */
    int classic_rows;  /*!< 行数（25 或 50，默认 25） */
    bool blink;        /*!< 经典模式：终止代码行闪烁 */

    struct bsod_string_item strings[BSOD_MAX_STRINGS];
    int string_count; /*!< 已添加的自定义字符串数量 */
};

/**
 * @brief 初始化主题为默认值
 * @param t 主题指针
 */
void bsod_theme_default(struct bsod_theme *t);

/**
 * @brief 从配置文件加载主题（覆盖未显式指定的项）
 *
 * 语法为简单的 key = value 行；`#`/`;` 开头为注释；
 * `[strings]` 段内每行 `标识 = text|x|y|size|color` 是一个自定义字符串条目。
 * 支持键：background/bg、foreground/fg、text_background/text_bg、stop_code、
 * rainbow、rainbow_speed、vga/classic、vga_cols、vga_rows、vga_blink/blink。
 *
 * @param t    主题指针
 * @param path 配置文件路径
 * @return 成功返回 0，打开/解析失败返回 -1（t 保持原样或已加载部分）
 */
int bsod_theme_load_file(struct bsod_theme *t, const char *path);

/**
 * @brief 解析一个命令行参数（argv[i]，为标题中的 bsod_theme 选项）
 *
 * 需要取值的选项会推进 *i；非主题选项返回 0 不动。
 *
 * @param t    主题指针
 * @param argc 参数个数
 * @param argv 参数数组
 * @param i    当前下标（会推进）
 * @return 1=已消费该参数；0=非主题选项；-1=主题选项参数缺失/非法
 */
int bsod_theme_parse_arg(struct bsod_theme *t, int argc, char **argv, int *i);

/**
 * @brief 解析 6 位十六进制颜色（支持 #RRGGBB / 0xRRGGBB / RRGGBB）
 * @param s    颜色字符串
 * @param out  输出 0x00RRGGBB
 * @return 成功返回 0，失败返回 -1
 */
int bsod_parse_color(const char *s, uint32_t *out);

/**
 * @brief 解析并添加一个自定义字符串条目
 *
 * 格式：`text|x|y|size|color`，字段以 `|` 分隔，color 可省略（使用前景色）。
 * 现代模式 x/y 为设计画布坐标，经典模式 x=列 y=行。
 *
 * @param t    主题指针
 * @param spec 字符串条目描述
 * @return 成功返回 0，失败返回 -1
 */
int bsod_theme_add_string(struct bsod_theme *t, const char *spec);

#endif /* BSOD_THEME_H */
