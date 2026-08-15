#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#include "drm_util.h"

#define LOG_ERR(fmt, ...) fprintf(stderr, "[drm] " fmt "\n", ##__VA_ARGS__)

void drm_bsod_ctx_init(drm_bsod_ctx *ctx)
{
    memset(ctx, 0, sizeof(*ctx));
    ctx->fd = -1;
    ctx->map_ptr = (uint32_t *)MAP_FAILED;
}

/* 检查设备上是否有已连接的显示器（只读查询，无需 Master 权限） */
static int has_connected_display(int fd)
{
    drmModeRes *res = drmModeGetResources(fd);
    int i;

    if (!res)
        return 0;
    for (i = 0; i < res->count_connectors; i++)
    {
        drmModeConnector *conn =
            drmModeGetConnector(fd, res->connectors[i]);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
        {
            drmModeFreeConnector(conn);
            drmModeFreeResources(res);
            return 1;
        }
        if (conn)
            drmModeFreeConnector(conn);
    }
    drmModeFreeResources(res);
    return 0;
}

int drm_open_device(drm_bsod_ctx *ctx, const char *path)
{
    if (path && path[0])
    {
        /* 显式指定设备路径 */
        ctx->fd = open(path, O_RDWR | O_CLOEXEC);
        if (ctx->fd < 0)
        {
            perror("打开 DRM 设备失败");
            return -1;
        }
    }
    else
    {
        /* 自动探测：依次尝试 /dev/dri/card0..card7，选第一个带显示器的卡 */
        int i;

        for (i = 0; i < 8; i++)
        {
            char dev[64];

            snprintf(dev, sizeof(dev), "/dev/dri/card%d", i);
            ctx->fd = open(dev, O_RDWR | O_CLOEXEC);
            if (ctx->fd < 0)
                continue;
            if (!has_connected_display(ctx->fd))
            {
                close(ctx->fd);
                ctx->fd = -1;
                continue;
            }
            break;
        }
        if (ctx->fd < 0)
        {
            LOG_ERR("未找到可用的 DRM 设备（已尝试 /dev/dri/card0..card7），"
                    "可先执行 ls /dev/dri 查看实际设备名");
            return -1;
        }
    }

    if (drmSetMaster(ctx->fd) < 0)
    {
        perror("drmSetMaster 抢占控制权失败");
        close(ctx->fd);
        ctx->fd = -1;
        return -1;
    }
    return 0;
}

int drm_find_connector(drm_bsod_ctx *ctx)
{
    int i;

    ctx->resources = drmModeGetResources(ctx->fd);
    if (!ctx->resources)
    {
        perror("获取 DRM 资源失败");
        return -1;
    }

    for (i = 0; i < ctx->resources->count_connectors; i++)
    {
        drmModeConnector *conn =
            drmModeGetConnector(ctx->fd, ctx->resources->connectors[i]);
        if (conn && conn->connection == DRM_MODE_CONNECTED && conn->count_modes > 0)
        {
            ctx->connector = conn;
            break;
        }
        if (conn)
        {
            drmModeFreeConnector(conn);
        }
    }

    if (!ctx->connector)
    {
        LOG_ERR("未找到已连接的显示器");
        return -1;
    }

    ctx->mode = ctx->connector->modes[0];
    ctx->width = ctx->mode.hdisplay;
    ctx->height = ctx->mode.vdisplay;

    /* 优先使用连接器绑定的编码器对应的 CRTC，否则回退到第一个 CRTC */
    if (ctx->connector->encoder_id)
    {
        drmModeEncoder *encoder =
            drmModeGetEncoder(ctx->fd, ctx->connector->encoder_id);
        if (encoder)
        {
            ctx->crtc_id = encoder->crtc_id;
            drmModeFreeEncoder(encoder);
        }
    }
    if (ctx->crtc_id == 0 && ctx->resources->count_crtcs > 0)
    {
        ctx->crtc_id = ctx->resources->crtcs[0];
    }

    return 0;
}

int drm_create_fb(drm_bsod_ctx *ctx)
{
    struct drm_mode_create_dumb create_req;
    struct drm_mode_map_dumb map_req;

    memset(&create_req, 0, sizeof(create_req));
    create_req.width = ctx->width;
    create_req.height = ctx->height;
    create_req.bpp = 32;

    if (ioctl(ctx->fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req) < 0)
    {
        perror("创建 Dumb Buffer 失败");
        return -1;
    }
    ctx->dumb_handle = create_req.handle;
    ctx->dumb_size = create_req.size;
    ctx->pitch = create_req.pitch;

    if (drmModeAddFB(ctx->fd, ctx->width, ctx->height, 24, 32,
                     create_req.pitch, create_req.handle, &ctx->fb_id) < 0)
    {
        perror("drmModeAddFB 失败");
        return -1;
    }

    memset(&map_req, 0, sizeof(map_req));
    map_req.handle = create_req.handle;
    if (ioctl(ctx->fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req) < 0)
    {
        perror("MAP_DUMB 失败");
        return -1;
    }

    ctx->map_ptr = (uint32_t *)mmap(0, create_req.size, PROT_READ | PROT_WRITE,
                                    MAP_SHARED, ctx->fd, map_req.offset);
    if (ctx->map_ptr == (uint32_t *)MAP_FAILED)
    {
        perror("mmap 显存失败");
        ctx->map_ptr = (uint32_t *)MAP_FAILED;
        return -1;
    }
    return 0;
}

int drm_set_mode(drm_bsod_ctx *ctx)
{
    ctx->saved_crtc = drmModeGetCrtc(ctx->fd, ctx->crtc_id);
    if (!ctx->saved_crtc)
    {
        perror("drmModeGetCrtc 保存原始状态失败");
        return -1;
    }
    return drm_flip(ctx);
}

int drm_flip(drm_bsod_ctx *ctx)
{
    if (drmModeSetCrtc(ctx->fd, ctx->crtc_id, ctx->fb_id, 0, 0,
                       &ctx->connector->connector_id, 1, &ctx->mode) < 0)
    {
        perror("drmModeSetCrtc 刷新失败");
        return -1;
    }
    return 0;
}

int drm_restore(drm_bsod_ctx *ctx)
{
    if (!ctx->saved_crtc)
    {
        return 0;
    }

    if (drmModeSetCrtc(ctx->fd, ctx->saved_crtc->crtc_id,
                       ctx->saved_crtc->buffer_id,
                       ctx->saved_crtc->x, ctx->saved_crtc->y,
                       &ctx->connector->connector_id, 1,
                       &ctx->saved_crtc->mode) < 0)
    {
        perror("还原 CRTC 输出失败");
        return -1;
    }
    return 0;
}

void drm_cleanup(drm_bsod_ctx *ctx)
{
    struct drm_mode_destroy_dumb destroy_req;

    if (ctx->saved_crtc)
    {
        drmModeFreeCrtc(ctx->saved_crtc);
        ctx->saved_crtc = NULL;
    }
    if (ctx->map_ptr != (uint32_t *)MAP_FAILED && ctx->dumb_size > 0)
    {
        munmap(ctx->map_ptr, ctx->dumb_size);
        ctx->map_ptr = (uint32_t *)MAP_FAILED;
    }
    if (ctx->fb_id > 0)
    {
        drmModeRmFB(ctx->fd, ctx->fb_id);
        ctx->fb_id = 0;
    }
    if (ctx->dumb_handle > 0)
    {
        memset(&destroy_req, 0, sizeof(destroy_req));
        destroy_req.handle = ctx->dumb_handle;
        ioctl(ctx->fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
        ctx->dumb_handle = 0;
    }
    if (ctx->connector)
    {
        drmModeFreeConnector(ctx->connector);
        ctx->connector = NULL;
    }
    if (ctx->resources)
    {
        drmModeFreeResources(ctx->resources);
        ctx->resources = NULL;
    }
    if (ctx->fd >= 0)
    {
        drmDropMaster(ctx->fd);
        close(ctx->fd);
        ctx->fd = -1;
    }
}
