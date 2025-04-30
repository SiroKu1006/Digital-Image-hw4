#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push,1)
// BMP 檔頭 (14 bytes)
typedef struct {
    unsigned short bfType;      // 'BM'=0x4D42
    unsigned int   bfSize;      // 檔案總大小
    unsigned short bfReserved1; 
    unsigned short bfReserved2;
    unsigned int   bfOffBits;   // pixel data 起始位移
} BMPFileHeader;

// BMP 資訊頭 (40 bytes)
typedef struct {
    unsigned int   biSize;
    int            biWidth;
    int            biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;    // 本題為 8-bit
    unsigned int   biCompression;
    unsigned int   biSizeImage;
    int            biXPelsPerMeter;
    int            biYPelsPerMeter;
    unsigned int   biClrUsed;
    unsigned int   biClrImportant;
} BMPInfoHeader;
#pragma pack(pop)

void  readBMP(const char* fname,
              BMPFileHeader *fh,
              BMPInfoHeader *ih,
              unsigned char **colorTable,
              unsigned char **data);
void  writeBMP(const char* fname,
               BMPFileHeader *fh,
               BMPInfoHeader *ih,
               unsigned char *colorTable,
               unsigned char *data);
unsigned char* blur3x3(unsigned char *data, int w, int h, int stride);
unsigned char* shrinkHalf(unsigned char *data,
                          int w, int h, int stride,
                          BMPInfoHeader *ih, BMPFileHeader *fh);

int main(void) {
    BMPFileHeader fh;
    BMPInfoHeader ih;
    unsigned char *colorTable = NULL, *data = NULL;

    // 讀入原圖
    readBMP("Fig0417(a).bmp", &fh, &ih, &colorTable, &data);
    int width  = ih.biWidth;
    int height = abs(ih.biHeight);
    int stride = ((width + 3) / 4) * 4;  

    // (a) 直接 50% 縮小
    {
        BMPFileHeader fh_a = fh;
        BMPInfoHeader ih_a = ih;
        unsigned char *out_a = shrinkHalf(data, width, height, stride, &ih_a, &fh_a);
        writeBMP("p1_a.bmp", &fh_a, &ih_a, colorTable, out_a);
        free(out_a);
    }

    // (b) 先 3×3 平均濾波，再 50% 縮小
    {
        unsigned char *blur = blur3x3(data, width, height, stride);
        BMPFileHeader fh_b = fh;
        BMPInfoHeader ih_b = ih;
        unsigned char *out_b = shrinkHalf(blur, width, height, stride, &ih_b, &fh_b);
        writeBMP("p1_b.bmp", &fh_b, &ih_b, colorTable, out_b);
        free(blur);
        free(out_b);
    }

    free(colorTable);
    free(data);
    return 0;
}

// 讀 8-bit BMP：檔頭／資訊頭／色表／像素
void readBMP(const char* fname,
             BMPFileHeader *fh,
             BMPInfoHeader *ih,
             unsigned char **colorTable,
             unsigned char **data) {
    FILE *fp = fopen(fname, "rb");
    if (!fp) { perror("fopen"); exit(1); }

    fread(fh, 1, sizeof(*fh), fp);
    fread(ih, 1, sizeof(*ih), fp);
    if (fh->bfType != 0x4D42 || ih->biBitCount != 8) {
        fprintf(stderr, "只支援 8-bit BMP\n"); exit(1);
    }

    int colorTableSize = fh->bfOffBits - sizeof(*fh) - sizeof(*ih);
    *colorTable = malloc(colorTableSize);
    fread(*colorTable, 1, colorTableSize, fp);

    int w = ih->biWidth, h = abs(ih->biHeight);
    int stride = ((w + 3) / 4) * 4;
    int dataSize = stride * h;
    *data = malloc(dataSize);
    fseek(fp, fh->bfOffBits, SEEK_SET);
    fread(*data, 1, dataSize, fp);
    fclose(fp);
}

// 寫 8-bit BMP：更新後的檔頭／資訊頭＋原色表＋新像素
void writeBMP(const char* fname,
              BMPFileHeader *fh,
              BMPInfoHeader *ih,
              unsigned char *colorTable,
              unsigned char *data) {
    FILE *fp = fopen(fname, "wb");
    if (!fp) { perror("fopen"); exit(1); }

    fwrite(fh, 1, sizeof(*fh), fp);
    fwrite(ih, 1, sizeof(*ih), fp);

    int colorTableSize = fh->bfOffBits - sizeof(*fh) - sizeof(*ih);
    fwrite(colorTable, 1, colorTableSize, fp);

    int w = ih->biWidth, h = abs(ih->biHeight);
    int stride = ((w + 3) / 4) * 4;
    fwrite(data, 1, stride * h, fp);
    fclose(fp);
}

// 3×3 平均濾波
unsigned char* blur3x3(unsigned char *data, int w, int h, int stride) {
    unsigned char *out = malloc(stride * h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int sum = 0, cnt = 0;
            for (int dy = -1; dy <= 1; dy++) {
                for (int dx = -1; dx <= 1; dx++) {
                    int yy = y + dy, xx = x + dx;
                    if (yy < 0) yy = 0; else if (yy >= h) yy = h - 1;
                    if (xx < 0) xx = 0; else if (xx >= w) xx = w - 1;
                    sum += data[yy*stride + xx];
                    cnt++;
                }
            }
            out[y*stride + x] = sum / cnt;
        }
    }
    return out;
}

// 50% 縮小 (最近鄰抽樣)，並更新檔頭資訊
unsigned char* shrinkHalf(unsigned char *data,
                          int w, int h, int stride,
                          BMPInfoHeader *ih, BMPFileHeader *fh) {
    int nw = w / 2, nh = h / 2;
    int nstride = ((nw + 3) / 4) * 4;
    unsigned char *out = malloc(nstride * nh);

    for (int y = 0; y < nh; y++) {
        for (int x = 0; x < nw; x++) {
            out[y*nstride + x] = data[(2*y)*stride + (2*x)];
        }
    }

    // 更新資訊頭 & 檔頭
    ih->biWidth     = nw;
    ih->biHeight    = (ih->biHeight > 0 ? nh : -nh);
    ih->biSizeImage = nstride * nh;
    fh->bfSize      = fh->bfOffBits + ih->biSizeImage;
    return out;
}
