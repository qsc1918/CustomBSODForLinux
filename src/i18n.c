#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "i18n.h"

/* ============ English（默认） ============ */
static const char *const pack_en[MSG_COUNT] = {
    [MSG_STEP_OPEN_TTY] = "Opening /dev/tty0 and getting the active VT...",
    [MSG_DESKTOP_ON_TTY] = "Desktop is running on tty%d\n",
    [MSG_STEP_SWITCH_VT] = "Switching to tty%d to suspend the desktop and release DRM...",
    [MSG_STEP_OPEN_DRM] = "Opening DRM device (auto-detect /dev/dri/card*)...",
    [MSG_STEP_SET_MASTER] = "Acquiring DRM Master...",
    [MSG_STEP_RESOLUTION] = "Native resolution: %ux%u",
    [MSG_STEP_FILL_FB] = "Filling blue screen pixels...",
    [MSG_STEP_PUSH_DONE] = "Blue screen shown! Will reboot after reaching 100%...",
    [MSG_AFTER_DELAY] = "[bsod] Progress done, in %d second(s): %s...\n",
    [MSG_ACTION_REBOOT] = "reboot system",
    [MSG_ACTION_RESTORE] = "restore and exit",
    [MSG_REBOOT_FAILED] = "[bsod] reboot failed, falling back to restore and exit",
    [MSG_CLEANUP] = "\nCleaning up DRM resources and restoring the terminal environment...\n",
    [MSG_SWITCH_BACK] = "Switching back to the desktop (tty%d)...\n",
    [MSG_RESTORED] = "Desktop restored.\n",

    [MSG_RENDER_TITLE] = "Your PC ran into a problem and needs to restart.",
    [MSG_RENDER_SUB] = "We're just collecting some error info, and then you can restart.",
    [MSG_RENDER_PERCENT] = "%d%% complete",
    [MSG_RENDER_PERCENT_100] = "100% complete",
    [MSG_RENDER_VISIT] = "For more info about this issue and possible fixes, visit",
    [MSG_RENDER_HELP] = "If you call a support person, give them this info:",
    [MSG_RENDER_STOP] = "Stop code: %s",

    [MSG_MON_START] = "[monitor] Watching system logs; on error: %s...\n",
    [MSG_MON_DRYRUN] = "print reason (dry-run)",
    [MSG_MON_SHOW_ACT] = "show blue screen",
    [MSG_MON_DETECTED] = "\n[monitor] System error detected: %s\n",
    [MSG_MON_SHOWING] = "[monitor] Showing blue screen...\n",
    [MSG_MON_END] = "[monitor] Blue screen finished, exiting.\n",
    [MSG_MON_OPEN_FAIL] = "sd_journal_open failed: %s\n",
    [MSG_MON_NEXT_FAIL] = "sd_journal_next failed: %s\n",
    [MSG_MON_WAIT_FAIL] = "sd_journal_wait failed: %s\n",

    [MSG_USAGE] = "Usage: %s [options]\n"
                  "\n"
                  "Modes (pick one):\n"
                  "  --monitor          Watch system logs in foreground, show blue screen on error\n"
                  "  --daemon           Run as a background daemon monitor\n"
                  "  --show \"reason\"   Show the blue screen once (manual BugCheck trigger)\n"
                  "  --dry-run          Monitor but only print detected errors (debug)\n"
                  "\n"
                  "Customization (ported from CustomBSOD):\n"
                  "  --config FILE      Load configuration from FILE (default: /etc/bsod.conf,\n"
                  "                     ~/.config/bsod/bsod.conf)\n"
                  "  --bg COLOR         Background color (#RRGGBB / 0xRRGGBB / RRGGBB)\n"
                  "  --fg COLOR         Foreground/text color\n"
                  "  --text-bg COLOR    Text background color (box behind text; default = background)\n"
                  "  --stop CODE        Custom stop code text shown on the blue screen\n"
                  "  --string SPEC      Add a custom string: text|x|y|size|color (repeatable)\n"
                  "  --rainbow          Animated rainbow background (color cycles)\n"
                  "  --rainbow-speed N  Rainbow color change speed, degrees/sec (default 100)\n"
                  "  --vga              Classic Windows 7 VGA text-mode blue screen\n"
                  "  --vga-cols N       Classic mode columns (default 80)\n"
                  "  --vga-rows N       Classic mode rows, 25 or 50 (default 25)\n"
                  "  --blink            Classic mode: blink the stop-code line\n"
                  "\n"
                  "Options:\n"
                  "  --restore          Restore the desktop and exit after the blue screen (default: reboot)\n"
                  "  --help, -h         Show this help\n"
                  "\n"
                  "Shows this help when run without arguments.\n",
    [MSG_UNKNOWN_OPT] = "Unknown option: %s\n",
    [MSG_NEED_ROOT] = "Error: this program requires root to control VT and DRM!\nRun as: sudo bsod\n",
    [MSG_DAEMON_FAIL] = "Failed to daemonize",
    [MSG_THEME_OPT_BAD] = "Invalid theme option or argument: %s\n",
    [MSG_CONFIG_LOADED] = "[bsod] Loaded configuration from %s\n",
    [MSG_CONFIG_LOAD_FAIL] = "[bsod] Failed to read configuration: %s\n",
};

/* ============ 简体中文 ============ */
static const char *const pack_zh_cn[MSG_COUNT] = {
    [MSG_STEP_OPEN_TTY] = "正在打开 /dev/tty0 并获取当前活动 VT...",
    [MSG_DESKTOP_ON_TTY] = "检测到当前桌面运行在 tty%d\n",
    [MSG_STEP_SWITCH_VT] = "正在切换至 tty%d 以让图形界面挂起并释放 DRM 权限...",
    [MSG_STEP_OPEN_DRM] = "正在打开 DRM 设备（自动探测 /dev/dri/card*）...",
    [MSG_STEP_SET_MASTER] = "正在抢占 DRM Master 控制权...",
    [MSG_STEP_RESOLUTION] = "成功获取物理原生分辨率: %ux%u",
    [MSG_STEP_FILL_FB] = "填充蓝屏像素数据...",
    [MSG_STEP_PUSH_DONE] = "蓝屏推送成功！进度到 100% 后将自动重启...",
    [MSG_AFTER_DELAY] = "[bsod] 进度完成，%d 秒后%s...\n",
    [MSG_ACTION_REBOOT] = "重启系统",
    [MSG_ACTION_RESTORE] = "恢复并退出",
    [MSG_REBOOT_FAILED] = "[bsod] reboot 失败，改为恢复并退出",
    [MSG_CLEANUP] = "\n正在清理 DRM 资源并恢复原本的终端环境...\n",
    [MSG_SWITCH_BACK] = "正在自动切回原桌面 (tty%d)...\n",
    [MSG_RESTORED] = "桌面恢复完成。\n",

    [MSG_RENDER_TITLE] = "你的电脑遇到问题，需要重启。",
    [MSG_RENDER_SUB] = "我们只展示某些错误信息，然后你可以重新启动。",
    [MSG_RENDER_PERCENT] = "%d%% 完成",
    [MSG_RENDER_PERCENT_100] = "100% 完成",
    [MSG_RENDER_VISIT] = "有关此问题的详细信息和可能的解决方法，请访问",
    [MSG_RENDER_HELP] = "如果询问技术人员，请向他们提供以下信息",
    [MSG_RENDER_STOP] = "终止代码：%s",

    [MSG_MON_START] = "[monitor] 正在实时监控系统日志，检测到错误时%s...\n",
    [MSG_MON_DRYRUN] = "打印原因（dry-run）",
    [MSG_MON_SHOW_ACT] = "显示蓝屏",
    [MSG_MON_DETECTED] = "\n[monitor] 检测到系统错误: %s\n",
    [MSG_MON_SHOWING] = "[monitor] 显示蓝屏...\n",
    [MSG_MON_END] = "[monitor] 蓝屏已结束，退出。\n",
    [MSG_MON_OPEN_FAIL] = "sd_journal_open 失败: %s\n",
    [MSG_MON_NEXT_FAIL] = "sd_journal_next 失败: %s\n",
    [MSG_MON_WAIT_FAIL] = "sd_journal_wait 失败: %s\n",

    [MSG_USAGE] = "用法: %s [选项]\n"
                  "\n"
                  "模式（至少指定其一）：\n"
                  "  --monitor          前台监控系统日志，错误时显示蓝屏\n"
                  "  --daemon           守护进程方式后台监控\n"
                  "  --show \"原因\"     一次性显示蓝屏（对应主动 BugCheck 触发）\n"
                  "  --dry-run          监控但只打印检测到的错误，不显示蓝屏（调试）\n"
                  "\n"
                  "外观自定义（移植自 CustomBSOD）：\n"
                  "  --config FILE      从 FILE 加载配置（默认 /etc/bsod.conf、\n"
                  "                     ~/.config/bsod/bsod.conf）\n"
                  "  --bg COLOR         背景颜色（#RRGGBB / 0xRRGGBB / RRGGBB）\n"
                  "  --fg COLOR         前景/文字颜色\n"
                  "  --text-bg COLOR    文字背景色（文字后色块；默认等于背景即不显示）\n"
                  "  --stop CODE        自定义终止代码文本\n"
                  "  --string SPEC      添加自定义字符串: 文本|x|y|字号|颜色（可重复）\n"
                  "  --rainbow          彩虹蓝屏（背景颜色持续循环变化）\n"
                  "  --rainbow-speed N  彩虹颜色变化速度（度/秒，默认 100）\n"
                  "  --vga              经典 Windows 7 VGA 文本模式蓝屏\n"
                  "  --vga-cols N       经典模式列数（默认 80）\n"
                  "  --vga-rows N       经典模式行数，25 或 50（默认 25）\n"
                  "  --blink            经典模式：终止代码行闪烁\n"
                  "\n"
                  "选项：\n"
                  "  --restore          蓝屏结束后恢复桌面并退出程序（默认是重启系统）\n"
                  "  --help, -h         显示本帮助\n"
                  "\n"
                  "无参数时显示本帮助。\n",
    [MSG_UNKNOWN_OPT] = "未知选项: %s\n",
    [MSG_NEED_ROOT] = "错误: 该程序需要 root 权限操作 VT 与 DRM！\n请使用: sudo bsod\n",
    [MSG_DAEMON_FAIL] = "守护进程化失败",
    [MSG_THEME_OPT_BAD] = "无效的主题选项或参数: %s\n",
    [MSG_CONFIG_LOADED] = "[bsod] 已从 %s 加载配置\n",
    [MSG_CONFIG_LOAD_FAIL] = "[bsod] 无法读取配置文件: %s\n",
};

/* ============ 繁體中文 ============ */
static const char *const pack_zh_tw[MSG_COUNT] = {
    [MSG_STEP_OPEN_TTY] = "正在開啟 /dev/tty0 並取得目前使用的 VT...",
    [MSG_DESKTOP_ON_TTY] = "偵測到目前桌面執行於 tty%d\n",
    [MSG_STEP_SWITCH_VT] = "正在切換至 tty%d 以讓圖形介面掛起並釋放 DRM 權限...",
    [MSG_STEP_OPEN_DRM] = "正在開啟 DRM 裝置（自動偵測 /dev/dri/card*）...",
    [MSG_STEP_SET_MASTER] = "正在取得 DRM Master 控制權...",
    [MSG_STEP_RESOLUTION] = "成功取得實體原生解析度: %ux%u",
    [MSG_STEP_FILL_FB] = "填入藍屏像素資料...",
    [MSG_STEP_PUSH_DONE] = "藍屏推送成功！進度到 100% 後將自動重新啟動...",
    [MSG_AFTER_DELAY] = "[bsod] 進度完成，%d 秒後%s...\n",
    [MSG_ACTION_REBOOT] = "重新啟動系統",
    [MSG_ACTION_RESTORE] = "恢復並退出",
    [MSG_REBOOT_FAILED] = "[bsod] reboot 失敗，改為恢復並退出",
    [MSG_CLEANUP] = "\n正在清理 DRM 資源並恢復原本的終端環境...\n",
    [MSG_SWITCH_BACK] = "正在自動切回原桌面 (tty%d)...\n",
    [MSG_RESTORED] = "桌面恢復完成。\n",

    [MSG_RENDER_TITLE] = "您的電腦發生問題，需要重新啟動。",
    [MSG_RENDER_SUB] = "我們只是收集一些錯誤資訊，然後您可以重新啟動。",
    [MSG_RENDER_PERCENT] = "%d%% 完成",
    [MSG_RENDER_PERCENT_100] = "100% 完成",
    [MSG_RENDER_VISIT] = "如需此問題的詳細資訊和可能的解決方法，請瀏覽",
    [MSG_RENDER_HELP] = "如果您需要聯絡技術支援，請向他們提供此資訊：",
    [MSG_RENDER_STOP] = "終止代碼：%s",

    [MSG_MON_START] = "[monitor] 正在即時監控系統日誌，偵測到錯誤時%s...\n",
    [MSG_MON_DRYRUN] = "列印原因（dry-run）",
    [MSG_MON_SHOW_ACT] = "顯示藍屏",
    [MSG_MON_DETECTED] = "\n[monitor] 偵測到系統錯誤: %s\n",
    [MSG_MON_SHOWING] = "[monitor] 顯示藍屏...\n",
    [MSG_MON_END] = "[monitor] 藍屏已結束，退出。\n",
    [MSG_MON_OPEN_FAIL] = "sd_journal_open 失敗: %s\n",
    [MSG_MON_NEXT_FAIL] = "sd_journal_next 失敗: %s\n",
    [MSG_MON_WAIT_FAIL] = "sd_journal_wait 失敗: %s\n",

    [MSG_USAGE] = "用法: %s [選項]\n"
                  "\n"
                  "模式（至少指定其一）：\n"
                  "  --monitor          前景監控系統日誌，錯誤時顯示藍屏\n"
                  "  --daemon           以守護行程方式於背景監控\n"
                  "  --show \"原因\"     一次顯示藍屏（對應主動 BugCheck 觸發）\n"
                  "  --dry-run          監控但只列印偵測到的錯誤，不顯示藍屏（除錯）\n"
                  "\n"
                  "外觀自訂（移植自 CustomBSOD）：\n"
                  "  --config FILE      從 FILE 載入設定（預設 /etc/bsod.conf、\n"
                  "                     ~/.config/bsod/bsod.conf）\n"
                  "  --bg COLOR         背景顏色（#RRGGBB / 0xRRGGBB / RRGGBB）\n"
                  "  --fg COLOR         前景/文字顏色\n"
                  "  --text-bg COLOR    文字背景色（文字後色塊；預設等於背景即不顯示）\n"
                  "  --stop CODE        自訂終止代碼文字\n"
                  "  --string SPEC      新增自訂字串: 文字|x|y|字號|顏色（可重複）\n"
                  "  --rainbow          彩虹藍屏（背景顏色持續循環變化）\n"
                  "  --rainbow-speed N  彩虹顏色變化速度（度/秒，預設 100）\n"
                  "  --vga              經典 Windows 7 VGA 文字模式藍屏\n"
                  "  --vga-cols N       經典模式列數（預設 80）\n"
                  "  --vga-rows N       經典模式行數，25 或 50（預設 25）\n"
                  "  --blink            經典模式：終止代碼行閃爍\n"
                  "\n"
                  "選項：\n"
                  "  --restore          藍屏結束後恢復桌面並退出程式（預設為重新啟動系統）\n"
                  "  --help, -h         顯示本說明\n"
                  "\n"
                  "無參數時顯示本說明。\n",
    [MSG_UNKNOWN_OPT] = "未知選項: %s\n",
    [MSG_NEED_ROOT] = "錯誤: 此程式需要 root 權限才能操作 VT 與 DRM！\n請使用: sudo bsod\n",
    [MSG_DAEMON_FAIL] = "守護行程化失敗",
    [MSG_THEME_OPT_BAD] = "無效的主題選項或參數: %s\n",
    [MSG_CONFIG_LOADED] = "[bsod] 已從 %s 載入設定\n",
    [MSG_CONFIG_LOAD_FAIL] = "[bsod] 無法讀取設定檔: %s\n",
};

/* ============ 日本語 ============ */
static const char *const pack_ja[MSG_COUNT] = {
    [MSG_STEP_OPEN_TTY] = "/dev/tty0 を開き、現在の VT を取得しています...",
    [MSG_DESKTOP_ON_TTY] = "デスクトップは tty%d で動作しています\n",
    [MSG_STEP_SWITCH_VT] = "tty%d に切り替えて、デスクトップを停止し DRM を解放しています...",
    [MSG_STEP_OPEN_DRM] = "DRM デバイスを開いています（/dev/dri/card* を自動検出）...",
    [MSG_STEP_SET_MASTER] = "DRM Master を取得しています...",
    [MSG_STEP_RESOLUTION] = "ネイティブ解像度: %ux%u",
    [MSG_STEP_FILL_FB] = "ブルースクリーンのピクセルを塗りつぶしています...",
    [MSG_STEP_PUSH_DONE] = "ブルースクリーンを表示しました！100% に達すると再起動します...",
    [MSG_AFTER_DELAY] = "[bsod] 進捗完了、%d 秒後に%s...\n",
    [MSG_ACTION_REBOOT] = "システムを再起動",
    [MSG_ACTION_RESTORE] = "復元して終了",
    [MSG_REBOOT_FAILED] = "[bsod] reboot に失敗したため、復元して終了します",
    [MSG_CLEANUP] = "\nDRM リソースを解放し、ターミナル環境を復元しています...\n",
    [MSG_SWITCH_BACK] = "デスクトップ (tty%d) に戻しています...\n",
    [MSG_RESTORED] = "デスクトップを復元しました。\n",

    [MSG_RENDER_TITLE] = "お使いの PC に問題が発生したため、再起動する必要があります。",
    [MSG_RENDER_SUB] = "エラー情報を収集しています。その後、再起動できます。",
    [MSG_RENDER_PERCENT] = "%d%% 完了",
    [MSG_RENDER_PERCENT_100] = "100% 完了",
    [MSG_RENDER_VISIT] = "この問題の詳細と可能な解決策については、次を参照してください。",
    [MSG_RENDER_HELP] = "サポート担当者に問い合わせる場合は、次の情報を提供してください：",
    [MSG_RENDER_STOP] = "停止コード: %s",

    [MSG_MON_START] = "[monitor] システムログを監視中、エラー時に%s...\n",
    [MSG_MON_DRYRUN] = "原因を出力（dry-run）",
    [MSG_MON_SHOW_ACT] = "ブルースクリーンを表示",
    [MSG_MON_DETECTED] = "\n[monitor] システムエラーを検出: %s\n",
    [MSG_MON_SHOWING] = "[monitor] ブルースクリーンを表示中...\n",
    [MSG_MON_END] = "[monitor] ブルースクリーン終了、退出します。\n",
    [MSG_MON_OPEN_FAIL] = "sd_journal_open に失敗: %s\n",
    [MSG_MON_NEXT_FAIL] = "sd_journal_next に失敗: %s\n",
    [MSG_MON_WAIT_FAIL] = "sd_journal_wait に失敗: %s\n",

    [MSG_USAGE] = "使用方法: %s [オプション]\n"
                  "\n"
                  "モード（いずれか1つを指定）：\n"
                  "  --monitor          フォアグラウンドでシステムログを監視し、エラー時にブルースクリーンを表示\n"
                  "  --daemon           バックグラウンドデーモンとして監視\n"
                  "  --show \"原因\"      ブルースクリーンを一度だけ表示（手動 BugCheck 相当）\n"
                  "  --dry-run          監視するが、検出したエラーのみを出力（デバッグ）\n"
                  "\n"
                  "外観カスタマイズ（CustomBSOD から移植）:\n"
                  "  --config FILE      FILE から設定を読み込み（既定: /etc/bsod.conf、\n"
                  "                     ~/.config/bsod/bsod.conf）\n"
                  "  --bg COLOR         背景色（#RRGGBB / 0xRRGGBB / RRGGBB）\n"
                  "  --fg COLOR         前景/文字色\n"
                  "  --text-bg COLOR    文字背景色（文字の後ろのボックス；既定は背景色）\n"
                  "  --stop CODE        表示するカスタム停止コード\n"
                  "  --string SPEC      カスタム文字列を追加: テキスト|x|y|サイズ|色（繰り返し可）\n"
                  "  --rainbow          虹色ブルースクリーン（背景色が循環変化）\n"
                  "  --rainbow-speed N  色相変化速度（度/秒、既定 100）\n"
                  "  --vga              クラシック Windows 7 VGA テキストモードのブルースクリーン\n"
                  "  --vga-cols N       クラシックモードの列数（既定 80）\n"
                  "  --vga-rows N       クラシックモードの行数、25 または 50（既定 25）\n"
                  "  --blink            クラシックモード: 停止コード行を点滅\n"
                  "\n"
                  "オプション：\n"
                  "  --restore          ブルースクリーン後にデスクトップを復元して終了（既定は再起動）\n"
                  "  --help, -h         このヘルプを表示\n"
                  "\n"
                  "引数なしで実行するとこのヘルプを表示します。\n",
    [MSG_UNKNOWN_OPT] = "不明なオプション: %s\n",
    [MSG_NEED_ROOT] = "エラー: このプログラムは VT と DRM の操作に root 権限が必要です！\nsudo bsod として実行してください\n",
    [MSG_DAEMON_FAIL] = "デーモン化に失敗しました",
    [MSG_THEME_OPT_BAD] = "テーマのオプションまたは引数が無効です: %s\n",
    [MSG_CONFIG_LOADED] = "[bsod] %s から設定を読み込みました\n",
    [MSG_CONFIG_LOAD_FAIL] = "[bsod] 設定ファイルを読み込めませんでした: %s\n",
};

/* ============ 한국어 ============ */
static const char *const pack_ko[MSG_COUNT] = {
    [MSG_STEP_OPEN_TTY] = "/dev/tty0을 열고 현재 VT를 가져오는 중...",
    [MSG_DESKTOP_ON_TTY] = "데스크톱이 tty%d에서 실행 중입니다\n",
    [MSG_STEP_SWITCH_VT] = "tty%d로 전환하여 데스크톱을 중지하고 DRM을 해제하는 중...",
    [MSG_STEP_OPEN_DRM] = "DRM 장치 여는 중（/dev/dri/card* 자동 감지）...",
    [MSG_STEP_SET_MASTER] = "DRM Master를 획득하는 중...",
    [MSG_STEP_RESOLUTION] = "기본 해상도: %ux%u",
    [MSG_STEP_FILL_FB] = "블루 스크린 픽셀을 채우는 중...",
    [MSG_STEP_PUSH_DONE] = "블루 스크린 표시 완료! 100%에 도달하면 재부팅합니다...",
    [MSG_AFTER_DELAY] = "[bsod] 진행 완료, %d초 후 %s...\n",
    [MSG_ACTION_REBOOT] = "시스템 재부팅",
    [MSG_ACTION_RESTORE] = "복원 후 종료",
    [MSG_REBOOT_FAILED] = "[bsod] reboot 실패, 복원 후 종료로 전환합니다",
    [MSG_CLEANUP] = "\nDRM 리소스를 정리하고 터미널 환경을 복원하는 중...\n",
    [MSG_SWITCH_BACK] = "데스크톱(tty%d)으로 돌아가는 중...\n",
    [MSG_RESTORED] = "데스크톱 복원 완료.\n",

    [MSG_RENDER_TITLE] = "PC에 문제가 발생하여 다시 시작해야 합니다.",
    [MSG_RENDER_SUB] = "오류 정보를 수집한 다음 다시 시작할 수 있습니다.",
    [MSG_RENDER_PERCENT] = "%d%% 완료",
    [MSG_RENDER_PERCENT_100] = "100% 완료",
    [MSG_RENDER_VISIT] = "이 문제에 대한 자세한 내용과 가능한 해결 방법은 다음을 참조하세요.",
    [MSG_RENDER_HELP] = "지원 담당자에게 문의하는 경우 다음 정보를 제공하세요:",
    [MSG_RENDER_STOP] = "중지 코드: %s",

    [MSG_MON_START] = "[monitor] 시스템 로그를 감시 중, 오류 시 %s...\n",
    [MSG_MON_DRYRUN] = "원인 출력(dry-run)",
    [MSG_MON_SHOW_ACT] = "블루 스크린 표시",
    [MSG_MON_DETECTED] = "\n[monitor] 시스템 오류 감지: %s\n",
    [MSG_MON_SHOWING] = "[monitor] 블루 스크린을 표시하는 중...\n",
    [MSG_MON_END] = "[monitor] 블루 스크린 종료, 프로그램 종료.\n",
    [MSG_MON_OPEN_FAIL] = "sd_journal_open 실패: %s\n",
    [MSG_MON_NEXT_FAIL] = "sd_journal_next 실패: %s\n",
    [MSG_MON_WAIT_FAIL] = "sd_journal_wait 실패: %s\n",

    [MSG_USAGE] = "사용법: %s [옵션]\n"
                  "\n"
                  "모드(하나를 선택):\n"
                  "  --monitor          포그라운드에서 시스템 로그를 감시하고 오류 시 블루 스크린 표시\n"
                  "  --daemon           백그라운드 데몬으로 감시\n"
                  "  --show \"원인\"      블루 스크린을 한 번 표시(수동 BugCheck 트리거)\n"
                  "  --dry-run          감시만 하고 감지된 오류만 출력(디버그)\n"
                  "\n"
                  "외관 사용자 지정(CustomBSOD에서 이식):\n"
                  "  --config FILE      FILE에서 설정 로드(기본: /etc/bsod.conf,\n"
                  "                     ~/.config/bsod/bsod.conf)\n"
                  "  --bg COLOR         배경색(#RRGGBB / 0xRRGGBB / RRGGBB)\n"
                  "  --fg COLOR         전경/문자색\n"
                  "  --text-bg COLOR    문자 배경색(문자 뒤 박스; 기본은 배경색)\n"
                  "  --stop CODE        표시할 사용자 지정 중지 코드\n"
                  "  --string SPEC      사용자 지정 문자열 추가: 텍스트|x|y|크기|색상(반복 가능)\n"
                  "  --rainbow          무지개 블루 스크린(배경색이 계속 변화)\n"
                  "  --rainbow-speed N  색상 변화 속도(도/초, 기본 100)\n"
                  "  --vga              클래식 Windows 7 VGA 텍스트 모드 블루 스크린\n"
                  "  --vga-cols N       클래식 모드 열 수(기본 80)\n"
                  "  --vga-rows N       클래식 모드 행 수, 25 또는 50(기본 25)\n"
                  "  --blink            클래식 모드: 중지 코드 줄 깜빡임\n"
                  "\n"
                  "옵션:\n"
                  "  --restore          블루 스크린 후 데스크톱을 복원하고 종료(기본은 재부팅)\n"
                  "  --help, -h         도움말 표시\n"
                  "\n"
                  "인수 없이 실행하면 이 도움말을 표시합니다.\n",
    [MSG_UNKNOWN_OPT] = "알 수 없는 옵션: %s\n",
    [MSG_NEED_ROOT] = "오류: 이 프로그램은 VT 및 DRM 제어에 root 권한이 필요합니다!\nsudo bsod로 실행하세요\n",
    [MSG_DAEMON_FAIL] = "데몬화 실패",
    [MSG_THEME_OPT_BAD] = "테마 옵션 또는 인수가 잘못되었습니다: %s\n",
    [MSG_CONFIG_LOADED] = "[bsod] %s 에서 설정을 불러왔습니다\n",
    [MSG_CONFIG_LOAD_FAIL] = "[bsod] 설정 파일을 읽을 수 없습니다: %s\n",
};

static const char *const *const packs[I18N_LANG_COUNT] = {
    [I18N_EN] = pack_en,
    [I18N_ZH_CN] = pack_zh_cn,
    [I18N_ZH_TW] = pack_zh_tw,
    [I18N_JA] = pack_ja,
    [I18N_KO] = pack_ko,
};

static const char *const lang_codes[I18N_LANG_COUNT] = {
    [I18N_EN] = "en",
    [I18N_ZH_CN] = "zh-CN",
    [I18N_ZH_TW] = "zh-TW",
    [I18N_JA] = "ja",
    [I18N_KO] = "ko",
};

static int g_inited = 0;
static enum i18n_lang g_lang = I18N_EN;

/* 按 POSIX 规范优先级读取本地化环境变量：LC_ALL > LC_MESSAGES > LANG */
static const char *lang_env(void)
{
    const char *v;

    v = getenv("LC_ALL");
    if (v && v[0])
        return v;
    v = getenv("LC_MESSAGES");
    if (v && v[0])
        return v;
    v = getenv("LANG");
    if (v && v[0])
        return v;
    return NULL;
}

int i18n_init(void)
{
    const char *v = lang_env();

    g_lang = I18N_EN;
    if (v && strncasecmp(v, "zh", 2) == 0)
    {
        /* 繁体：TW/HK/MO 地区或 Hant 脚本 */
        if (strcasestr(v, "TW") || strcasestr(v, "HK") ||
            strcasestr(v, "MO") || strcasestr(v, "Hant"))
            g_lang = I18N_ZH_TW;
        else
            g_lang = I18N_ZH_CN; /* zh / zh_CN / zh_SG / Hans... */
    }
    else if (v && strncasecmp(v, "ja", 2) == 0)
        g_lang = I18N_JA;
    else if (v && strncasecmp(v, "ko", 2) == 0)
        g_lang = I18N_KO;
    g_inited = 1;
    return 0;
}

const char *i18n_tr(enum i18n_msg id)
{
    const char *s;

    if (!g_inited)
        i18n_init();
    if (id < 0 || id >= MSG_COUNT)
        return "";
    s = packs[g_lang][id];
    return s ? s : "";
}

const char *i18n_lang_code(void)
{
    if (!g_inited)
        i18n_init();
    return lang_codes[g_lang];
}
