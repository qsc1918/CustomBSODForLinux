#ifndef I18N_H
#define I18N_H

/**
 * @file i18n.h
 * @brief 内置语言包（国际化）
 *
 * 程序内置 5 种语言包（编译期内置，不依赖系统 locale 包），
 * 按 POSIX 规范读取标准本地化环境变量选择语言
 * （优先级 LC_ALL > LC_MESSAGES > LANG，如 LANG=zh_CN.UTF-8），
 * 未设置或无法识别时默认英文。
 */

/** @brief 支持的语言 */
enum i18n_lang
{
    I18N_EN = 0, /*!< English（默认） */
    I18N_ZH_CN,  /*!< 简体中文 */
    I18N_ZH_TW,  /*!< 繁體中文 */
    I18N_JA,     /*!< 日本語 */
    I18N_KO,     /*!< 한국어 */
    I18N_LANG_COUNT,
};

/** @brief 消息 ID（每种语言包都提供这些条目） */
enum i18n_msg
{
    /* bsod.c 流程与状态 */
    MSG_STEP_OPEN_TTY,
    MSG_DESKTOP_ON_TTY,
    MSG_STEP_SWITCH_VT,
    MSG_STEP_OPEN_DRM,
    MSG_STEP_SET_MASTER,
    MSG_STEP_RESOLUTION,
    MSG_STEP_FILL_FB,
    MSG_STEP_PUSH_DONE,
    MSG_AFTER_DELAY,
    MSG_ACTION_REBOOT,
    MSG_ACTION_RESTORE,
    MSG_REBOOT_FAILED,
    MSG_CLEANUP,
    MSG_SWITCH_BACK,
    MSG_RESTORED,

    /* 蓝屏画面内容 */
    MSG_RENDER_TITLE,
    MSG_RENDER_SUB,
    MSG_RENDER_PERCENT,
    MSG_RENDER_PERCENT_100,
    MSG_RENDER_VISIT,
    MSG_RENDER_HELP,
    MSG_RENDER_STOP,

    /* monitor */
    MSG_MON_START,
    MSG_MON_DRYRUN,
    MSG_MON_SHOW_ACT,
    MSG_MON_DETECTED,
    MSG_MON_SHOWING,
    MSG_MON_END,
    MSG_MON_OPEN_FAIL,
    MSG_MON_NEXT_FAIL,
    MSG_MON_WAIT_FAIL,

    /* main */
    MSG_USAGE,
    MSG_UNKNOWN_OPT,
    MSG_NEED_ROOT,
    MSG_DAEMON_FAIL,
    MSG_THEME_OPT_BAD,
    MSG_CONFIG_LOADED,
    MSG_CONFIG_LOAD_FAIL,

    MSG_COUNT
};

/**
 * @brief 初始化语言包（读取标准 locale 环境变量 LC_ALL/LC_MESSAGES/LANG；
 * 可重复调用，未初始化时 i18n_tr 会自动调用）
 * @return 成功返回 0
 */
int i18n_init(void);

/**
 * @brief 取当前语言下某条消息的文本（可能含 printf 格式占位符）
 * @param id 消息 ID
 * @return 文本字符串（缺失或非法 ID 返回空串）
 */
const char *i18n_tr(enum i18n_msg id);

/** @brief 当前语言代码，如 "en" / "zh-CN" */
const char *i18n_lang_code(void);

#endif /* I18N_H */
