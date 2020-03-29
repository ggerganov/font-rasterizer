/* thanks to : https://github.com/justinmeiners/stb-truetype-example */

#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h" /* http://nothings.org/stb/stb_image_write.h */

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h" /* http://nothings.org/stb/stb_truetype.h */

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        printf("usage: %s font.ttf height [width]\n", argv[0]);
        printf("\n");
        printf("       rasterizes the provided ttf font into cells with specified height in pixels\n");
        printf("       if width is provided, it overrides the auto detected value\n");
        printf("       the generated raster data is printed on the standard output\n");
        return -1;
    }

    const char * fontFName = argv[1];
    int height = atoi(argv[2]);
    int width = argc > 3 ? atoi(argv[3]) : 0;

    unsigned char* fontBuffer;

    /* load font file */
    {
        long size;
        FILE* fontFile = fopen(argv[1], "rb");
        fseek(fontFile, 0, SEEK_END);
        size = ftell(fontFile);
        fseek(fontFile, 0, SEEK_SET);

        fontBuffer = malloc(size);

        fread(fontBuffer, size, 1, fontFile);
        fclose(fontFile);
    }

    /* prepare font */
    stbtt_fontinfo info;
    if (!stbtt_InitFont(&info, fontBuffer, 0)) {
        printf("failed to load ttf font\n");
        return -2;
    }

    int b_w = 0; /* bitmap width */
    int b_h = height; /* bitmap height */
    int c_w = width; /* char width */
    int c_h = height; /* char height */

    /* calculate font scaling */
    float scale = stbtt_ScaleForPixelHeight(&info, c_h);

    if (c_w == 0) {
        /* determine max char width */
        for (unsigned char ch = 0; ch < 128; ++ch) {
            int ax;
            int lsb;
            stbtt_GetCodepointHMetrics(&info, ch, &ax, &lsb);
            if (ax*scale > c_w) {
                c_w = ax*scale;
            }
        }
    }

    b_w = 128*c_w;

    /* create a bitmap for the phrase */
    unsigned char* bitmap = calloc(b_w*b_h, sizeof(unsigned char));

    int x = 0, maxx = 0;
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &lineGap);

    ascent *= scale;
    descent *= scale;

    /* begin C constants output */
    printf("static const int FONT_SIZE_X = %d;\n", c_w);
    printf("static const int FONT_SIZE_Y = %d;\n\n", c_h);
    printf("static const unsigned char kFontRaster[128][FONT_SIZE_Y*FONT_SIZE_X] = {\n");

    for (unsigned char ch = 0; ch < 128; ++ch) {
        /* how wide is this character */
        int ax;
        int lsb;
        stbtt_GetCodepointHMetrics(&info, ch, &ax, &lsb);

        /* get bounding box for character (may be offset to account for chars that dip above or below the line */
        int c_x1, c_y1, c_x2, c_y2;
        stbtt_GetCodepointBitmapBox(&info, ch, scale, scale, &c_x1, &c_y1, &c_x2, &c_y2);

        /* compute y (different characters have different heights */
        int y = ascent + c_y1;
        if (y < 0) y = 0;

        /* render character (stride and offset is important here) */
        int byteOffset = x + (lsb*scale) + (y*b_w);
        stbtt_MakeCodepointBitmap(&info, bitmap + byteOffset, c_x2 - c_x1, c_y2 - c_y1, b_w, scale, scale, ch);

        /* output C array data */
        printf("    {");
        int cw = ax*scale;
        for (int yy = 0; yy < c_h; ++yy) {
            for (int xx = 0; xx < c_w; ++xx) {
                int v = bitmap[byteOffset + (yy - y)*b_w + xx];
                printf(" %3d,", v);
                if (v > 0) {
                    maxx = xx;
                }
            }
        }
        printf(" }, // U+%04d\n", ch);

        /* advance x */
        x += ax*scale;

        /* add kerning */
        if (ch < 127) {
            int kern;
            kern = stbtt_GetCodepointKernAdvance(&info, ch, ch + 1);
            x += kern*scale;
        }
    }

    printf("};\n");

    if (maxx + 1 < c_w) {
        printf("// warning : max character width was %d. consider lowering the width parameter to this value\n", maxx + 1);
    }

    /* save out a 1 channel image */
    {
        char tmp[128];
        snprintf(tmp, 128, "raster-h%d.png", c_h);
        stbi_write_png(tmp, b_w, b_h, 1, bitmap, b_w);
    }

    free(fontBuffer);
    free(bitmap);

    return 0;
}
