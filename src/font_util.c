#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include <ft2build.h>
#include FT_FREETYPE_H

#include "config.h"
#include "render.h"
#include "font_util.h"

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif

#define MAX_PATH_LEN 512

/* 无 fontconfig 时的回退字体路径（按优先级） */
static const char *const fallback_fonts[] = {
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
    "/usr/share/fonts/opentype/noto/NotoSansCJK-Medium.ttc",
    "/usr/share/fonts/truetype/droid/DroidSansFallbackFull.ttf",
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    NULL,
};

/* ---- UTF-8 解码：解析一个字符的码点并把指针前移 ----
 * 返回 0 表示字符串结束；返回 0xFFFFFFFF 表示非法字节（跳过） */
static uint32_t utf8_decode(const char **s)
{
    const unsigned char *p = (const unsigned char *)*s;
    uint32_t cp;

    if (p[0] == 0)
        return 0;

    if (p[0] < 0x80)
    {
        cp = p[0];
        *s += 1;
    }
    else if ((p[0] & 0xE0) == 0xC0 && (p[1] & 0xC0) == 0x80)
    {
        cp = ((uint32_t)(p[0] & 0x1F) << 6) | (p[1] & 0x3F);
        *s += 2;
    }
    else if ((p[0] & 0xF0) == 0xE0 && (p[1] & 0xC0) == 0x80 &&
             (p[2] & 0xC0) == 0x80)
    {
        cp = ((uint32_t)(p[0] & 0x0F) << 12) |
             ((uint32_t)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        *s += 3;
    }
    else if ((p[0] & 0xF8) == 0xF0 && (p[1] & 0xC0) == 0x80 &&
             (p[2] & 0xC0) == 0x80 && (p[3] & 0xC0) == 0x80)
    {
        cp = ((uint32_t)(p[0] & 0x07) << 18) |
             ((uint32_t)(p[1] & 0x3F) << 12) |
             ((uint32_t)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        *s += 4;
    }
    else
    {
        *s += 1; /* 非法字节 */
        return 0xFFFFFFFF;
    }
    return cp;
}

int font_init(font_ctx *fc)
{
    FT_Library lib;

    if (!fc)
        return -1;
    if (fc->library)
        return 0;
    if (FT_Init_FreeType(&lib) != 0)
    {
        fprintf(stderr, "FT_Init_FreeType 失败\n");
        return -1;
    }
    fc->library = lib;
    return 0;
}

void font_cleanup(font_ctx *fc)
{
    if (!fc)
        return;
    if (fc->face)
    {
        FT_Done_Face((FT_Face)fc->face);
        fc->face = NULL;
    }
    if (fc->library)
    {
        FT_Done_FreeType((FT_Library)fc->library);
        fc->library = NULL;
    }
}

#ifdef HAVE_FONTCONFIG
/* 用 fontconfig 按族名/语言在系统字体目录里查找字体文件与面序号 */
static int font_find_fc(const char *family, char *path, size_t pathsz,
                        int *face_index)
{
    FcPattern *pat;
    FcPattern *match;
    FcChar8 *file = NULL;
    FcResult result;
    int index = 0;
    int ok = -1;

    pat = FcPatternCreate();
    if (!pat)
        return -1;
    FcPatternAddString(pat, FC_FAMILY, (const FcChar8 *)family);
    FcPatternAddString(pat, FC_LANG, (const FcChar8 *)"zh-cn");

    FcConfigSubstitute(NULL, pat, FcMatchPattern);
    FcDefaultSubstitute(pat);

    match = FcFontMatch(NULL, pat, &result);
    FcPatternDestroy(pat);
    if (!match)
        return -1;

    if (FcPatternGetString(match, FC_FILE, 0, &file) == FcResultMatch && file)
    {
        snprintf(path, pathsz, "%s", (char *)file);
        FcPatternGetInteger(match, FC_INDEX, 0, &index);
        *face_index = index;
        ok = 0;
    }
    FcPatternDestroy(match);
    return ok;
}
#endif /* HAVE_FONTCONFIG */

int font_set_size(font_ctx *fc, unsigned int pixel_size)
{
    if (!fc || !fc->face)
        return -1;
    if (FT_Set_Pixel_Sizes((FT_Face)fc->face, 0, pixel_size) != 0)
    {
        fprintf(stderr, "FT_Set_Pixel_Sizes(%u) 失败\n", pixel_size);
        return -1;
    }
    return 0;
}

int font_load_file(font_ctx *fc, const char *path, int face_index,
                   unsigned int pixel_size)
{
    FT_Face face;

    if (!fc)
        return -1;
    if (!fc->library && font_init(fc) != 0)
        return -1;

    if (fc->face)
    {
        FT_Done_Face((FT_Face)fc->face);
        fc->face = NULL;
    }

    if (FT_New_Face((FT_Library)fc->library, path, face_index, &face) != 0)
    {
        fprintf(stderr, "FT_New_Face 失败: %s (face=%d)\n", path, face_index);
        return -1;
    }
    fc->face = face;
    return font_set_size(fc, pixel_size);
}

int font_load(font_ctx *fc, const char *family, unsigned int pixel_size)
{
    char path[MAX_PATH_LEN];
    int index = 0;
    int i;

    if (!fc)
        return -1;
    if (!fc->library && font_init(fc) != 0)
        return -1;

#ifdef HAVE_FONTCONFIG
    if (font_find_fc(family, path, sizeof(path), &index) == 0)
    {
        return font_load_file(fc, path, index, pixel_size);
    }
    fprintf(stderr, "[font] fontconfig 未找到族名 %s，回退目录扫描\n", family);
#endif

    /* 回退：扫描常见系统字体目录 */
    for (i = 0; fallback_fonts[i]; i++)
    {
        if (access(fallback_fonts[i], R_OK) == 0)
        {
            fprintf(stderr, "[font] 使用回退字体: %s\n", fallback_fonts[i]);
            return font_load_file(fc, fallback_fonts[i], 0, pixel_size);
        }
    }

    fprintf(stderr, "未找到可用字体（family=%s）\n", family);
    return -1;
}

int font_text_width(font_ctx *fc, const char *text, unsigned int pixel_size)
{
    int w = 0;

    if (!fc || !fc->face || !text)
        return 0;
    if (font_set_size(fc, pixel_size) != 0)
        return 0;

    while (*text)
    {
        uint32_t cp = utf8_decode(&text);

        if (cp == 0)
            break;
        if (cp != 0xFFFFFFFF &&
            FT_Load_Char((FT_Face)fc->face, cp, FT_LOAD_DEFAULT) == 0)
        {
            w += (int)(((FT_Face)fc->face)->glyph->advance.x >> 6);
        }
    }
    return w;
}

int font_line_height(font_ctx *fc, unsigned int pixel_size)
{
    FT_Face face;

    if (!fc || !fc->face)
        return 0;
    if (font_set_size(fc, pixel_size) != 0)
        return 0;

    face = (FT_Face)fc->face;
    if (!face->size)
        return 0;
    return (int)(face->size->metrics.height >> 6);
}

int font_draw_text(font_ctx *fc, render_ctx *r, int x, int y,
                   const char *text, uint32_t color, unsigned int pixel_size)
{
    FT_Face face;
    const char *p;
    FT_GlyphSlot slot;
    FT_Bitmap *bm;
    int min_left = 0; /* 整段文字最左的 bitmap_left */
    int max_top = 0;  /* 整段文字最高的 bitmap_top */
    int first = 1;
    int pen_x;
    int baseline;
    unsigned int row, col;

    if (!fc || !fc->face || !text)
        return 0;
    if (font_set_size(fc, pixel_size) != 0)
        return 0;

    face = (FT_Face)fc->face;

    /* 第一遍：测量整段文字的实际左边界与顶部 */
    for (p = text; *p;)
    {
        uint32_t cp = utf8_decode(&p);

        if (cp == 0xFFFFFFFF)
            continue;
        if (FT_Load_Char(face, cp, FT_LOAD_DEFAULT) != 0)
            continue;

        slot = face->glyph;
        if (first)
        {
            min_left = slot->bitmap_left;
            max_top = slot->bitmap_top;
            first = 0;
        }
        else
        {
            if (slot->bitmap_left < min_left)
                min_left = slot->bitmap_left;
            if (slot->bitmap_top > max_top)
                max_top = slot->bitmap_top;
        }
    }

    /* (x, y) 为整段文字左上角：左边界对齐 x，顶部对齐 y */
    pen_x = x - min_left;
    baseline = y + max_top;

    /* 第二遍：实际渲染 */
    for (p = text; *p;)
    {
        uint32_t cp = utf8_decode(&p);

        if (cp == 0)
            break;
        if (cp == 0xFFFFFFFF)
        {
            pen_x += (int)pixel_size;
            continue;
        }
        if (FT_Load_Char(face, cp, FT_LOAD_RENDER) != 0)
        {
            pen_x += (int)pixel_size;
            continue;
        }

        slot = face->glyph;
        bm = &slot->bitmap;
        if (bm->width > 0 && bm->rows > 0)
        {
            int gx = pen_x + slot->bitmap_left;
            int gy = baseline - slot->bitmap_top;

            for (row = 0; row < bm->rows; row++)
            {
                for (col = 0; col < bm->width; col++)
                {
                    uint8_t a = bm->buffer[row * bm->pitch + col];

                    if (a)
                        render_blend_pixel(r, gx + (int)col,
                                           gy + (int)row, color, a);
                }
            }
        }
        pen_x += (int)(slot->advance.x >> 6);
    }
    return pen_x - (x - min_left);
}
