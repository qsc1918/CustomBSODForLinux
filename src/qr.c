#include <stdlib.h>
#include <stdint.h>

#include "render.h"
#include "qr.h"
#include "qrcodegen.h"

int qr_matrix_alloc(qr_matrix *q, int size)
{
    if (!q || size <= 0)
        return -1;
    q->modules = (uint8_t *)calloc((size_t)size * (size_t)size,
                                   sizeof(uint8_t));
    if (!q->modules)
        return -1;
    q->size = size;
    return 0;
}

void qr_matrix_free(qr_matrix *q)
{
    if (!q)
        return;
    free(q->modules);
    q->modules = NULL;
    q->size = 0;
}

int qr_generate_text(qr_matrix *q, const char *text, int min_version,
                     int max_version)
{
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t temp[qrcodegen_BUFFER_LEN_MAX];
    int size;
    int x, y;

    if (!q || !text)
        return -1;

    if (!qrcodegen_encodeText(text, temp, qrcode, qrcodegen_Ecc_MEDIUM,
                              min_version, max_version,
                              qrcodegen_Mask_AUTO, true))
        return -1;

    size = qrcodegen_getSize(qrcode);
    if (qr_matrix_alloc(q, size) != 0)
        return -1;

    for (y = 0; y < size; y++)
    {
        for (x = 0; x < size; x++)
        {
            q->modules[(size_t)y * (size_t)size + (size_t)x] =
                qrcodegen_getModule(qrcode, x, y) ? 1 : 0;
        }
    }
    return 0;
}

void qr_draw(render_ctx *r, const qr_matrix *q, int x, int y,
             int size, const qr_style *st)
{
    int n;       /* 模块数 */
    int nominal; /* 名义模块大小（用于留白） */
    int qz;      /* 四周留白（像素） */
    int panel;   /* 背景面板边长（像素） */
    int total;   /* 含外边框的总边长（像素） */
    int px, py;  /* 数据区左上角 */
    int i, j;

    if (!r || !q || !q->modules || !st || size <= 0)
        return;

    n = q->size;
    nominal = size / n;
    if (nominal < 1)
        nominal = 1;

    qz = st->quiet * nominal;
    panel = size + qz * 2;
    total = panel + st->border * 2;

    /* 外边框 */
    render_fill_rect(r, x, y, total, total, st->frame);
    /* 背景面板 */
    render_fill_rect(r, x + st->border, y + st->border, panel, panel, st->bg);
    /* 数据区：用等分边界法精确铺满 size。
     * 第 i 个模块占 [floor(i*size/n), floor((i+1)*size/n))，
     * 首尾恰好 0..size，保证总宽高固定为 size，不随内容/版本变化。 */
    px = x + st->border + qz;
    py = y + st->border + qz;
    for (j = 0; j < n; j++)
    {
        int y0 = py + (int)((int64_t)j * size / n);
        int y1 = py + (int)((int64_t)(j + 1) * size / n);

        for (i = 0; i < n; i++)
        {
            int x0 = px + (int)((int64_t)i * size / n);
            int x1 = px + (int)((int64_t)(i + 1) * size / n);

            if (q->modules[(size_t)j * (size_t)n + (size_t)i])
                render_fill_rect(r, x0, y0, x1 - x0, y1 - y0, st->fg);
        }
    }
}
