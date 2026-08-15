#ifndef DRM_UTIL_H
#define DRM_UTIL_H

#include <stdint.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

/**
 * @file drm_util.h
 * @brief DRM (Direct Rendering Manager) 工具函数
 *
 * 封装打开 DRM 设备、抢占 Master、查找显示器、创建 dumb framebuffer、
 * 推送到屏幕以及恢复/清理的完整生命周期。
 */

/** @brief DRM 显示上下文：贯穿一次蓝屏显示周期的全部状态 */
typedef struct
{
    int fd;                      /*!< DRM 设备文件描述符 */
    drmModeRes *resources;       /*!< DRM 全局资源 */
    drmModeConnector *connector; /*!< 已连接的显示器 */
    drmModeCrtc *saved_crtc;     /*!< 保存的原始 CRTC 状态（用于恢复） */
    drmModeModeInfo mode;        /*!< 选中的显示模式 */
    uint32_t crtc_id;            /*!< 选中的 CRTC */
    uint32_t width;              /*!< 水平分辨率 */
    uint32_t height;             /*!< 垂直分辨率 */
    uint32_t pitch;              /*!< 每行字节数（stride，含可能的 padding） */
    uint32_t fb_id;              /*!< 帧缓冲 ID */
    uint32_t dumb_handle;        /*!< dumb buffer 句柄 */
    uint32_t dumb_size;          /*!< dumb buffer 大小（字节） */
    uint32_t *map_ptr;           /*!< 映射到用户空间的显存指针 */
} drm_bsod_ctx;

/**
 * @brief 初始化上下文（必须在其他操作前调用）
 * @param ctx 上下文指针
 */
void drm_bsod_ctx_init(drm_bsod_ctx *ctx);

/**
 * @brief 打开 DRM 设备并抢占 Master 控制权
 * @param ctx  上下文指针
 * @param path 设备路径，如 "/dev/dri/card0"；传 NULL 或空串表示自动探测：
 *            依次尝试 /dev/dri/card0..card7，选第一个带已连接显示器的卡
 * @return 成功返回 0，失败返回 -1
 */
int drm_open_device(drm_bsod_ctx *ctx, const char *path);

/**
 * @brief 查找已连接的显示器，并确定分辨率与 CRTC
 * @param ctx 上下文指针
 * @return 成功返回 0，失败返回 -1
 */
int drm_find_connector(drm_bsod_ctx *ctx);

/**
 * @brief 创建 dumb framebuffer 并映射显存（内容由调用方填充）
 * @param ctx 上下文指针
 * @return 成功返回 0，失败返回 -1
 */
int drm_create_fb(drm_bsod_ctx *ctx);

/**
 * @brief 保存原始 CRTC 状态并将帧缓冲推送到屏幕
 * @param ctx 上下文指针
 * @return 成功返回 0，失败返回 -1
 */
int drm_set_mode(drm_bsod_ctx *ctx);

/**
 * @brief 重新推送当前帧缓冲到屏幕（动画刷新用，不重复保存原始状态）
 * @param ctx 上下文指针
 * @return 成功返回 0，失败返回 -1
 */
int drm_flip(drm_bsod_ctx *ctx);

/**
 * @brief 还原保存的原始 CRTC 输出
 * @param ctx 上下文指针
 * @return 成功返回 0，失败返回 -1
 */
int drm_restore(drm_bsod_ctx *ctx);

/**
 * @brief 释放全部 DRM 资源（munmap、删 FB、销毁 dumb、放弃 Master、关闭 fd）
 * @param ctx 上下文指针
 */
void drm_cleanup(drm_bsod_ctx *ctx);

#endif /* DRM_UTIL_H */
