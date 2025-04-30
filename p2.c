#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#define M_PI 3.14159265358979323846
#pragma pack(push,1)
// BMP 檔頭 (14 bytes)
typedef struct {
    unsigned short bfType;     // 'BM' = 0x4D42
    unsigned int   bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int   bfOffBits;
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

// 讀 8-bit BMP，記得呼叫方要 free colorTable、data
void readBMP(const char *fname,
             BMPFileHeader *fh,
             BMPInfoHeader *ih,
             unsigned char **colorTable,
             unsigned char **data)
{
    FILE *fp = fopen(fname, "rb");
    if (!fp) { perror("fopen"); exit(1); }
    fread(fh, 1, sizeof(*fh), fp);
    fread(ih, 1, sizeof(*ih), fp);
    if (fh->bfType!=0x4D42 || ih->biBitCount!=8) {
        fprintf(stderr,"只支援 8-bit BMP\n"); exit(1);
    }
    int ctSize = fh->bfOffBits - sizeof(*fh) - sizeof(*ih);
    *colorTable = malloc(ctSize);
    fread(*colorTable,1,ctSize,fp);
    int w = ih->biWidth, h = abs(ih->biHeight);
    int stride = ((w+3)/4)*4;
    *data = malloc(stride*h);
    fseek(fp, fh->bfOffBits, SEEK_SET);
    fread(*data,1,stride*h,fp);
    fclose(fp);
}

// 寫 8-bit BMP (header + colour table + data)
void writeBMP(const char *fname,
              BMPFileHeader *fh,
              BMPInfoHeader *ih,
              unsigned char *colorTable,
              unsigned char *data)
{
    FILE *fp = fopen(fname,"wb");
    if (!fp) { perror("fopen"); exit(1); }
    fwrite(fh,1,sizeof(*fh),fp);
    fwrite(ih,1,sizeof(*ih),fp);
    int ctSize = fh->bfOffBits - sizeof(*fh) - sizeof(*ih);
    fwrite(colorTable,1,ctSize,fp);
    int w = ih->biWidth, h = abs(ih->biHeight);
    int stride = ((w+3)/4)*4;
    fwrite(data,1,stride*h,fp);
    fclose(fp);
}

// 最近鄰旋轉 (順時針 angle 度 = -angle 弧度)
unsigned char* rotateNN(unsigned char *src,
                        int w, int h, int stride,
                        double angle_deg)
{
    double thi = -angle_deg * M_PI/180.0;
    double c = cos(thi), s = sin(thi);
    double cx = w/2.0, cy = h/2.0;
    unsigned char *out = malloc(stride*h);
    // 初始化為 0 (黑)
    memset(out,0,stride*h);
    for(int y=0; y<h; y++){
        for(int x=0; x<w; x++){
            // 對應回來源座標
            double dx = x - cx, dy = y - cy;
            double sx =  dx*c + dy*s + cx;
            double sy = -dx*s + dy*c + cy;
            int ix = (int)(sx + 0.5);
            int iy = (int)(sy + 0.5);
            if(ix>=0 && ix<w && iy>=0 && iy<h){
                out[y*stride + x] = src[iy*stride + ix];
            }
        }
    }
    return out;
}

// 逐像素相乘 /255，並 clamp [0,255]
unsigned char* multiplyMoire(unsigned char *a,
                             unsigned char *b,
                             int w, int h, int stride)
{
    unsigned char *out = malloc(stride*h);
    for(int i=0; i<stride*h; i++){
        int v = a[i]*b[i]/255;
        out[i] = (unsigned char)(v<0?0:(v>255?255:v));
    }
    return out;
}

int main(void)
{
    BMPFileHeader fh;
    BMPInfoHeader ih;
    unsigned char *colorTable = NULL, *orig = NULL;

    // 1. 讀入原圖 Lines.bmp
    readBMP("Lines.bmp", &fh, &ih, &colorTable, &orig);
    int w = ih.biWidth;
    int h = abs(ih.biHeight);
    int stride = ((w+3)/4)*4;

    // 2. 順時針 5° 旋轉，並寫檔備份
    unsigned char *rot5 = rotateNN(orig, w, h, stride, 5.0);
    writeBMP("p2_a.bmp", &fh, &ih, colorTable, rot5);

    // 3. 原圖 × 旋轉後 圖，輸出 Moire
    unsigned char *moire = multiplyMoire(orig, rot5, w, h, stride);
    writeBMP("p2_b.bmp", &fh, &ih, colorTable, moire);

    // 釋放
    free(orig);
    free(rot5);
    free(moire);
    free(colorTable);

    return 0;
}
