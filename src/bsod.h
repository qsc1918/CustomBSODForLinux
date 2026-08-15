#ifndef BSOD_H
#define BSOD_H

#include "bsod_theme.h"

/**
 * @file bsod.h
 * @brief 蓝屏入口
 */

/** @brief 蓝屏结束后的行为 */
enum bsod_exit_mode
{
    BSOD_EXIT_REBOOT = 0, /*!< 蓝屏结束后重启系统（默认） */
    BSOD_EXIT_RESTORE,    /*!< 蓝屏结束后恢复桌面并退出程序 */
};

/**
 * @brief 设置蓝屏结束后的行为（重启 或 恢复退出）
 * @param mode 退出模式
 */
void bsod_set_exit_mode(enum bsod_exit_mode mode);

/**
 * @brief 设置蓝屏外观主题（颜色 / 终止代码 / 自定义字符串 / 彩虹 / VGA 模式）
 *
 * 主题会整体复制进蓝屏流程；不调用时使用默认主题（经典 Windows 10 蓝屏）。
 * 应在 bsod_show() / monitor_run() 之前调用。
 *
 * @param theme 主题指针（可随后释放，内部会复制）
 */
void bsod_set_theme(const struct bsod_theme *theme);

/**
 * @brief 显示一次完整蓝屏
 *
 * 流程：切到空闲 VT -> 打开 DRM 并抢占 Master -> 查找显示器 -> 创建帧缓冲 ->
 * 渲染内容（含原因）-> 推屏 -> 进度动画到 100% -> 延迟 2 秒 ->
 * 按 bsod_set_exit_mode() 设置的模式执行（默认重启系统，或恢复并退出程序）。
 *
 * @param reason 蓝屏原因字符串（用于终止代码、帮助链接与二维码）
 * @return 成功返回 0，失败返回 -1
 */
int bsod_show(const char *reason);

#endif /* BSOD_H */
