#include <stdint.h>
#include <stddef.h>

#include "render.h"

void render_init(render_ctx *r, uint32_t *buf, uint32_t width,
                 uint32_t height, uint32_t stride)
{
    r->buf = buf;
    r->width = width;
    r->height = height;
    r->stride = stride;
}

void render_clear(render_ctx *r, uint32_t color)
{
    uint32_t row_px = r->stride / sizeof(uint32_t);
    uint32_t y;

    for (y = 0; y < r->height; y++)
    {
        uint32_t *row = r->buf + (uint32_t)y * row_px;
        uint32_t x;

        for (x = 0; x < r->width; x++)
        {
            row[x] = color;
        }
    }
}

void render_fill_rect(render_ctx *r, int x, int y, int w, int h,
                      uint32_t color)
{
    int x0 = x < 0 ? 0 : x;
    int y0 = y < 0 ? 0 : y;
    int x1 = x + w;
    int y1 = y + h;
    int xx, yy;
    uint32_t row_px = r->stride / sizeof(uint32_t);

    if (x1 > (int)r->width)
        x1 = (int)r->width;
    if (y1 > (int)r->height)
        y1 = (int)r->height;
    if (x0 >= x1 || y0 >= y1)
        return;

    for (yy = y0; yy < y1; yy++)
    {
        uint32_t *row = r->buf + (uint32_t)yy * row_px;

        for (xx = x0; xx < x1; xx++)
        {
            row[xx] = color;
        }
    }
}

void render_blend_pixel(render_ctx *r, int x, int y, uint32_t color,
                        uint8_t alpha)
{
    uint32_t sr, sg, sb;
    uint32_t dr, dg, db;
    uint32_t a, inv;
    uint32_t *dst;

    if (x < 0 || y < 0 || x >= (int)r->width || y >= (int)r->height)
        return;
    if (alpha == 0)
        return;

    dst = r->buf + (uint32_t)y * (r->stride / sizeof(uint32_t)) +
          (uint32_t)x;

    if (alpha == 255)
    {
        *dst = color;
        return;
    }

    sr = (color >> 16) & 0xFF;
    sg = (color >> 8) & 0xFF;
    sb = color & 0xFF;
    dr = (*dst >> 16) & 0xFF;
    dg = (*dst >> 8) & 0xFF;
    db = *dst & 0xFF;

    a = alpha;
    inv = 255 - a;
    *dst = (((sr * a + dr * inv) / 255) << 16) |
           (((sg * a + dg * inv) / 255) << 8) |
           ((sb * a + db * inv) / 255);
}
