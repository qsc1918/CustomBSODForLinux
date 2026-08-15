#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/vt.h>

#include "vt_util.h"

int vt_open_tty(void)
{
    int fd = open("/dev/tty0", O_RDWR);
    if (fd < 0)
    {
        perror("打开 /dev/tty0 失败");
        return -1;
    }
    return fd;
}

int vt_get_active(int tty_fd)
{
    struct vt_stat vt_info;

    memset(&vt_info, 0, sizeof(vt_info));
    if (ioctl(tty_fd, VT_GETSTATE, &vt_info) < 0)
    {
        perror("获取当前 VT 编号失败");
        return -1;
    }
    return vt_info.v_active;
}

int vt_switch_to(int tty_fd, int vt)
{
    if (ioctl(tty_fd, VT_ACTIVATE, vt) < 0)
    {
        perror("VT_ACTIVATE 切换失败");
        return -1;
    }
    if (ioctl(tty_fd, VT_WAITACTIVE, vt) < 0)
    {
        perror("VT_WAITACTIVE 等待切换失败");
        return -1;
    }
    return 0;
}
