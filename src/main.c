#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

#include "bsod.h"
#include "monitor.h"
#include "i18n.h"

static void usage(const char *prog)
{
    fprintf(stderr, i18n_tr(MSG_USAGE), prog);
}

/* 守护进程化：fork + setsid，标准流重定向到 /dev/null */
static int daemonize(void)
{
    pid_t pid = fork();
    int fd;

    if (pid < 0)
        return -1;
    if (pid > 0)
        _exit(0); /* 父进程退出 */

    if (setsid() < 0)
        return -1;

    fd = open("/dev/null", O_RDWR);
    if (fd >= 0)
    {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > STDERR_FILENO)
            close(fd);
    }
    return 0;
}

/* 加载配置文件到主题：
 *   --config FILE 显式指定（必须存在）；
 *   否则依次尝试 /etc/bsod.conf 与 ~/.config/bsod/bsod.conf（可选，缺失忽略）。 */
static void load_theme_config(struct bsod_theme *theme, const char *config_path)
{
    const char *xdg, *home;
    char path[1024];

    if (config_path)
    {
        if (bsod_theme_load_file(theme, config_path) != 0)
        {
            fprintf(stderr, i18n_tr(MSG_CONFIG_LOAD_FAIL), config_path);
            exit(1);
        }
        fprintf(stderr, i18n_tr(MSG_CONFIG_LOADED), config_path);
        return;
    }

    if (access("/etc/bsod.conf", R_OK) == 0)
    {
        if (bsod_theme_load_file(theme, "/etc/bsod.conf") == 0)
            fprintf(stderr, i18n_tr(MSG_CONFIG_LOADED), "/etc/bsod.conf");
    }

    xdg = getenv("XDG_CONFIG_HOME");
    home = getenv("HOME");
    path[0] = '\0';
    if (xdg && xdg[0])
        snprintf(path, sizeof(path), "%s/bsod/bsod.conf", xdg);
    else if (home && home[0])
        snprintf(path, sizeof(path), "%s/.config/bsod/bsod.conf", home);
    if (path[0] && access(path, R_OK) == 0)
    {
        if (bsod_theme_load_file(theme, path) == 0)
            fprintf(stderr, i18n_tr(MSG_CONFIG_LOADED), path);
    }
}

int main(int argc, char **argv)
{
    struct bsod_theme theme;
    const char *config_path = NULL;
    int monitor = 0;
    int daemon = 0;
    int dry = 0;
    int restore = 0;
    const char *reason = NULL;
    int i;

    bsod_theme_default(&theme);

    /* 第一遍：仅查找 --config 路径，随后先加载配置文件再应用命令行覆盖 */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc)
        {
            config_path = argv[++i];
            break;
        }
    }
    load_theme_config(&theme, config_path);

    /* 第二遍：解析主题选项与主选项（命令行优先级高于配置文件） */
    for (i = 1; i < argc; i++)
    {
        const char *optname;
        int t;

        if (strcmp(argv[i], "--config") == 0)
        {
            if (i + 1 < argc)
                i++;
            continue;
        }

        optname = argv[i];
        t = bsod_theme_parse_arg(&theme, argc, argv, &i);
        if (t < 0)
        {
            fprintf(stderr, i18n_tr(MSG_THEME_OPT_BAD), optname);
            usage(argv[0]);
            return 2;
        }
        if (t > 0)
            continue;

        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            usage(argv[0]);
            return 0;
        }
        else if (strcmp(argv[i], "--monitor") == 0)
            monitor = 1;
        else if (strcmp(argv[i], "--daemon") == 0)
            daemon = 1;
        else if (strcmp(argv[i], "--dry-run") == 0)
            dry = 1;
        else if (strcmp(argv[i], "--restore") == 0)
            restore = 1;
        else if (strcmp(argv[i], "--show") == 0 && i + 1 < argc)
            reason = argv[++i];
        else
        {
            fprintf(stderr, i18n_tr(MSG_UNKNOWN_OPT), argv[i]);
            usage(argv[0]);
            return 2;
        }
    }

    /* 无任何模式参数：显示帮助 */
    if (!reason && !monitor && !daemon && !dry)
    {
        usage(argv[0]);
        return 0;
    }

    if (getuid() != 0)
    {
        fprintf(stderr, "%s", i18n_tr(MSG_NEED_ROOT));
        return 1;
    }

    /* 应用蓝屏主题（颜色 / 终止代码 / 自定义字符串 / 彩虹 / VGA 模式） */
    bsod_set_theme(&theme);

    /* 蓝屏结束行为：默认重启系统；--restore 改为恢复并退出程序 */
    if (restore)
        bsod_set_exit_mode(BSOD_EXIT_RESTORE);

    /* 一次性蓝屏（对应 CustomBSOD 的「主动 BugCheck」触发） */
    if (reason)
        return bsod_show(reason) == 0 ? 0 : 1;

    /* 监控：守护进程模式下后台运行 */
    if (daemon && daemonize() != 0)
    {
        perror(i18n_tr(MSG_DAEMON_FAIL));
        return 1;
    }
    return monitor_run(dry) == 0 ? 0 : 1;
}
