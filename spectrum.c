#include<stdio.h>
#include<stdlib.h>
#include<math.h>
#include"fft.h"
#pragma warning (disable:4996)
typedef long INT32;
typedef unsigned short int INT16;
typedef unsigned char U_CHAR;

#define UCH(x)	((int) (x))
#define GET_2B(array,offset)  ((INT16) UCH(array[offset]) + \
			       (((INT16) UCH(array[offset+1])) << 8))
#define GET_4B(array,offset)  ((INT32) UCH(array[offset]) + \
			       (((INT32) UCH(array[offset+1])) << 8) + \
			       (((INT32) UCH(array[offset+2])) << 16) + \
			       (((INT32) UCH(array[offset+3])) << 24))
#define FREAD(file,buf,sizeofbuf)  \
  ((size_t) fread((void *) (buf), (size_t) 1, (size_t) (sizeofbuf), (file)))

#define	PI	3.14159

int ReadDataSize(char *name);
void ReadImageData(char *name, U_CHAR *bmpfileheader, U_CHAR *bmpinfoheader, U_CHAR *color_table, U_CHAR *data);


int main()
{
   FILE *output_file = 0 ;
   
   U_CHAR bmpfileheader1[14] = { 0 } ;
   U_CHAR bmpinfoheader1[40] = { 0 } ;
   U_CHAR *data1, *data2, *data3, color_table1[1024];
   COMPLEX *c, *c2;

 
   INT32 biWidth = 0 ;		
   INT32 biHeight = 0 ;

   int i, j, k, temp, biWidth4;
   int histo_table[256] = { 0 };
   double max, D, H;
   int D0=15, n=2;

   //rotate.bmp為Fig0424(a).bmp 旋轉45度
   i = ReadDataSize("rotate.bmp");
   data1 = (U_CHAR *)malloc( i );
   if (data1 == NULL) {
      exit(0);
   }
  
   data2 = (U_CHAR *)malloc( i );
   if (data2 == NULL) {
      exit(0);
   }

   data3 = (U_CHAR *)malloc( i );
   if (data3 == NULL) {
      exit(0);
   }

   ReadImageData("rotate.bmp", bmpfileheader1, bmpinfoheader1, color_table1, data1);
   biWidth           =   GET_4B(bmpinfoheader1,4);
   biHeight          =   GET_4B(bmpinfoheader1,8);


   c = (COMPLEX *)malloc( biHeight*biWidth*sizeof(COMPLEX) );
   if (c == NULL) {
      exit(0);
   }
   c2 = (COMPLEX *)malloc( biHeight*biWidth*sizeof(COMPLEX) );
   if (c2 == NULL) {
      exit(0);
   }

   biWidth4 = ((biWidth+3)/4 *4);

   // Process the file
   // centering每個像速成上-1^(i+1)
   for (i=0; i < biHeight; i++)
   {
       k = i* biWidth4;
       for (j=0; j < biWidth; j++)
       {
		   c[k].real = data1[k];
		   c[k].imag = 0.0;
		   if ((i+j)%2 == 1)
				c[k].real = 0.0-c[k].real;

           k = k+1;
       }
   }

   if (!FFT2D(c, biHeight, biWidth, 1))
   {
	   printf("Stop!\n");
	   exit(0);
   }

   //


   // Spectrum
   // find the maximum
   max = 0.0;
   for (i=0; i < biHeight; i++)
   {
       k = i* biWidth4;
       for (j=0; j < biWidth; j++)
       {
		   if (sqrt(c[k].real*c[k].real+c[k].imag*c[k].imag) > max)
			   max = sqrt(c[k].real*c[k].real+c[k].imag*c[k].imag);

           k = k+1;
       }
   }
   printf("max=%f\n", max);
   //
   for (i=0; i < biHeight; i++)
   {
       k = i* biWidth4;
       for (j=0; j < biWidth; j++)
       {
		   temp = (int) (30*log(1+10000*sqrt(c[k].real*c[k].real+c[k].imag*c[k].imag)/max));
		   if (temp > 255)	temp = 255;
		   if (temp < 0) temp = 0;
		   data2[k] = (U_CHAR) temp;

           k = k+1;
       }
   }


   // Phase
   //
   for (i=0; i < biHeight; i++)
   {
       k = i* biWidth4;
       for (j=0; j < biWidth; j++)
       {
			   
		   temp = (int) (127*atan2(c[k].imag, c[k].real)/PI+127);
		   if (temp > 255)	temp = 255;
		   if (temp < 0) temp = 0;
			data3[k] = (U_CHAR) temp;

			k = k+1;
       }
   }

   // Spectrum:創立新圖spectrum.bmp
   /* 開啟新檔案 */
   if( ( output_file = fopen("spectrum.bmp","wb") ) == NULL ){
      fprintf(stderr,"Output file can't open.\n");
      exit(0);
   }

   fwrite(bmpfileheader1, sizeof(bmpfileheader1), 1, output_file);
   fwrite(bmpinfoheader1, sizeof(bmpinfoheader1), 1, output_file);

   fwrite(color_table1, 1024, 1, output_file);
 
   fwrite(data2, ((biWidth*1 +3)/4 *4)*biHeight*1, 1, output_file);
 
   fclose (output_file);

   // Phase:創立新圖片phase.bmp
   /* 開啟新檔案 */
   if( ( output_file = fopen("phase.bmp","wb") ) == NULL ){
      fprintf(stderr,"Output file can't open.\n");
      exit(0);
   }

   fwrite(bmpfileheader1, sizeof(bmpfileheader1), 1, output_file);
   fwrite(bmpinfoheader1, sizeof(bmpinfoheader1), 1, output_file);

   fwrite(color_table1, 1024, 1, output_file);
 
   fwrite(data3, ((biWidth*1 +3)/4 *4)*biHeight*1, 1, output_file);
 
   fclose (output_file);

   free(data1);
   free(data2);
   free(data3);
   free(c);
   free(c2);

  
   return 0;
}

int ReadDataSize(char *name)
{
   FILE *input_file = 0 ;
   U_CHAR bmpfileheader[14] = { 0 } ;
   U_CHAR bmpinfoheader[40] = { 0 } ;
   
   INT32 biWidth = 0 ;		
   INT32 biHeight = 0 ;
   INT16 BitCount = 0 ;

   /* 開啟檔案 */
   if( ( input_file = fopen(name,"rb") ) == NULL ){
      fprintf(stderr,"File can't open.\n");
      exit(0);
   }

   FREAD(input_file,bmpfileheader,14);
   FREAD(input_file,bmpinfoheader,40);

   if (GET_2B(bmpfileheader,0) == 0x4D42) /* 'BM' */
      fprintf(stdout,"BMP file.\n");
   else{
      fprintf(stdout,"Not bmp file.\n");
      exit(0);
   }

   biWidth           =   GET_4B(bmpinfoheader,4);
   biHeight          =   GET_4B(bmpinfoheader,8);
   BitCount          =   GET_2B(bmpinfoheader,14);

   if (BitCount != 8) {
      fprintf(stderr,"Not a 8-bit file.\n");
      fclose (input_file);
      exit(0);
   }

   // 
   fclose (input_file);

   return ((biWidth*1 +3)/4 *4)*biHeight*1;
}

void ReadImageData(char *name, U_CHAR *bmpfileheader, U_CHAR *bmpinfoheader, U_CHAR *color_table, U_CHAR *data)
{
   FILE *input_file = 0 ;
   
   INT32 FileSize = 0 ;
   INT32 bfOffBits =0 ;
   INT32 headerSize =0 ;
   INT32 biWidth = 0 ;		
   INT32 biHeight = 0 ;
   INT16 biPlanes = 0 ;
   INT16 BitCount = 0 ;
   INT32 biCompression = 0 ;
   INT32 biImageSize = 0;
   INT32 biXPelsPerMeter = 0 ,biYPelsPerMeter = 0 ;
   INT32 biClrUsed = 0 ;
   INT32 biClrImp = 0 ;

   /* 開啟檔案 */
   if( ( input_file = fopen(name,"rb") ) == NULL ){
      fprintf(stderr,"File can't open.\n");
      exit(0);
   }

   FREAD(input_file,bmpfileheader,14);
   FREAD(input_file,bmpinfoheader,40);

   if (GET_2B(bmpfileheader,0) == 0x4D42) /* 'BM' */
      fprintf(stdout,"BMP file.\n");
   else{
      fprintf(stdout,"Not bmp file.\n");
      exit(0);
   }

   FileSize           =   GET_4B(bmpfileheader,2);
   bfOffBits         =   GET_4B(bmpfileheader,10);
   headerSize      =   GET_4B(bmpinfoheader,0);
   biWidth           =   GET_4B(bmpinfoheader,4);
   biHeight          =   GET_4B(bmpinfoheader,8);
   biPlanes          =   GET_2B(bmpinfoheader,12);
   BitCount          =   GET_2B(bmpinfoheader,14);
   biCompression   =   GET_4B(bmpinfoheader,16);
   biImageSize      =   GET_4B(bmpinfoheader,20);
   biXPelsPerMeter =   GET_4B(bmpinfoheader,24);
   biYPelsPerMeter =   GET_4B(bmpinfoheader,28);
   biClrUsed         =   GET_4B(bmpinfoheader,32);
   biClrImp          =   GET_4B(bmpinfoheader,36);

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
	"ColorsImportant = %ld \n",FileSize,bfOffBits,headerSize,biWidth,biHeight,biPlanes,
	BitCount,biCompression,biImageSize,biXPelsPerMeter,biYPelsPerMeter,biClrUsed,biClrImp);

   if (BitCount != 8) {
      fprintf(stderr,"Not a 8-bit file.\n");
      fclose (input_file);
      exit(0);
   }

   FREAD(input_file,color_table,1024);

   //
   
   //
   fseek(input_file,bfOffBits,SEEK_SET);
   FREAD(input_file,data,((biWidth*1 +3)/4 *4)*biHeight*1);
   // 
   fclose (input_file);
}

//void padding(