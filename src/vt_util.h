#ifndef VT_UTIL_H
#define VT_UTIL_H

/**
 * @file vt_util.h
 * @brief 虚拟终端 (VT) 工具函数
 *
 * 用于获取当前活动 VT、切换 VT，从而在显示蓝屏前
 * 将图形界面 (Wayland/X11) 挂起并释放其 DRM Master 独占权。
 */

/**
 * @brief 打开 /dev/tty0
 * @return 成功返回文件描述符，失败返回 -1
 */
int vt_open_tty(void);

/**
 * @brief 获取当前活动的 VT 编号
 * @param tty_fd /dev/tty0 的文件描述符
 * @return 成功返回 VT 编号（>= 1），失败返回 -1
 */
int vt_get_active(int tty_fd);

/**
 * @brief 切换到指定的 VT 并等待切换完成
 * @param tty_fd /dev/tty0 的文件描述符
 * @param vt     目标 VT 编号
 * @return 成功返回 0，失败返回 -1
 */
int vt_switch_to(int tty_fd, int vt);

#endif /* VT_UTIL_H */
