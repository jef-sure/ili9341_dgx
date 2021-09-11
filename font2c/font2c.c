#include <ctype.h>
#include <ft2build.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include FT_GLYPH_H
#include FT_MODULE_H
#include FT_FREETYPE_H
#include FT_TRUETYPE_DRIVER_H

typedef struct {
    size_t bitmapOffset; ///< Pointer into GFXfont->bitmap
    uint16_t width;         ///< Bitmap dimensions in pixels
    uint16_t height;        ///< Bitmap dimensions in pixels
    uint16_t xAdvance;      ///< Distance to advance cursor (x axis)
    int16_t xOffset;        ///< X dist from cursor pos to UL corner
    int16_t yOffset;        ///< Y dist from cursor pos to UL corner
} glyph_t;

typedef struct {
    uint32_t codePoint;
    glyph_t ginfo;
} LoadedGlyph;

typedef struct _cp_ranges {
    int first, number;
    int gOffset;
    struct _cp_ranges *next;
} cp_ranges_t;

typedef struct {
    int first, number;
    glyph_t *glyphs;
} glyph_array_t;

cp_ranges_t *CPRanges = 0;

int isInRange(int codePoint) {
    for (cp_ranges_t *r = CPRanges; r; r = r->next)
        if (codePoint >= r->first && codePoint < r->first + r->number) return 1;
    return 0;
}

cp_ranges_t *SortedCharMap = 0;

void cpr_insert_cp(int codePoint) {
    cp_ranges_t *r;
    if (SortedCharMap == 0) {
        SortedCharMap = malloc(sizeof(cp_ranges_t));
        SortedCharMap->first = codePoint;
        SortedCharMap->number = 1;
        SortedCharMap->next = 0;
        return;
    } else {
        if (codePoint < SortedCharMap->first) {
            if (codePoint == SortedCharMap->first - 1) {
                SortedCharMap->first--;
                SortedCharMap->number++;
                return;
            }
            cp_ranges_t *t = SortedCharMap;
            SortedCharMap = malloc(sizeof(cp_ranges_t));
            SortedCharMap->first = codePoint;
            SortedCharMap->number = 1;
            SortedCharMap->next = t;
            return;
        }
        for (r = SortedCharMap; r; r = r->next) {
            if (codePoint >= r->first && codePoint < r->first + r->number) return;
            if (codePoint == r->first + r->number) {
                r->number++;
                if (r->next && r->next->first - 1 == codePoint) {
                    cp_ranges_t *t = r->next;
                    r->number += t->number;
                    r->next = t->next;
                    free(t);
                }
                return;
            }
            if (!r->next || r->next->first > codePoint) {
                cp_ranges_t *t = malloc(sizeof(cp_ranges_t));
                t->first = codePoint;
                t->number = 1;
                t->next = r->next;
                r->next = t;
                return;
            }
        }
    }
}

void clearFontName(char *fname) {
    while (*fname) {
        char *fl = fname;
        while (*fl && !isalnum(*fl)) {
            ++fl;
        }
        if (fl != fname) {
            memmove(fname, fl, strlen(fl) + 1);
        }
        ++fname;
    }
}

void encodeBMXRLE(uint8_t *bitmap, uint16_t pitch, uint16_t width, uint16_t height, uint8_t **ret, size_t *ret_len) {
    width = (width + 7) / 8;
    size_t sq = (size_t) width * height;
    uint8_t *xbm = (uint8_t*) malloc(sq);
    memcpy(xbm, bitmap, width);
    for (size_t yi = 1; yi < height; ++yi) {
        for (size_t xi = 0; xi < width; xi++)
            xbm[yi * width + xi] = bitmap[yi * pitch + xi] ^ bitmap[(yi - 1) * pitch + xi];
    }
    uint8_t *rbm = (uint8_t*) calloc(2, sq);
    size_t ro = 0;
    for (size_t xi = 0; xi < sq;) {
        uint8_t cb = xbm[xi];
        uint8_t bl = 1;
        for (; bl < 127 && xi + bl < sq; ++bl)
            if (xbm[xi + bl] != cb) break;
        if (bl == 1 && cb < 129) {
            rbm[ro++] = cb;
        } else {
            rbm[ro++] = bl | 128;
            rbm[ro++] = cb;
        }
        xi += bl;
    }
    *ret_len = ro;
    *ret = (uint8_t*) realloc(rbm, ro);
}

void decodeBMXRLE(uint8_t *bitmap, uint16_t width, uint16_t height, uint8_t *encoded, size_t encoded_len) {
    width = (width + 7) / 8;
    size_t sq = (size_t) width * height;
    for (size_t xi = 0, ei = 0; ei < encoded_len && xi < sq;) {
        uint8_t bl = encoded[ei++];
        if (bl < 129) {
            bitmap[xi++] = bl;
        } else {
            uint8_t cb = encoded[ei++];
            bl &= 0x7f;
            while (bl-- && xi < sq)
                bitmap[xi++] = cb;
        }
    }
    for (size_t xi = width; xi < sq; xi++)
        bitmap[xi] = bitmap[xi] ^ bitmap[xi - width];
}

void encodeBM(uint8_t *bitmap, uint16_t pitch, uint16_t width, uint16_t height, uint8_t **ret, size_t *ret_len) {
    width = (width + 7) / 8;
    size_t sq = (size_t) width * height;
    uint8_t *xbm = (uint8_t*) malloc(sq);
    for (size_t xi = 0; xi < height; ++xi) {
        memcpy(xbm + xi * width, bitmap + pitch * xi, width);
    }
    *ret_len = sq;
    *ret = xbm;
} // no decode required

int main(int argc, char *argv[]) {
    int i, j, err, size, bitmapOffset = 0;
    FT_Library library;
    FT_Face face;
    FT_Glyph glyph;
    FT_Bitmap *bitmap;
    FT_BitmapGlyphRec *g;
    LoadedGlyph *glyphs;

    if (argc < 3) {
        fprintf(stderr, "Usage: %s fontfile size [first last] .. [firstN lastN]\n", argv[0]);
        return 1;
    }

    size = atoi(argv[2]);

    for (int ri = 3; ri + 1 < argc; ri += 2) {
        int first = strtol(argv[ri], 0, 0);
        int last = strtol(argv[ri + 1], 0, 0);
        if (!CPRanges) {
            CPRanges = malloc(sizeof(cp_ranges_t));
            CPRanges->next = 0;
        } else {
            cp_ranges_t *r = malloc(sizeof(cp_ranges_t));
            r->next = CPRanges;
            CPRanges = r;
        }
        CPRanges->first = first;
        CPRanges->number = last - first + 1;
    }

    // Init FreeType lib, load font
    if ((err = FT_Init_FreeType(&library))) {
        fprintf(stderr, "FreeType init error: %d", err);
        return err;
    }

    // Use TrueType engine version 35, without subpixel rendering.
    // This improves clarity of fonts since this library does not
    // support rendering multiple levels of gray in a glyph.
    // See https://github.com/adafruit/Adafruit-GFX-Library/issues/103
//    FT_UInt interpreter_version = TT_INTERPRETER_VERSION_35;
//    FT_Property_Set(library, "truetype", "interpreter-version", &interpreter_version);

    if ((err = FT_New_Face(library, argv[1], 0, &face))) {
        fprintf(stderr, "Font load error: %d", err);
        FT_Done_FreeType(library);
        return err;
    }
    char fontname[256], funcname[256];
    snprintf(funcname, 256, "%s%s%d", face->family_name, face->style_name, size);
    clearFontName(funcname);
    strcpy(fontname, funcname);
    strncat(fontname, ".c", 255);
    FILE *fontOut = fopen(fontname, "w");
    if (!fontOut) {
        fprintf(stderr, "Error opening font file %s for writing: ", strerror(errno));
        return -1;
    }
    glyphs = calloc(face->num_glyphs, sizeof(LoadedGlyph));
    err = FT_Select_Charmap(face, FT_ENCODING_UNICODE);
    if (err) {
        fprintf(stderr, "FreeType FT_Select_Charmap error: %d", err);
        return err;
    }
    /*
     // << 6 because '26dot6' fixed-point format
     FT_Set_Char_Size(face, size << 6, 0, DPI, 0);
     */
    FT_Set_Pixel_Sizes(face, size, 0);

    FT_UInt gindex;

    fprintf(fontOut, "#include \"dgx_font.h\"\n");
    fprintf(fontOut, "static const uint8_t bitmaps[] = {\n  ");
    int fComma = 0;
    int32_t yOffsetLowest = 0, xWidest = 0;
    double xWidthAverage = 0;
    // Process glyphs and output huge bitmap data array
    for (i = FT_Get_First_Char(face, &gindex), j = 0; gindex != 0; i = FT_Get_Next_Char(face, i, &gindex)) {
        if (CPRanges && !isInRange(i)) continue;
        // MONO renderer provides clean image with perfect crop
        // (no wasted pixels) via bitmap struct.
        if ((err = FT_Load_Char(face, i, FT_LOAD_TARGET_MONO))) {
            fprintf(stderr, "Error %d loading char '%c'\n", err, i);
            continue;
        }

        if ((err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_MONO))) {
            fprintf(stderr, "Error %d rendering char '%c'\n", err, i);
            continue;
        }

        if ((err = FT_Get_Glyph(face->glyph, &glyph))) {
            fprintf(stderr, "Error %d getting glyph '%c'\n", err, i);
            continue;
        }

        bitmap = &face->glyph->bitmap;
        g = (FT_BitmapGlyphRec*) glyph;
        glyphs[j].codePoint = i;
        glyphs[j].ginfo.bitmapOffset = bitmapOffset;
        glyphs[j].ginfo.width = bitmap->width;
        glyphs[j].ginfo.height = bitmap->rows;
        glyphs[j].ginfo.xAdvance = face->glyph->advance.x >> 6;
        glyphs[j].ginfo.xOffset = g->left;
        glyphs[j].ginfo.yOffset = 1 - g->top;
        if (j == 0) {
            yOffsetLowest = glyphs[j].ginfo.yOffset;
            xWidest = glyphs[j].ginfo.width;
            xWidthAverage = glyphs[j].ginfo.width;
        } else {
            if (yOffsetLowest > glyphs[j].ginfo.yOffset) yOffsetLowest = glyphs[j].ginfo.yOffset;
            if (xWidest < glyphs[j].ginfo.width) xWidest = glyphs[j].ginfo.width;
            xWidthAverage += glyphs[j].ginfo.width;
        }
        cpr_insert_cp(i);
        char gname[64];
        if (0 == FT_Get_Glyph_Name(face, gindex, gname, sizeof(gname))) {
            fprintf(fontOut, "/* %04X - %.63s */", i, gname);
        } else {
            fprintf(fontOut, "/* %04X */", i);
        }
        uint8_t *ret;
        size_t ret_len;
        encodeBM(bitmap->buffer, bitmap->pitch, bitmap->width, bitmap->rows, &ret, &ret_len);
        for (int m = 0; m < bitmap->rows; ++m) {
            fprintf(fontOut, "\n/* ");
            for (int n = 0; n < bitmap->width; ++n) {
                uint8_t b = bitmap->buffer[bitmap->pitch * m + n / 8];
                fprintf(fontOut, "%c", (b & (0x80 >> (n & 7))) ? '#' : ' ');

            }
            fprintf(fontOut, " */");
        }
        for (int m = 0; m < ret_len; ++m) {
            if (fComma != 0) {
                fprintf(fontOut, ", ");
            }
            ++fComma;
            if (m % 12 == 0) fprintf(fontOut, "\n");
            fprintf(fontOut, "0x%02X", ret[m]);
        }
        bitmapOffset += ret_len;
        free(ret);
        fprintf(fontOut, "\n");
        FT_Done_Glyph(glyph);
        ++j;
    }
    if (j) {
        printf("Total width: %f, #glyphs: %d; avg = %f\n", xWidthAverage, j, xWidthAverage / j);
        xWidthAverage /= j;
    }
    fprintf(fontOut, "};\n\n"); // End bitmap array
    fprintf(stderr, "Total encoded length: %d\n", bitmapOffset);
    fprintf(fontOut, "static const glyph_t glyphs[] = {\n  ");
    int gidx = 0;
    for (cp_ranges_t *r = SortedCharMap; r; r = r->next) {
        r->gOffset = gidx;
        for (int cp = r->first; cp < r->first + r->number; ++cp) {
            if (gidx != 0) fprintf(fontOut, ", ");
            for (int gi = 0; gi < j; ++gi) {
                if (glyphs[gi].codePoint == cp) {
                    fprintf(fontOut, "  { bitmaps + %5lu, %3u, %3d, %3u, %4d, %4d } /* %04X */\n",
                            glyphs[gi].ginfo.bitmapOffset, glyphs[gi].ginfo.width, glyphs[gi].ginfo.height,
                            glyphs[gi].ginfo.xAdvance, glyphs[gi].ginfo.xOffset, glyphs[gi].ginfo.yOffset, cp);
                    gidx++;
                    break;
                }
            }
        }
    }
    fprintf(fontOut, "};\n");
    fprintf(fontOut, "static const glyph_array_t glyph_ranges[] = {\n  ");
    for (cp_ranges_t *r = SortedCharMap; r; r = r->next) {
        if (r != SortedCharMap) fprintf(fontOut, ",");
        fprintf(fontOut, " {0x%-4x, 0x%-4x, glyphs + %d }\n", r->first, r->number, r->gOffset);
    }
    fprintf(fontOut, ", {0x%-4x, 0x%-4x, %d }\n", 0, 0, 0);
    fprintf(fontOut, "};\n");
    fprintf(fontOut, "// Bitmap size: %d\n", bitmapOffset);

    fprintf(fontOut, "dgx_font_t* %s() {\n\t"
            "static dgx_font_t rval = { glyph_ranges, %ld, %d, %d, %f };\n\t"
            "return &rval;\n}\n",
            funcname,
            (face->size->metrics.height == 0 ? (long) glyphs[0].ginfo.height : face->size->metrics.height >> 6),
            yOffsetLowest, xWidest, xWidthAverage);
    fclose(fontOut);
    strcpy(fontname, funcname);
    strncat(fontname, ".h", 255);
    FILE *fontHeaderOut = fopen(fontname, "w");
    if (!fontHeaderOut) {
        fprintf(stderr, "Error opening font header file %s for writing: ", strerror(errno));
        return -1;
    }
    fprintf(fontHeaderOut, "#ifndef _TGX_FONT_%s_H_\n#define _TGX_FONT_%s_H_\n", funcname, funcname);
    fprintf(fontHeaderOut, "#include \"dgx_font.h\"\n");
    fprintf(fontHeaderOut, "#ifdef __cplusplus\n// @formatter:off\nextern \"C\" {\n// @formatter:on\n#endif\n");
    fprintf(fontHeaderOut, "dgx_font_t* %s();\n", funcname);
    fprintf(fontHeaderOut, "#ifdef __cplusplus\n// @formatter:off\n}\n// @formatter:on\n#endif\n");
    fprintf(fontHeaderOut, "#endif /* _TGX_FONT_%s_H_ */\n", funcname);

    fclose(fontHeaderOut);

    FT_Done_FreeType(library);

    return 0;
}

