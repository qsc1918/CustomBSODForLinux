#ifndef FONT_UTIL_H
#define FONT_UTIL_H

#include <stdint.h>

#include "render.h"

/**
 * @file font_util.h
 * @brief 文本渲染工具（FreeType + fontconfig）
 *
 * 通过 fontconfig 从系统字体目录（/usr/share/fonts 等）按族名发现字体，
 * 使用 FreeType 光栅化字形，并把抗锯齿位图混合进帧缓冲。
 * 若未启用/未找到 fontconfig，则回退到扫描常见系统字体目录。
 */

/** @brief 字体上下文：一次字体加载/渲染的独立状态（可同时持有多个） */
typedef struct
{
    void *library; /*!< FT_Library（对上层隐藏类型） */
    void *face;    /*!< FT_Face（对上层隐藏类型） */
} font_ctx;

/**
 * @brief 初始化 FreeType 库
 * @param fc 字体上下文
 * @return 成功返回 0，失败返回 -1
 */
int font_init(font_ctx *fc);

/**
 * @brief 释放 FreeType 资源
 * @param fc 字体上下文
 */
void font_cleanup(font_ctx *fc);

/**
 * @brief 从系统字体目录加载字体（按族名 + 中文语言偏好）
 * @param fc         字体上下文
 * @param family     字体族名，如 "Noto Sans CJK SC"
 * @param pixel_size 字号（像素）
 * @return 成功返回 0，失败返回 -1
 */
int font_load(font_ctx *fc, const char *family, unsigned int pixel_size);

/**
 * @brief 直接按文件路径加载字体
 * @param fc         字体上下文
 * @param path       字体文件路径（.ttf/.otf/.ttc）
 * @param face_index 字体面序号（ttc 集合中的索引，通常为 0）
 * @param pixel_size 字号（像素）
 * @return 成功返回 0，失败返回 -1
 */
int font_load_file(font_ctx *fc, const char *path, int face_index,
                   unsigned int pixel_size);

/**
 * @brief 设置当前字号（像素）
 * @param fc         字体上下文
 * @param pixel_size 字号
 * @return 成功返回 0，失败返回 -1
 */
int font_set_size(font_ctx *fc, unsigned int pixel_size);

/**
 * @brief 计算一段文本的绘制宽度（用于居中/排版）
 * @param fc         字体上下文
 * @param text       UTF-8 文本
 * @param pixel_size 字号
 * @return 宽度（像素），失败返回 0
 */
int font_text_width(font_ctx *fc, const char *text, unsigned int pixel_size);

/**
 * @brief 计算指定字号下的行高（像素，字体 ascender-descender）
 * @param fc         字体上下文
 * @param pixel_size 字号
 * @return 行高（像素），失败返回 0
 */
int font_line_height(font_ctx *fc, unsigned int pixel_size);

/**
 * @brief 在帧缓冲的指定位置绘制文本
 * @param fc         字体上下文
 * @param r          渲染上下文
 * @param x,y        整段文字左上角坐标（对齐到实际渲染内容的最左与最顶像素）
 * @param text       UTF-8 文本
 * @param color      文字颜色（0x00RRGGBB）
 * @param pixel_size 字号（像素）
 * @return 实际绘制总宽度（像素）
 */
int font_draw_text(font_ctx *fc, render_ctx *r, int x, int y,
                   const char *text, uint32_t color, unsigned int pixel_size);

#endif /* FONT_UTIL_H */
