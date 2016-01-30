/*

alpha2ds

Tool for conversion of pc graphics files (with alpha like png) 
to a custom 16bit format for use on embedded devices.

===========================================================================================================
Format Image File 4bit:
1. RAW greyscale image data, 4 bits per pixel

Format Image File 8bit:
1. 32bit-word number of 8bit words comprising the data (header excluded)
2. 16bit-word dimension X in pixels
3. 16bit-word dimension Y in pixels
4. 16bit-word configuration data
     Bit0 = 0 uncompressed
	 Bit1 = 1 RLE compressed
5. Data to EOF (8bit)
	 - Either raw or RLE compressed (8-bit wise)
	 - index 0 = transparent

Format Palette File 8bit (raw):
1. 256 x 16bit RGB555 Color entry (bit 15 unused)
   (entry 0 unused)

Format Image File 16bit:
1. 32bit-word number of 16bit words comprising the data (header excluded)
2. 16bit-word dimension X in pixels
3. 16bit-word dimension Y in pixels
4. 16bit-word configuration data
     Bit0 = 0 uncompressed
	 Bit1 = 1 RLE compressed
5. Data to EOF 
   - Either raw or RLE compressed (16-bit wise) according to Bit0 of config word
   - Starting with the placeholder value if RLE compressed 
   - RGB555 Data
   - Big-endian
   - Bit16 = 0 - Pixel completely transparent (Alpha == 0)
   - Bit16 = 1 - Pixel is not completely transparent (Alpha != 0)

Format Alpha File:
1. 32bit-word number of 8bit words comprising the data (header excluded)
2. 16bit-word dimension X in pixels
3. 16bit-word dimension Y in pixels
4. 16bit-word configuration data
     Bit0 = 0 uncompressed
	 Bit0 = 1 RLE compressed
5. Data to EOF (8bit)
	 - Either raw or RLE compressed (8-bit wise)
     - Containts 8bit alpha data for each pixel

Format RGB444 Packed File:
1. 32bit-word number of 16bit words comprising the data (header excluded)
2. 16bit-word dimension X in pixels
3. 16bit-word dimension Y in pixels
4. 16bit-word configuration data
     Bit0 = 0 uncompressed

5. Data to EOF 
	4 bits RGB packed so that 2 pixels fit in 3 bytes
	  
	Byte   |   1    |   2    |   3    |
	Color   BBBBGGGG RRRRBBBB GGGGRRRR
	Pixel   11111111 11112222 22222222

Format BGR565:
1. 32bit-word number of 16bit words comprising the data (header excluded)
2. 16bit-word dimension X in pixels
3. 16bit-word dimension Y in pixels
4. 16bit-word configuration data
     Bit0 = 0 uncompressed
	 Bit0 = 1 compressed

5. Data to EOF 
	16 bits BGR 
	  
	Byte   |   1    |   2    | 
	Color   BBBBBGGG GGGRRRRR 
	

===========================================================================================================
Bjoern Seip
TURBO D3 GMBH
www.td3.com
Copyright (C) 2010
===========================================================================================================



The following open source products where used (but not hurt) in the making of this tool:

===========================================================================================================
FreeImage library V 3.9.1
http://freeimage.sourceforge.net/

Under the terms of FreeImage Public License
===========================================================================================================


===========================================================================================================
Basic Compression Library V 1.2 (RLE Compression algorithm only) 
http://bcl.sourceforge.net/
Author:      Marcus Geelnard
Description: RLE coder/decoder interface.

License:
 
Copyright (c) 2003-2006 Marcus Geelnard
This software is provided ’as-is’, without any express or implied warranty. In no event will the authors
be held liable for any damages arising from the use of this software.
Permission is granted to anyone to use this software for any purpose, including commercial
applications, and to alter it and redistribute it freely, subject to the following restrictions:
1. The origin of this software must not be misrepresented; you must not claim that you wrote the
original software. If you use this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be misrepresented as being
the original software.
3. This notice may not be removed or altered from any source distribution.

-----------------------------------------------------------------------------------------------------------
AUTHORS REMARK: The code contained in this software has been altered against Marcus' version.
There are now an 8-bit and a 16-bit version of the RLE compression and decompression routines.
Otherwise the code is unaltered.
===========================================================================================================

*/

#include "stdafx.h"

#include <assert.h>
#include <stdio.h>
#include <io.h>
#include <string.h>
#include <stdlib.h>

#include "FreeImage.h"
#include "rle.h"

#ifndef MAX_PATH
#define MAX_PATH	260
#endif

#define FI16_555_RED_MASK		0x7C00
#define FI16_555_GREEN_MASK		0x03E0
#define FI16_555_BLUE_MASK		0x001F
#define FI16_555_RED_SHIFT		10
#define FI16_555_GREEN_SHIFT	5
#define FI16_555_BLUE_SHIFT		0

#define RGB555(b, g, r) ((((b) >> 3) << FI16_555_BLUE_SHIFT) | (((g) >> 3) << FI16_555_GREEN_SHIFT) | (((r) >> 3) << FI16_555_RED_SHIFT))


#define FI16_444_RED_MASK		0x7C00
#define FI16_444_GREEN_MASK		0x03E0
#define FI16_444_BLUE_MASK		0x001F
#define FI16_444_RED_SHIFT		0
#define FI16_444_GREEN_SHIFT	4
#define FI16_444_BLUE_SHIFT		8

#define RGB444(b, g, r) ((((b) >> 4) << FI16_444_BLUE_SHIFT) | (((g) >> 4) << FI16_444_GREEN_SHIFT) | (((r) >> 4) << FI16_444_RED_SHIFT))



#define FI16_565_RED_MASK		0xF800
#define FI16_565_GREEN_MASK		0x07C0
#define FI16_565_BLUE_MASK		0x001F
#define FI16_565_RED_SHIFT		0
#define FI16_565_GREEN_SHIFT	6
#define FI16_565_BLUE_SHIFT		11

#define BGR565(r, g, b) ((((b) >> 3) << FI16_565_BLUE_SHIFT) | (((g) >> 3) << FI16_565_GREEN_SHIFT) | (((r) >> 3) << FI16_565_RED_SHIFT))

#define FI16_R565_RED_MASK		0xF800
#define FI16_R565_GREEN_MASK	0x07E0
#define FI16_R565_BLUE_MASK		0x001F
#define FI16_R565_RED_SHIFT		11
#define FI16_R565_GREEN_SHIFT	6
#define FI16_R565_BLUE_SHIFT	0

#define RGB565(r, g, b) ((((b) >> 3) << FI16_R565_BLUE_SHIFT) | (((g) >> 3) << FI16_R565_GREEN_SHIFT) | (((r) >> 3) << FI16_R565_RED_SHIFT))


#define RGBFROMBE555(v) (unsigned short int)( (((unsigned short int)v & 0x7C00) >> 10) | (((unsigned short int)v & 0x3E0) << 1) | (((unsigned short int)v & 0x1F) << 11) )


//=======================================================
// max
//=======================================================

unsigned char max(unsigned char a, unsigned char b)
{
	if (a > b) return a;
	return b;
}

//=======================================================
// Parm
//=======================================================

enum OutputWidth
{
	OutputWidth1Bit,
	OutputWidth8Bit,
	OutputWidth16Bit,
	OutputWidth4Bit,
	OutputWidth3x4Bit

};

struct ALPHA2DSPARMS
{
	bool optQuiet;
	bool optHelp;
	bool optAlphaExternal;
	bool optAlphaInternal;
	bool optDebug;
	bool optRLE;
	bool optTile;
	bool optAlphaTransparent;
	bool optNoHeader;
	bool optWidthmap;
	bool optBGR565;
	bool optRGB565;
	unsigned short int TileSize;
	OutputWidth OutputWidth;
	char ExtensionImage[MAX_PATH];
	char ExtensionAlpha[MAX_PATH];
	char Palettepath[MAX_PATH];
	char Filefilter[MAX_PATH];

} Parm;

//=======================================================
// parminit
//=======================================================
void parminit()
{
	Parm.optHelp = 0;
	Parm.optQuiet = 0;
	Parm.optAlphaExternal = 0;
	Parm.optAlphaInternal = 0;
	Parm.optDebug = 0;
	Parm.optRLE = 0;
	Parm.optTile = 0;
	Parm.optNoHeader = false;
	Parm.optBGR565 = false;
	Parm.TileSize = 8;
	Parm.OutputWidth = OutputWidth16Bit;
	Parm.optAlphaTransparent = 0;
	Parm.optWidthmap = false;
	Parm.optRGB565 = false;
	strcpy(Parm.ExtensionImage,"bin");
	strcpy(Parm.ExtensionAlpha,"bin");
	strcpy(Parm.Filefilter,"*.png");
	strcpy(Parm.Palettepath ,"");
}
//=======================================================
// showsyntax
//=======================================================
void showsyntax ( void ) 
{
	printf("\nalpha2ds image converter 1.0\n\n");
	printf("by Bjoern Seip for TURBO D3 GMBH (c) 2006\n\n");
	printf(FreeImage_GetCopyrightMessage());
	printf("\n");
	printf("Basic Compression Library 1.20\n\n");
	printf("Usage:   alpha2ds [-f filter (default: *.png)]\n");
	printf("                  [-e extension for image file (default: .bin)]\n");
	printf("                  [-g extension for alpha file (default: .bin)]\n");
	printf("                  [-p palette file for import/export]\n");
	printf("                  [options]\n\n"); 
	printf("Options: -a   output separate alpha files\n");
//	printf("         -i   embed alpha information\n");
	printf("         -r   compress output by RLE\n");
	printf("         -1   make 1 bit file using alpha value\n");
	printf("         -8   make 8 bit file and optimal palette (cut after 256 colors)\n");
	printf("         -4   make 4 bit greyscale file\n");
	printf("         -5   make RGB444 packed file without transparency\n");
	printf("         -6   make BGR565 file without transparency \n");
	printf("         -7   make RGB565 file without transparency \n");
	printf("         -t   make tiles (default: off)\n");
	printf("         -d   tilesize (must be even, default: 8)\n");
	printf("         -c   alpha pixels fully transparent\n");
	printf("         -w   write width file for tiles (requires -t)\n");
	printf("         -n   no header output\n");
	printf("         -q   quiet operation\n");
	printf("         -v   print verbose information\n");
	printf("         -h   print this\n\n");

	printf("Default Operation:\n");
	printf("          Make 16bit RGB555 files from all *.png files in current directory,\n");
	printf("          set transparency bit according to input data\n");
}

//=======================================================
// check2args
//=======================================================
int check2args(int argc, int i, char *next, char *message)
{
	if (argc == (i + 1)){
		if (!Parm.optQuiet) printf("%s\n",message);
		return(0);
	}
	else {
	 if ( next[0] == '-' ) {
			if (!Parm.optQuiet) printf("%s\n",message);
			return(0);
	}
	}
	return(1);
}

//=======================================================
// processcmdline  
//=======================================================
int processcmdline (int argc, char *argv[])
{
	int result = 1;

	for (int i=1; i<argc; i++)
	{
	 
	 if ( (argv[i][0] != '-') || (strlen(argv[i]) < 2 ) ){
	  if (!Parm.optQuiet)	printf("invalid argument %s\n",argv[i]);
		 result = 0;
	 } else

	 switch (argv[i][1])
	 {

	  case 'e':
		  if (check2args(argc, i, argv[i+1], "-e must be followed by a file extension (Default: bin")) {
				strncpy(Parm.ExtensionImage,argv[i+1],5);
				i++;
		 } else result = 0;
		 break;

	  case 'g':
		  if (check2args(argc, i, argv[i+1], "-g must be followed by a file extension (Default: alpha.bin")) {
				strncpy(Parm.ExtensionAlpha,argv[i+1],5);
				i++;
		 } else result = 0;
		 break;

	  case 'f':
		  if (check2args(argc, i, argv[i+1], "-f must be followed by a filter expression (Default: *.png)")) {
				strncpy(Parm.Filefilter,argv[i+1],MAX_PATH);
				i++;
		 } else result = 0;
		 break;

	  case 'p':
		  if (check2args(argc, i, argv[i+1], "-p must be followed by a valid file path")) {
				strncpy(Parm.Palettepath,argv[i+1],MAX_PATH);
				i++;
		 } else result = 0;
		 break;

	  case 'd':
		  if (check2args(argc, i, argv[i+1], "-d must be followed by an even integer number <= 64")) {
				Parm.TileSize = atoi(argv[i+1]);
				i++;
		 } else result = 0;
		 break;

	  case 'c': 
		  Parm.optAlphaTransparent = 1;
		 break;

	  case 't': 
		  Parm.optTile = 1;
		 break;

	  case '8': 
		 Parm.OutputWidth = OutputWidth8Bit;
		 break;

	  case '1': 
		 Parm.OutputWidth = OutputWidth1Bit;
		 break;

	  case '4': 
		 Parm.OutputWidth = OutputWidth4Bit;
		 break;

	  case '5': 
		 Parm.OutputWidth = OutputWidth3x4Bit;
		 break;

	  case '6': 
		 Parm.optBGR565 = true;
		 break;		 

	  case '7': 
		  Parm.optRGB565 = true;
		 break;		 

	  case 'w': 
		  Parm.optWidthmap = 1;
		 break;

	  case 'v': 
		 Parm.optDebug  = 1;
		 break;

	  case 'a': 
		 Parm.optAlphaExternal = 1;
		 break;

	  case 'n': 
		 Parm.optNoHeader = true;
		 break;

	  case 'i': 
		 Parm.optAlphaInternal = 1;
		 break;

	  case 'r': 
		 Parm.optRLE = 1;
		 break;

	  case 'h': 
		 Parm.optHelp = 1;
		 break;

	  default:
	 	 if (!Parm.optQuiet) printf("invalid argument %s\n",argv[i]);
		 result = 0;

	 }
	}

	return (result);
}


//=======================================================
// GenericLoader
//=======================================================
/** Generic image loader
	@param lpszPathName Pointer to the full file name
	@param flag Optional load flag constant
	@return Returns the loaded dib if successful, returns NULL otherwise
*/
FIBITMAP* GenericLoader(const char* lpszPathName, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

	// check the file signature and deduce its format
	// (the second argument is currently not used by FreeImage)
	fif = FreeImage_GetFileType(lpszPathName, 0);
	if(fif == FIF_UNKNOWN) {
		// no signature ?
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName);
	}
	// check that the plugin has reading capabilities ...
	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		// ok, let's load the file
		FIBITMAP *dib = FreeImage_Load(fif, lpszPathName, flag);
		// unless a bad file format, we are done !
		return dib;
	}
	return NULL;
}

//=======================================================
// GenericWriter
//=======================================================
/** Generic image writer
	@param dib Pointer to the dib to be saved
	@param lpszPathName Pointer to the full file name
	@param flag Optional save flag constant
	@return Returns true if successful, returns false otherwise
*/
bool GenericWriter(FIBITMAP* dib, const char* lpszPathName, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	BOOL bSuccess = FALSE;

	if(dib) {
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName);
		if(fif != FIF_UNKNOWN ) {
			// check that the plugin has sufficient writing and export capabilities ...
			WORD bpp = FreeImage_GetBPP(dib);
			if(FreeImage_FIFSupportsWriting(fif) && FreeImage_FIFSupportsExportBPP(fif, bpp)) {
				// ok, we can save the file
				bSuccess = FreeImage_Save(fif, dib, lpszPathName, flag);
				// unless an abnormal bug, we are done !
			}
		}
	}
	return (bSuccess == TRUE) ? true : false;
}

//=======================================================
// FreeImageErrorHandler
//=======================================================
/**
	FreeImage error handler
	@param fif Format / Plugin responsible for the error 
	@param message Error message
*/
void FreeImageErrorHandler(FREE_IMAGE_FORMAT fif, const char *message) {
	printf("\n*** "); 
	printf("%s Format\n", FreeImage_GetFormatFromFIF(fif));
	printf(message);
	printf(" ***\n");
}

// ----------------------------------------------------------


bool WriteBufferToFile(void * buffer, int size, int count,const char filename[])
{
	FILE * file;

	file = fopen(filename,"wb");
	if (!file) return false;
	fwrite(buffer,size,count,file);
	fclose(file);
	return true;
}

#define CONFIG_UNCOMPRESSED (0)
#define CONFIG_COMPRESSED	(1)
#define CONFIG_16BIT		(0)
#define CONFIG_8BIT			(1 << 1)

//=======================================================
// main
//=======================================================
int 
main(int argc, char *argv[]) {

	char sourcefile_name[MAX_PATH];
	char base_name[MAX_PATH];
	char imagefile_name[MAX_PATH];
	char alphafile_name[MAX_PATH];
	char image_path[MAX_PATH];
	unsigned short palette[256];
	char palettefile_name[MAX_PATH];
	char widthfile_name[MAX_PATH];
	char heightfile_name[MAX_PATH];



	parminit();
	processcmdline(argc, argv);

	if (Parm.optHelp) {
		showsyntax();
		return 0;
	}


	const char *input_dir = ".\\";
	FIBITMAP *dib = NULL;
	int id = 1;

	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_Initialise();
#endif // FREEIMAGE_LIB

	// initialize your own FreeImage error handler
	FreeImage_SetOutputMessage(FreeImageErrorHandler);

	// batch convert all supported bitmaps
	_finddata_t finddata;
	intptr_t handle;


	// scan all files
	strcpy(image_path, input_dir);
	strcat(image_path, Parm.Filefilter);


	if ((handle = _findfirst(image_path, &finddata)) != -1) {
		do {
			

			strcpy(sourcefile_name, input_dir);
			strcat(sourcefile_name, finddata.name);

			strcpy(base_name, finddata.name);
			if (strcspn(base_name,".") != strlen(base_name)) base_name[strcspn(base_name,".")] = '\0';

			strcpy(imagefile_name, base_name);
			strcat(imagefile_name, ".");
			if (Parm.optRLE) strcat(imagefile_name, "rle.");
			strcat(imagefile_name, Parm.ExtensionImage);

			for (int i = 0; i < 256; i++) palette[i] = 0;

			if (Parm.OutputWidth == OutputWidth8Bit)
			{
				if (*Parm.Palettepath)
				{
					FILE * oldpalettefile;
					oldpalettefile = fopen(Parm.Palettepath,"rb");
					if (oldpalettefile)	{
						fread(&palette,2,256,oldpalettefile);
						fclose(oldpalettefile);
					}
					strcpy(palettefile_name,Parm.Palettepath);
				} else {
					strcpy(palettefile_name, base_name);
					strcat(palettefile_name, ".pal.bin");
				}
			}

			if (Parm.optWidthmap && Parm.optTile)
			{
				strcpy(widthfile_name, base_name);
				strcat(widthfile_name, ".width.bin");
				strcpy(heightfile_name, base_name);
				strcat(heightfile_name, ".height.bin");
			}


			if (Parm.optAlphaExternal) {
				strcpy(alphafile_name, "alpha");
				strcat(alphafile_name, base_name);
				if (Parm.optRLE) strcat(alphafile_name, ".rle");
				strcat(alphafile_name, ".");
				strcat(alphafile_name, Parm.ExtensionAlpha);
			}

			// open and load the file using the default load option
			dib = GenericLoader(sourcefile_name, 0);

			if (dib != NULL) {

				/********************************************************************************/
				/* Init handling of single image                                                */
				/********************************************************************************/
				FILE *imagefile = 0;
				FILE *alphafile = 0;
				FILE *palettefile = 0;
				FILE *widthfile = 0;
				FILE *heightfile = 0;

				int bytespp = FreeImage_GetLine(dib) / FreeImage_GetWidth(dib);

				unsigned int x = FreeImage_GetWidth(dib);
				unsigned int y = FreeImage_GetHeight(dib);
				unsigned int pixel_count = x*y;
				unsigned int config = 0;

				unsigned short int * image_buffer16;
				unsigned char * image_buffer8;
				unsigned short * image_buffer4;
				unsigned char * image_buffer1;
				unsigned char * alpha_buffer;
				unsigned char * tile_width_buffer;
				unsigned char * tile_height_buffer;
				unsigned char * image_buffer_4bitpacked;

				unsigned int color_count = 0;

				image_buffer16 = (unsigned short int *)malloc(pixel_count*2);
				memset(image_buffer16,0,pixel_count * 2);

				image_buffer8 = (unsigned char *)malloc(pixel_count);
				memset(image_buffer8,0,pixel_count);

				image_buffer4 = (unsigned short *)malloc(pixel_count);
				memset(image_buffer4,0,pixel_count);

				image_buffer1 = (unsigned char *)malloc(pixel_count/4);
				memset(image_buffer1,0,pixel_count/4);

				alpha_buffer = (unsigned char *)malloc(pixel_count);
				memset(alpha_buffer,0,pixel_count);

				tile_width_buffer = (unsigned char *)malloc( (x/Parm.TileSize) * (y/Parm.TileSize));
				memset(tile_width_buffer, 0, (x/Parm.TileSize) * (y/Parm.TileSize));

				tile_height_buffer = (unsigned char *)malloc( (x/Parm.TileSize) * (y/Parm.TileSize));
				memset(tile_height_buffer, 0, (x/Parm.TileSize) * (y/Parm.TileSize));

				image_buffer_4bitpacked = (unsigned char *)malloc(pixel_count*2);
				memset(image_buffer_4bitpacked,0,pixel_count * 2);
				
					

				unsigned int pos = 0;

				/********************************************************************************/
				/* Provide image information for verbose mode                                   */
				/********************************************************************************/
				if (Parm.optDebug)
				{
					printf ("File %s Width %u Height %d\n",sourcefile_name,x,y);
				}

				/********************************************************************************/
				/* Walk through pixels and fill buffers                                         */
				/********************************************************************************/
				for(unsigned y_c = FreeImage_GetHeight(dib); y_c > 0 ; y_c--) // reverse order of lines
				{ 

					BYTE *bits = FreeImage_GetScanLine(dib, y_c-1);
					unsigned short int pixel;
					for(unsigned x_c = 0; x_c < FreeImage_GetWidth(dib); x_c++) {
						 printf("bpp %u  X %u Y %u  alpha %u  R %u G %u B %u\n",bytespp,x,y,bits[FI_RGBA_ALPHA],bits[FI_RGBA_RED],bits[FI_RGBA_GREEN],bits[FI_RGBA_BLUE]);

						
						image_buffer4[pos] = RGB444(bits[FI_RGBA_RED],bits[FI_RGBA_GREEN],bits[FI_RGBA_BLUE]);
						

						if (Parm.optBGR565) {
							pixel = RGB555(bits[FI_RGBA_RED],bits[FI_RGBA_GREEN],bits[FI_RGBA_BLUE]);
							pixel = RGBFROMBE555(pixel);
						} else if (Parm.optRGB565) {
							pixel = RGB565(bits[FI_RGBA_RED],bits[FI_RGBA_GREEN],bits[FI_RGBA_BLUE]);
						} else {
							pixel = RGB555(bits[FI_RGBA_RED],bits[FI_RGBA_GREEN],bits[FI_RGBA_BLUE]);
						}

						// 8-bit w/ palette
						if ( (bits[FI_RGBA_ALPHA] == 0) || (Parm.optAlphaTransparent && bits[FI_RGBA_ALPHA] != 255) )
							image_buffer8[pos] = 0; // fully transparent pixels always position 0
						else {

							if (pixel == 0) image_buffer8[pos] = 1; // color 0,0,0 always at position 1
							else
							{

								image_buffer8[pos] = 0;

								int i = 2; // first two palette entries are fixed

								while ( (i < 256) && (palette[i] != 0) && (image_buffer8[pos] == 0) )
								{
									if (palette[i] == pixel) image_buffer8[pos] = i;
									i++;
								}

								if (image_buffer8[pos] == 0)
								{
									color_count ++;
									if (i < 256) {
										palette[i] = pixel;
										image_buffer8[pos] = i;
									} else {
										image_buffer8[pos] = 255;
										
									}
								}
							}

						}

						// 1-bit bw 
						if (bits[FI_RGBA_ALPHA] == 0)
							image_buffer1[pos/8] = (image_buffer1[pos/8] & ~(1 << (pos % 8) ) );
						else 
							image_buffer1[pos/8] = (image_buffer1[pos/8] |  (1 << (pos % 8) ) );

						
						if (Parm.optBGR565) {

							// BGR565: 16-bit w/o transparency bit
							image_buffer16[pos] = pixel;

						} else if (Parm.optRGB565) {

							// RGB565: 16-bit w/o transparency bit
							image_buffer16[pos] = pixel;


						} else if ( (bits[FI_RGBA_ALPHA] == 0)  || (Parm.optAlphaTransparent && bits[FI_RGBA_ALPHA] != 255) ) {

							// 16-bit w/ transparency bit (unset)
							image_buffer16[pos] = pixel & ~(1 << 15);

						} else {

							// 16-bit w/ transparency bit (set)
							image_buffer16[pos] = pixel | (1 << 15);

						}

						// alpha data
						alpha_buffer[pos] = bits[FI_RGBA_ALPHA];

						// jump to next pixel
						pos++;
						bits += bytespp;
					}
				}

				if (Parm.OutputWidth == OutputWidth8Bit) 
				{
					if (color_count > 255)
					 if (!Parm.optQuiet) printf("Warning: Palette overflow, %u colors detected in %u pixels.\n",color_count,pixel_count);
				}

				/********************************************************************************/
				/* Rearrange 8-bit, 1-bit and alpha buffers into tiles if requested             */
				/********************************************************************************/
				if (Parm.optTile)
				{
					if (Parm.OutputWidth == OutputWidth8Bit) 
					{
						unsigned char * tilebuffer;
						tilebuffer = new unsigned char[pixel_count];
						unsigned char * tilepointer = tilebuffer;

						int tilecount_x = x / Parm.TileSize;
						int tilecount_y = y / Parm.TileSize;


						for (int tiley = 0; tiley < tilecount_y; tiley++)
						{
							for (int tilex = 0; tilex < tilecount_x; tilex++)
							{

								int tilestarty = tiley * Parm.TileSize;
								int tilestartx = tilex * Parm.TileSize;

								for (int i = 0; i < Parm.TileSize; i++)
								{

									for (int j = 0; j < Parm.TileSize; j++)
									{
										tilepointer[0] = image_buffer8[ ((tilestarty + i) *x) + (tilestartx + j)  ];
										tilepointer++;
									}
								}
							}
						}

						image_buffer8 = tilebuffer;
					} // if (Parm.OutputWidth == OutputWidth8Bit) 

					if (Parm.OutputWidth == OutputWidth1Bit) 
					{
						unsigned char * tilebuffer;
						tilebuffer = new unsigned char[pixel_count/8];
						unsigned char * tilepointer = tilebuffer;
						memset(tilebuffer,0,pixel_count/8);
						unsigned char last_pixel_x = 0;
						unsigned char last_pixel_y = 0;

						int tilecount_x = x / Parm.TileSize;
						int tilecount_y = y / Parm.TileSize;

						for (int tiley = 0; tiley < tilecount_y; tiley++)
						{
							for (int tilex = 0; tilex < tilecount_x; tilex++)
							{
								last_pixel_x = 0;
								last_pixel_y = 0;

								for (int i = 0; i < Parm.TileSize; i++)
								{
									for (int j = 0; j < Parm.TileSize / 8; j++)
									{ 
										tilepointer[0] = image_buffer1[ 
											(tiley * (x / 8) * Parm.TileSize) +    // Start of Tilerow
											(tilex * (Parm.TileSize / 8))  +	   // Start of Tilecolumn
											(i * (x / 8)) +						   // Line within Tile
											j									   // Pixel within Line
										];
										
										unsigned char pixcount = tilepointer[0];

										if (pixcount & 0x80) last_pixel_x = max(last_pixel_x,(8*j) + 8);
										else if (pixcount & 0x40) last_pixel_x = max(last_pixel_x,(8*j) + 7);
										else if (pixcount & 0x20) last_pixel_x = max(last_pixel_x,(8*j) + 6);
										else if (pixcount & 0x10) last_pixel_x = max(last_pixel_x,(8*j) + 5);
										else if (pixcount & 0x08) last_pixel_x = max(last_pixel_x,(8*j) + 4);
										else if (pixcount & 0x04) last_pixel_x = max(last_pixel_x,(8*j) + 3);
										else if (pixcount & 0x02) last_pixel_x = max(last_pixel_x,(8*j) + 2);
										else if (pixcount & 0x01) last_pixel_x = max(last_pixel_x,(8*j) + 1);

										if (pixcount) last_pixel_y = i;

										tilepointer++;
									}
								}

								if (Parm.optDebug)
								{
									printf("Tile %d - w %d h %d\n",(tiley*tilecount_x)+tilex,last_pixel_x,last_pixel_y);
								}

								tile_width_buffer[(tiley*tilecount_x)+tilex] = last_pixel_x;
								tile_height_buffer[(tiley*tilecount_x)+tilex] = last_pixel_y;
							}
						}
						

						image_buffer1 = tilebuffer;
					} // if (Parm.OutputWidth == OutputWidth1Bit) 

					if (Parm.optAlphaExternal) 
					{
						unsigned char * tilebuffer;
						tilebuffer = new unsigned char[pixel_count];
						unsigned char * tilepointer = tilebuffer;

						int tilecount_x = x / Parm.TileSize;
						int tilecount_y = y / Parm.TileSize;

						for (int tiley = 0; tiley < tilecount_y; tiley++)		// for each tilerow
						{
							for (int tilex = 0; tilex < tilecount_x; tilex++)	// for each tile column
							{
								for (int i = 0; i < Parm.TileSize; i++)			// for each line within tile
								{
									for (int j = 0; j < Parm.TileSize; j++)		// for pixel within line
									{ 
										tilepointer[0] = alpha_buffer[ 
											(tiley * (x) * Parm.TileSize) +    // Start of Tilerow
											(tilex * (Parm.TileSize))  +	   // Start of Tilecolumn
											(i * (x)) +						   // Line within Tile
											j								   // Pixel within Line
										];
										tilepointer++;
									}
								}
							}
						}
						alpha_buffer = tilebuffer;
					} // if (Parm.OutputWidth == OutputWidth1Bit) 


				}

				/********************************************************************************/
				/* Pack data for RGB444 file format                                             */
				/********************************************************************************/
				if (Parm.OutputWidth == OutputWidth3x4Bit)
				{
					int i,j;
					
					j = 0;

					for (i=0; i<pixel_count/2; i++)
					{
						image_buffer_4bitpacked[j]   = ((image_buffer4[i*2]   & 0xFF0) >> 4);
						image_buffer_4bitpacked[j+1] = ((image_buffer4[i*2]   & 0x00F) << 4) | ((image_buffer4[i*2+1] & 0xF00) >> 8);
						image_buffer_4bitpacked[j+2] = ((image_buffer4[i*2+1] & 0x0FF));

						j += 3;
					}
				}

				/********************************************************************************/
				/* Open files according to option settings                                      */
				/********************************************************************************/
				imagefile = fopen(imagefile_name, "wb");
				if (!imagefile) {
					if (!Parm.optQuiet) printf("Error opening image file %s\n for writing.",imagefile_name);
					return 1;
				}

				if (Parm.optAlphaExternal)
				{
					alphafile = fopen(alphafile_name, "wb");
					if (!alphafile) {
						if (!Parm.optQuiet) printf("Error opening alpha file %s\n for writing.",alphafile_name);
						return 2;
					}
				}



				/********************************************************************************/
				/* Save image and alpha data                                                    */
				/********************************************************************************/
				if (Parm.optRLE)
				{
					config = CONFIG_COMPRESSED;

					if (Parm.OutputWidth == OutputWidth8Bit) config |= CONFIG_8BIT;
					else config |= CONFIG_16BIT;

					unsigned short int * compress_buffer;
					compress_buffer = (unsigned short int *)malloc(pixel_count*2*257/256+1);
					

					unsigned int outsize;

					if (Parm.OutputWidth == OutputWidth8Bit) outsize = RLE_Compress8(image_buffer8,(unsigned char *)compress_buffer,pixel_count);
					else if (Parm.OutputWidth == OutputWidth1Bit) outsize = RLE_Compress8(image_buffer1,(unsigned char *)compress_buffer,pixel_count/8);
					else outsize = RLE_Compress16(image_buffer16,compress_buffer,pixel_count);

					if (!Parm.optQuiet) printf("%s (Size %u) -> %s (Size %u)\n",sourcefile_name,pixel_count*2,imagefile_name,outsize);

					unsigned int uncompressed_size = pixel_count*2;


					if (!Parm.optNoHeader)
					{
						fwrite(&outsize,4,1,imagefile);
						fwrite(&x,2,1,imagefile);
						fwrite(&y,2,1,imagefile);
						fwrite(&config,2,1,imagefile);
					}

					//debug only
					if (Parm.optDebug)
					{
						// For debugging, decompress data after compression and write the decompressed image
						unsigned short int * decompress_buffer;
						decompress_buffer = (unsigned short int *)malloc(pixel_count*2*257/256+1);
						printf("WARNING: Output decompressed for debugging\n"); 
						printf("pixel_count %u outsize %u\n",pixel_count,outsize);
						if (Parm.OutputWidth == OutputWidth8Bit) RLE_Uncompress8((unsigned char *)compress_buffer,(unsigned char *)decompress_buffer,outsize);
						else RLE_Uncompress16(compress_buffer,decompress_buffer,outsize);
						if (Parm.OutputWidth == OutputWidth8Bit) fwrite(decompress_buffer,1,pixel_count,imagefile);
						else fwrite(decompress_buffer,2,pixel_count,imagefile);
						free(decompress_buffer);

					} else {
						// regular operation, write compressed data to disc
						if (Parm.OutputWidth == OutputWidth8Bit) fwrite(compress_buffer,1,outsize,imagefile);
						else if (Parm.OutputWidth == OutputWidth1Bit) fwrite(compress_buffer,1,outsize,imagefile);
						else fwrite(compress_buffer,2,outsize,imagefile);

					}

					if (Parm.optAlphaExternal) {
						int outsize = RLE_Compress8(alpha_buffer,(unsigned char *)compress_buffer,pixel_count);
						fwrite(&outsize,4,1,alphafile);
						fwrite(&x,2,1,alphafile);
						fwrite(&y,2,1,alphafile);
						fwrite(&config,2,1,alphafile);
						fwrite(compress_buffer,1,outsize,alphafile);
					}

					free(compress_buffer);

				} else {

					config = CONFIG_UNCOMPRESSED;

					if (Parm.OutputWidth == OutputWidth8Bit) config |= CONFIG_8BIT;
					else config |= CONFIG_16BIT;

					if (!Parm.optQuiet) printf("%s -> %s (Size %u)\n",sourcefile_name,imagefile_name,pixel_count*2);

					if (!Parm.optNoHeader)
					{
						fwrite(&pixel_count,4,1,imagefile);
						fwrite(&x,2,1,imagefile);
						fwrite(&y,2,1,imagefile);
						fwrite(&config,2,1,imagefile);
					}

					if (Parm.OutputWidth == OutputWidth8Bit) fwrite(image_buffer8,1,pixel_count,imagefile);
					else if (Parm.OutputWidth == OutputWidth1Bit) fwrite(image_buffer1,1,pixel_count/8,imagefile);
					else if (Parm.OutputWidth == OutputWidth3x4Bit)  fwrite(image_buffer_4bitpacked,1,pixel_count*3/2,imagefile);
					else fwrite(image_buffer16,2,pixel_count,imagefile);

					if (Parm.optAlphaExternal) {
						fwrite(&pixel_count,4,1,alphafile);
						fwrite(&x,2,1,alphafile);
						fwrite(&y,2,1,alphafile);
						fwrite(&config,2,1,alphafile);
						fwrite(alpha_buffer,1,pixel_count,alphafile);
					}
				}

				/********************************************************************************/
				/* Save palette data                                                            */
				/********************************************************************************/
				if (Parm.OutputWidth == OutputWidth8Bit)
				{
					if (Parm.optDebug) for (int i = 2; (i<256) && (palette[i] != 0);i++) printf("pal %x - %x\n",i,palette[i]);

					palettefile = fopen(palettefile_name,"wb");
					if (!palettefile) {
						if (!Parm.optQuiet) printf("Error opening palette file %s for writing.\n",palettefile_name);
						return 3;
					}
					fwrite(&palette,2,256,palettefile);
					fclose(palettefile);
				}


				/********************************************************************************/
				/* Save tile dimension data                                                     */
				/********************************************************************************/
				if (Parm.optWidthmap && Parm.optTile)
				{
					widthfile = fopen(widthfile_name, "wb");
					if (!widthfile) {
						if (!Parm.optQuiet) printf("Error opening width file %s for writing.",widthfile_name);
						return 2;
					}
					fwrite(tile_width_buffer,1,(x/Parm.TileSize) * (y/Parm.TileSize), widthfile);
					fclose(widthfile);

					heightfile = fopen(heightfile_name, "wb");
					if (!heightfile) {
						if (!Parm.optQuiet) printf("Error opening height file %s for writing.",heightfile_name);
						return 2;
					}
					fwrite(tile_height_buffer,1,(x/Parm.TileSize) * (y/Parm.TileSize), heightfile);
					fclose(heightfile);
				}



				/********************************************************************************/
				/* Free resources                                                               */
				/********************************************************************************/
				if (alphafile) fclose(alphafile);
				if (imagefile) fclose(imagefile);

				alphafile = 0;
				imagefile = 0;
/*
				free(tile_height_buffer);
				free(tile_width_buffer);
				free(alpha_buffer);
				free(image_buffer16);
				free(image_buffer8);
//				free(image_buffer1);
				free(image_buffer4);
				free(image_buffer_4bitpacked);
*/
				FreeImage_Unload(dib);

			} else {
				if (!Parm.optQuiet) printf("Error loading %s\n",sourcefile_name);
			}





		} while (_findnext(handle, &finddata) == 0);

		_findclose(handle);
	}

	// call this ONLY when linking with FreeImage as a static library
#ifdef FREEIMAGE_LIB
	FreeImage_DeInitialise();
#endif // FREEIMAGE_LIB

	return 0;
}

