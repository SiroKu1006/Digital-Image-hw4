#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "fft.h"
#define M_PI 3.14159265358979323846
#pragma pack(push, 1)
// BMP 檔案標頭（14 bytes）
typedef struct
{
    unsigned short bfType;      // 一定要是 'BM' (0x4D42)
    unsigned int bfSize;        // 檔案大小 (bytes)
    unsigned short bfReserved1; // 0
    unsigned short bfReserved2; // 0
    unsigned int bfOffBits;     // 從檔頭開始到像素資料的位移
} BMPFILEHEADER;

// BMP 資訊標頭（40 bytes）
typedef struct
{
    unsigned int biSize;         // 本結構大小 (40)
    int biWidth;                 // 寬度 (pixels)
    int biHeight;                // 高度 (pixels)，正值底部先存、負值頂部先存
    unsigned short biPlanes;     // 一定要是 1
    unsigned short biBitCount;   // 色深 (8 for 8-bit)
    unsigned int biCompression;  // 壓縮方式 (0 = BI_RGB 無壓縮)
    unsigned int biSizeImage;    // 像素資料大小 (bytes)
    int biXPelsPerMeter;         // 水平解析度
    int biYPelsPerMeter;         // 垂直解析度
    unsigned int biClrUsed;      // 調色盤顏色數
    unsigned int biClrImportant; // 重要顏色數
} BMPINFOHEADER;
#pragma pack(pop)

typedef unsigned char U8;
typedef long INT32;
typedef unsigned short INT16;
#define UCH(x) ((int)(x))
#define GET_2B(a, o) ((INT16)UCH(a[o]) + (((INT16)UCH(a[o + 1])) << 8))
#define GET_4B(a, o) ((INT32)UCH(a[o]) + (((INT32)UCH(a[o + 1])) << 8) + (((INT32)UCH(a[o + 2])) << 16) + (((INT32)UCH(a[o + 3])) << 24))

int ReadDataSize(const char *name);
void ReadImageData(const char *name,
                   U8 *bmpfh, U8 *bmpih,
                   U8 *colTbl, U8 *data);
void WriteImage(const char *name,
                U8 *bmpfh, U8 *bmpih,
                U8 *colTbl, U8 *data,
                int width, int height);
unsigned char *rotateNN(unsigned char *src,
                        int w, int h, int stride,
                        double angle_deg);

int main()
{
    BMPFILEHEADER fh;
    BMPINFOHEADER ih;
    U8 *colTbl = malloc(1024);
    int width, height, stride, dataSize;
    U8 *data, *outSpec, *outPhase;

    // 1. 讀圖
    ReadImageData("Fig0424(a).bmp", (U8 *)&fh, (U8 *)&ih, colTbl, NULL);
    U8 *pInfo = (U8 *)&ih;
    width = GET_4B(pInfo, 4);
    height = GET_4B(pInfo, 8);
    stride = ((width * 1 + 3) / 4) * 4;
    dataSize = stride * height;
    data = malloc(dataSize);
    ReadImageData("Fig0424(a).bmp",
                  (U8 *)&fh, (U8 *)&ih,
                  colTbl, data);

    // 旋轉影像
    U8 *rotatedData = rotateNN(data, width, height, stride, 45.0); // 45 度

    // 中心化並複製到 COMPLEX 陣列
    COMPLEX *c = malloc(sizeof(COMPLEX) * width * height);
    for (int y = 0; y < height; y++)
    {
        int row = y * stride;
        for (int x = 0; x < width; x++)
        {
            int idx = y * width + x;
            double val = rotatedData[row + x];
            if ((x + y) & 1)
                val = -val;
            c[idx].real = val;
            c[idx].imag = 0.0;
        }
    }

    // 3. 2D FFT
    //   注意：FFT2D(c, nx, ny, dir)，若影像寬高相同，可任意
    FFT2D(c, width, height, 1);

    // 4. 計算 spectrum & phase，並將結果對數／線性歸一至 [0,255]
    outSpec = malloc(dataSize);
    outPhase = malloc(dataSize);
    // 先找最大 magnitude (用於歸一化 log-scale)
    double maxMag = 0.0;
    for (int i = 0; i < width * height; i++)
    {
        double m = sqrt(c[i].real * c[i].real + c[i].imag * c[i].imag);
        if (m > maxMag)
            maxMag = m;
    }
    for (int y = 0; y < height; y++)
    {
        int row = y * stride;
        for (int x = 0; x < width; x++)
        {
            int idx = y * width + x;
            double re = c[idx].real;
            double im = c[idx].imag;

            // log magnitude
            double mag = log(1.0 + sqrt(re * re + im * im)) / log(1.0 + maxMag);
            int iv = (int)(mag * 255.0 + 0.5);
            outSpec[row + x] = (U8)(iv < 0 ? 0 : (iv > 255 ? 255 : iv));

            // phase in [−π,π] → shift to [0,255]
            double ph = atan2(im, re);
            int ip = (int)((ph / M_PI * 127.0) + 127.0 + 0.5);
            outPhase[row + x] = (U8)(ip < 0 ? 0 : (ip > 255 ? 255 : ip));
        }
    }

    // 5. 寫檔
    // WriteImage("p3_spectrum.bmp", (U8 *)&fh, (U8 *)&ih, colTbl, outSpec, width, height); 
    WriteImage("p3_phase.bmp", (U8 *)&fh, (U8 *)&ih, colTbl, outPhase, width, height);

    // 釋放
    free(data);
    free(c);
    free(outSpec);
    free(outPhase);
    free(colTbl);
    return 0;
}

// 讀檔頭+資訊頭+色表+像素
void ReadImageData(const char *name,
                   U8 *fh, U8 *ih,
                   U8 *colTbl, U8 *data)
{
    FILE *fp = fopen(name, "rb");
    if (!fp)
    {
        perror("fopen");
        exit(1);
    }
    fread(fh, 1, 14, fp);
    fread(ih, 1, 40, fp);
    int off = GET_4B(fh, 10);
    int ctSize = off - 14 - 40;
    if (colTbl)
        fread(colTbl, 1, ctSize, fp);
    int w = GET_4B(ih, 4), h = GET_4B(ih, 8);
    int st = ((w * 1 + 3) / 4) * 4;
    if (data)
    {
        fseek(fp, off, SEEK_SET);
        fread(data, 1, st * h, fp);
    }
    fclose(fp);
}

// 寫 BMP（含更新 size/image 大小）
void WriteImage(const char *name,
                U8 *fh, U8 *ih,
                U8 *colTbl, U8 *data,
                int width, int height)
{
    int st = ((width * 1 + 3) / 4) * 4;
    // 更新資訊頭
    memcpy(ih + 4, &width, 4);
    memcpy(ih + 8, &height, 4);
    int imgSize = st * height;
    memcpy(ih + 20, &imgSize, 4);
    int off = GET_4B(fh, 10);
    int fileSize = off + imgSize;
    memcpy(fh + 2, &fileSize, 4);

    FILE *fp = fopen(name, "wb");
    if (!fp)
    {
        perror("fopen");
        exit(1);
    }
    fwrite(fh, 1, 14, fp);
    fwrite(ih, 1, 40, fp);
    fwrite(colTbl, 1, off - 14 - 40, fp);
    fwrite(data, 1, imgSize, fp);
    fclose(fp);
}

// 最近鄰旋轉 (順時針 angle 度 = -angle 弧度)
unsigned char *rotateNN(unsigned char *src,
                        int w, int h, int stride,
                        double angle_deg)
{
    double thi = -angle_deg * M_PI / 180.0;
    double c = cos(thi), s = sin(thi);
    double cx = w / 2.0, cy = h / 2.0;
    unsigned char *out = malloc(stride * h);
    // 初始化為 0 (黑)
    memset(out, 0, stride * h);
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < w; x++)
        {
            // 對應回來源座標
            double dx = x - cx, dy = y - cy;
            double sx = dx * c + dy * s + cx;
            double sy = -dx * s + dy * c + cy;
            int ix = (int)(sx + 0.5);
            int iy = (int)(sy + 0.5);
            if (ix >= 0 && ix < w && iy >= 0 && iy < h)
            {
                out[y * stride + x] = src[iy * stride + ix];
            }
        }
    }
    return out;
}