#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

/**
 * @file render.h
 * @brief 帧缓冲绘制原语
 *
 * 直接操作 DRM dumb framebuffer（XRGB8888, 32bpp）的像素绘制函数，
 * 供清屏、写字、二维码等上层能力复用。
 */

/** @brief 渲染上下文：描述一块可绘制的帧缓冲 */
typedef struct
{
    uint32_t *buf;   /*!< 帧缓冲像素基址 */
    uint32_t width;  /*!< 可见宽度（像素） */
    uint32_t height; /*!< 可见高度（像素） */
    uint32_t stride; /*!< 每行字节数（含可能的 padding） */
} render_ctx;

/**
 * @brief 初始化渲染上下文
 * @param r      上下文指针
 * @param buf    帧缓冲像素基址
 * @param width  可见宽度（像素）
 * @param height 可见高度（像素）
 * @param stride 每行字节数
 */
void render_init(render_ctx *r, uint32_t *buf, uint32_t width,
                 uint32_t height, uint32_t stride);

/**
 * @brief 清屏：将整屏填充为指定颜色（0x00RRGGBB）
 * @param r     上下文指针
 * @param color 填充颜色
 */
void render_clear(render_ctx *r, uint32_t color);

/**
 * @brief 填充一个矩形区域（自动裁剪到屏幕范围内）
 * @param r     上下文指针
 * @param x,y   左上角坐标
 * @param w,h   宽高
 * @param color 填充颜色
 */
void render_fill_rect(render_ctx *r, int x, int y, int w, int h,
                      uint32_t color);

/**
 * @brief 将颜色按 alpha 混合写入一个像素
 * @param r     上下文指针
 * @param x,y   像素坐标
 * @param color 前景色（0x00RRGGBB）
 * @param alpha 不透明度 0~255（255 表示直接覆盖）
 */
void render_blend_pixel(render_ctx *r, int x, int y, uint32_t color,
                        uint8_t alpha);

#endif /* RENDER_H */
