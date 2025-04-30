#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#pragma warning(disable:4996)

typedef long INT32;
typedef unsigned short int INT16;
typedef unsigned char U_CHAR;

#define M_PI 3.14159265358979323846
#define UCH(x)	((int) (x))
#define GET_2B(array,offset)  ((INT16) UCH(array[offset]) + \
			       (((INT16) UCH(array[offset+1])) << 8))
#define GET_4B(array,offset)  ((INT32) UCH(array[offset]) + \
			       (((INT32) UCH(array[offset+1])) << 8) + \
			       (((INT32) UCH(array[offset+2])) << 16) + \
			       (((INT32) UCH(array[offset+3])) << 24))
#define FREAD(file,buf,sizeofbuf)  \
  ((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))

int ReadDataSize(char* name);
void ReadImageData(char* name, U_CHAR* bmpfileheader, U_CHAR* bmpinfoheader, U_CHAR* color_table, U_CHAR* data);
void RotateImage(U_CHAR* srcData, INT32 biWidth, INT32 biHeight, U_CHAR* dstData, double angle);

int main()
{
	FILE* output_file = 0;

	U_CHAR bmpfileheader1[14] = { 0 };
	U_CHAR bmpinfoheader1[40] = { 0 };
	U_CHAR* data1, * data2, * new_data, color_table1[1024];


	INT32 biWidth = 0;
	INT32 biHeight = 0;

	int i, j, k, temp, biWidth4, sum, i2, j2;
	int histo_table[256] = { 0 };

	i = ReadDataSize("Fig0424(a).bmp");
	data1 = (U_CHAR*)malloc(i);
	if (data1 == NULL) {
		exit(0);
	}

	ReadImageData("Fig0424(a).bmp", bmpfileheader1, bmpinfoheader1, color_table1, data1);
	biWidth = GET_4B(bmpinfoheader1, 4);
	biHeight = GET_4B(bmpinfoheader1, 8);

	//
	//i = ReadDataSize("new2.bmp");
	data2 = (U_CHAR*)malloc(biWidth * biHeight);
	if (data2 == NULL) {
		exit(0);
	}

	//rotate the image by -21 degrees
	RotateImage(data1, biWidth, biHeight, data2, 21.0);

	//
	/* 開啟新檔案 */
	if ((output_file = fopen("rotated.bmp", "wb")) == NULL) {
		fprintf(stderr, "Output file can't open.\n");
		exit(0);
	}

	fwrite(bmpfileheader1, sizeof(bmpfileheader1), 1, output_file);
	fwrite(bmpinfoheader1, sizeof(bmpinfoheader1), 1, output_file);

	fwrite(color_table1, 1024, 1, output_file);

	fwrite(data2, biWidth * biHeight, 1, output_file);

	fclose(output_file);

	free(data1);
	free(data2);

	return 0;
}

int ReadDataSize(char* name)
{
	FILE* input_file = 0;
	U_CHAR bmpfileheader[14] = { 0 };
	U_CHAR bmpinfoheader[40] = { 0 };

	INT32 biWidth = 0;
	INT32 biHeight = 0;
	INT16 BitCount = 0;

	/* 開啟檔案 */
	if ((input_file = fopen(name, "rb")) == NULL) {
		fprintf(stderr, "File can't open.\n");
		exit(0);
	}

	FREAD(input_file, bmpfileheader, 14);
	FREAD(input_file, bmpinfoheader, 40);

	if (GET_2B(bmpfileheader, 0) == 0x4D42) /* 'BM' */
		fprintf(stdout, "BMP file.\n");
	else {
		fprintf(stdout, "Not bmp file.\n");
		exit(0);
	}

	biWidth = GET_4B(bmpinfoheader, 4);
	biHeight = GET_4B(bmpinfoheader, 8);
	BitCount = GET_2B(bmpinfoheader, 14);

	if (BitCount != 8) {
		fprintf(stderr, "Not a 8-bit file.\n");
		fclose(input_file);
		exit(0);
	}

	// 
	fclose(input_file);

	return biWidth * biHeight;
}

void ReadImageData(char* name, U_CHAR* bmpfileheader, U_CHAR* bmpinfoheader, U_CHAR* color_table, U_CHAR* data)
{
	FILE* input_file = 0;

	INT32 FileSize = 0;
	INT32 bfOffBits = 0;
	INT32 headerSize = 0;
	INT32 biWidth = 0;
	INT32 biHeight = 0;
	INT16 biPlanes = 0;
	INT16 BitCount = 0;
	INT32 biCompression = 0;
	INT32 biImageSize = 0;
	INT32 biXPelsPerMeter = 0, biYPelsPerMeter = 0;
	INT32 biClrUsed = 0;
	INT32 biClrImp = 0;

	/* 開啟檔案 */
	if ((input_file = fopen(name, "rb")) == NULL) {
		fprintf(stderr, "File can't open.\n");
		exit(0);
	}

	FREAD(input_file, bmpfileheader, 14);
	FREAD(input_file, bmpinfoheader, 40);

	if (GET_2B(bmpfileheader, 0) == 0x4D42) /* 'BM' */
		fprintf(stdout, "BMP file.\n");
	else {
		fprintf(stdout, "Not bmp file.\n");
		exit(0);
	}

	FileSize = GET_4B(bmpfileheader, 2);
	bfOffBits = GET_4B(bmpfileheader, 10);
	headerSize = GET_4B(bmpinfoheader, 0);
	biWidth = GET_4B(bmpinfoheader, 4);
	biHeight = GET_4B(bmpinfoheader, 8);
	biPlanes = GET_2B(bmpinfoheader, 12);
	BitCount = GET_2B(bmpinfoheader, 14);
	biCompression = GET_4B(bmpinfoheader, 16);
	biImageSize = GET_4B(bmpinfoheader, 20);
	biXPelsPerMeter = GET_4B(bmpinfoheader, 24);
	biYPelsPerMeter = GET_4B(bmpinfoheader, 28);
	biClrUsed = GET_4B(bmpinfoheader, 32);
	biClrImp = GET_4B(bmpinfoheader, 40);

	printf("FileSize = %ld \n"
		"DataOffset = %ld \n"
		"HeaderSize = %ld \n"
		"Width = %ld \n"
		"Height = %ld \n"
		"Planes = %d \n"
		"BitCount = %d \n"
		"Compression = %ld \n"
		"ImageSize = %ld \n"
		"XpixelsPerM = %ld \n"
		"YpixelsPerM = %ld \n"
		"ColorsUsed = %ld \n"
		"ColorsImportant = %ld \n", FileSize, bfOffBits, headerSize, biWidth, biHeight, biPlanes,
		BitCount, biCompression, biImageSize, biXPelsPerMeter, biYPelsPerMeter, biClrUsed, biClrImp);

	if (BitCount != 8) {
		fprintf(stderr, "Not a 8-bit file.\n");
		fclose(input_file);
		exit(0);
	}

	FREAD(input_file, color_table, 1024);

	//

	//
	fseek(input_file, GET_4B(bmpfileheader, 10), SEEK_SET);
	FREAD(input_file, data, GET_4B(bmpinfoheader, 20));
	// 
	fclose(input_file);
}

void RotateImage(U_CHAR* srcData, INT32 biWidth, INT32 biHeight, U_CHAR* dstData, double angle) {
	double radians = angle * M_PI / 180.0;
	double cosine = cos(radians);
	double sine = sin(radians);

	double centerX = biWidth / 2.0;
	double centerY = biHeight / 2.0;

	for (int y = 0; y < biHeight; y++) {
		for (int x = 0; x < biWidth; x++) {
			double offsetX = x - centerX;//新原點:x座標
			double offsetY = y - centerY;//新原點:y座標
			double srcX = offsetX * cosine - offsetY * sine + centerX;//轉換原點
			double srcY = offsetX * sine + offsetY * cosine + centerY;

			//找最相鄰的點
			int nearestX = (int)(srcX + 0.5);
			int nearestY = (int)(srcY + 0.5);

			// 確認轉換後的點是不是再圖裡(1024*1024)
			if (nearestX >= 0 && nearestX < biWidth && nearestY >= 0 && nearestY < biHeight) {
				// 放入新圖
				dstData[y * biWidth + x] = srcData[nearestY * biWidth + nearestX];
			}
			else {
				dstData[y * biWidth + x] = 0; // 超出圖，值設為0
			}
		}
	}

}
