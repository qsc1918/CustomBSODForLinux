#ifndef QR_H
#define QR_H

#include <stdint.h>

#include "render.h"

/**
 * @file qr.h
 * @brief 二维码绘制
 *
 * 使用内置的 qrcodegen 库（Nayuki, MIT License）生成可扫描的真实二维码，
 * 绘制统一复用 qr_draw()。
 */

/** @brief 二维码模块矩阵 */
typedef struct
{
    int size;         /*!< 矩阵边长（模块数） */
    uint8_t *modules; /*!< size*size，1=黑色模块，0=白色模块 */
} qr_matrix;

/** @brief 二维码绘制样式 */
typedef struct
{
    uint32_t fg;    /*!< 模块颜色（内容） */
    uint32_t bg;    /*!< 背景面板颜色 */
    uint32_t frame; /*!< 外边框颜色 */
    int border;     /*!< 外边框宽度（像素） */
    int quiet;      /*!< 二维码四周留白（模块数，标准为 4） */
} qr_style;

/**
 * @brief 分配模块矩阵内存
 * @param q    矩阵指针
 * @param size 边长（模块数）
 * @return 成功返回 0，失败返回 -1
 */
int qr_matrix_alloc(qr_matrix *q, int size);

/**
 * @brief 释放模块矩阵内存
 * @param q 矩阵指针
 */
void qr_matrix_free(qr_matrix *q);

/**
 * @brief 生成真实二维码（编码给定文本）
 *
 * 使用内置 qrcodegen 库把 UTF-8 文本编码为可扫描的二维码模块矩阵。
 * @param q          矩阵指针（自动分配内存，用后需 qr_matrix_free）
 * @param text       UTF-8 文本内容
 * @param min_version 最小版本（1~40）
 * @param max_version 最大版本（1~40）
 * @return 成功返回 0，失败返回 -1（文本过长或编码失败）
 */
int qr_generate_text(qr_matrix *q, const char *text, int min_version,
                     int max_version);

/**
 * @brief 在帧缓冲指定位置绘制二维码
 *
 * 布局由外到内：外边框(border) -> 背景面板(bg) -> 四周留白(quiet) -> 模块(fg)。
 * @param r     渲染上下文
 * @param q     模块矩阵
 * @param x,y   外边框左上角坐标
 * @param size  二维码总宽高（像素），即数据区域边长；单个模块大小由 size/q->size
 *              反推，因取整实际绘制边长可能略小于 size
 * @param style 绘制样式
 */
void qr_draw(render_ctx *r, const qr_matrix *q, int x, int y,
             int size, const qr_style *style);

#endif /* QR_H */
