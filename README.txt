
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
