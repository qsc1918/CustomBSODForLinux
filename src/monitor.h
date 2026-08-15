#ifndef MONITOR_H
#define MONITOR_H

/**
 * @file monitor.h
 * @brief 系统日志监控（journald 实时跟随）
 */

/**
 * @brief 监控系统日志，检测到错误时显示蓝屏并执行退出动作（阻塞运行）
 *
 * 通过 sd-journal 实时跟随 journald。当某条日志的优先级 <= err(3)
 * 或消息含失败特征（failed/error/main process exited 等）时，
 * 生成一行原因并调用 bsod_show()；蓝屏结束按 bsod_set_exit_mode() 设置的模式
 * （默认重启系统，或恢复并退出）执行，随后本程序退出。
 *
 * @param dry_run 为真时只打印检测到的原因而不显示蓝屏（调试用）
 * @return 成功返回 0，失败返回 -1
 */
int monitor_run(int dry_run);

#endif /* MONITOR_H */
