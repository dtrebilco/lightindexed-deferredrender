/***********      .---.         .-"-.      *******************\
* -------- *     /   ._.       / ´ ` \     * ---------------- *
* Author's *     \_  (__\      \_°v°_/     * humus@rogers.com *
*   note   *     //   \\       //   \\     * ICQ #47010716    *
* -------- *    ((     ))     ((     ))    * ---------------- *
*          ****--""---""-------""---""--****                  ********\
* This file is a part of the work done by Humus. You are free to use  *
* the code in any way you like, modified, unmodified or copy'n'pasted *
* into your own work. However, I expect you to respect these points:  *
*  @ If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  @ For use in anything commercial, please request my approval.      *
*  @ Share your work and ideas too as much as you can.                *
\*********************************************************************/

#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "../Platform.h"

#define ALL_MIPMAPS 127

// Image loading flags
#define DONT_LOAD_MIPMAPS 0x1

enum FORMAT {
	FORMAT_NONE     = 0,
	// Plain formats
	FORMAT_R8       = 1,
	FORMAT_RG8      = 2,
	FORMAT_RGB8     = 3,
	FORMAT_RGBA8    = 4,

	FORMAT_R16      = 5,
	FORMAT_RG16     = 6,
	FORMAT_RGB16    = 7,
	FORMAT_RGBA16   = 8,

	FORMAT_R16F     = 9,
	FORMAT_RG16F    = 10,
	FORMAT_RGB16F   = 11,
	FORMAT_RGBA16F  = 12,

	FORMAT_R32F     = 13,
	FORMAT_RG32F    = 14,
	FORMAT_RGB32F   = 15,
	FORMAT_RGBA32F  = 16,

	// Packed formats
	FORMAT_RGBE8    = 17,
	FORMAT_RGB9E5   = 18,
	FORMAT_RG11B10F = 19,
	FORMAT_RGB565   = 20,
	FORMAT_RGBA4    = 21,
	FORMAT_RGB10A2  = 22,

	// Depth formats
	FORMAT_DEPTH16  = 23,
	FORMAT_DEPTH24  = 24,

	// Compressed formats
	FORMAT_DXT1     = 25,
	FORMAT_DXT3     = 26,
	FORMAT_DXT5     = 27,
	FORMAT_ATI1N    = 28,
	FORMAT_ATI2N    = 29,
};

#define FORMAT_I8    FORMAT_R8
#define FORMAT_IA8   FORMAT_RG8
#define FORMAT_I16   FORMAT_R16
#define FORMAT_IA16  FORMAT_RG16
#define FORMAT_I16F  FORMAT_R16F
#define FORMAT_IA16F FORMAT_RG16F
#define FORMAT_I32F  FORMAT_R32F
#define FORMAT_IA32F FORMAT_RG32F

inline bool isPlainFormat(const FORMAT format){
	return (format <= FORMAT_RGBA32F);
}

inline bool isPackedFormat(const FORMAT format){
	return (format >= FORMAT_RGBE8 && format <= FORMAT_RGB10A2);
}

inline bool isDepthFormat(const FORMAT format){
	return (format >= FORMAT_DEPTH16 && format <= FORMAT_DEPTH24);
}

inline bool isCompressedFormat(const FORMAT format){
	return (format >= FORMAT_DXT1) && (format <= FORMAT_ATI2N);
}

inline bool isFloatFormat(const FORMAT format){
//	return (format >= FORMAT_R16F && format <= FORMAT_RGBA32F);
	return (format >= FORMAT_R16F && format <= FORMAT_RG11B10F);
}

inline int getChannelCount(const FORMAT format){
	static const int chCount[] = {
		0,
		1, 2, 3, 4,
		1, 2, 3, 4,
		1, 2, 3, 4,
		1, 2, 3, 4,
		3, 3, 3, 3, 4, 4,
		1, 1,
		3, 4, 4, 1, 2,
	};
	return chCount[format];
}

// Accepts only plain formats
inline int getBytesPerChannel(const FORMAT format){
	if (format <= FORMAT_RGBA8) return 1;
	if (format <= FORMAT_RGBA16F) return 2;
	return 4;
}

// Accepts only plain and packed
inline int getBytesPerPixel(const FORMAT format){
	static const int bytesPP[] = {
		0,
		1, 2, 3, 4,
		2, 4, 6, 8,
		2, 4, 6, 8,
		4, 8, 12, 16,
		4, 4, 4, 2, 2, 4,
	};
	return bytesPP[format];
}

// Accepts only compressed formats
inline int getBytesPerBlock(const FORMAT format){
	return (format == FORMAT_DXT1 || format == FORMAT_ATI1N)? 8 : 16;
}

const char *getFormatString(const FORMAT format);
FORMAT getFormatFromString(char *string);

class Image {
public:
	Image();
	Image(const Image &img);
	~Image();

	unsigned char *create(const FORMAT fmt, const int w, const int h, const int d, const int mipMapCount);
	void free();
	void clear();

	unsigned char *getPixels() const { return pixels; }
	unsigned char *getPixels(const int mipMapLevel) const;
	int getMipMapCount() const { return nMipMaps; }
	int getMipMapCountFromDimesions() const;
	int getMipMappedSize(const int firstMipMapLevel = 0, int nMipMapLevels = ALL_MIPMAPS, FORMAT srcFormat = FORMAT_NONE) const;
	int getSliceSize(const int mipMapLevel = 0, FORMAT srcFormat = FORMAT_NONE) const;
	int getPixelCount(const int firstMipMapLevel = 0, int nMipMapLevels = ALL_MIPMAPS) const;

	int getWidth () const { return width;  }
	int getHeight() const { return height; }
	int getDepth () const { return depth;  }
	int getWidth (const int mipMapLevel) const;
	int getHeight(const int mipMapLevel) const;
	int getDepth (const int mipMapLevel) const;

	bool is1D()   const { return (depth == 1 && height == 1); }
	bool is2D()   const { return (depth == 1 && height >  1); }
	bool is3D()   const { return (depth >  1); }
	bool isCube() const { return (depth == 0); }

	FORMAT getFormat() const { return format; }
	void setFormat(const FORMAT form){ format = form; }

	int getExtraDataBytes() const { return nExtraData; }
	ubyte *getExtraData() const { return extraData; }
	void setExtraData(void *data, const int nBytes){
		nExtraData = nBytes;
		extraData = (unsigned char *) data;
	}

	bool loadHTEX(const char *fileName);
	bool loadDDS(const char *fileName, uint flags = 0);
#ifndef NO_HDR
	bool loadHDR(const char *fileName);
#endif // NO_HDR
#ifndef NO_JPEG
	bool loadJPEG(const char *fileName);
#endif // NO_JPEG
#ifndef NO_PNG
	bool loadPNG(const char *fileName);
#endif // NO_PNG
#ifndef NO_TGA
	bool loadTGA(const char *fileName);
#endif // NO_TGA
#ifndef NO_BMP
	bool loadBMP(const char *fileName);
#endif // NO_BMP
#ifndef NO_PCX
	bool loadPCX(const char *fileName);
#endif // NO_PCX

	bool saveHTEX(const char *fileName);
	bool saveDDS(const char *fileName);
#ifndef NO_HDR
	bool saveHDR(const char *fileName);
#endif // NO_HDR
#ifndef NO_JPEG
	bool saveJPEG(const char *fileName, const int quality);
#endif // NO_JPEG
#ifndef NO_PNG
	bool savePNG(const char *fileName);
#endif // NO_PNG
#ifndef NO_TGA
	bool saveTGA(const char *fileName);
#endif // NO_TGA
#ifndef NO_BMP
	bool saveBMP(const char *fileName);
#endif // NO_BMP
#ifndef NO_PCX
	bool savePCX(const char *fileName);
#endif // NO_PCX

	bool loadImage(const char *fileName, uint flags = 0);
	bool loadSlicedImage(const char **fileNames, const int nImages, uint flags = 0);
	bool saveImage(const char *fileName);

	void loadFromMemory(void *mem, const FORMAT frmt, const int w, const int h, const int d, const int mipMapCount, bool ownsMemory);

	bool createMipMaps(const int mipMaps = ALL_MIPMAPS);
	bool removeMipMaps(const int firstMipMap, const int mipMapsToSave = ALL_MIPMAPS);

	bool uncompressImage();
	bool unpackImage();

	bool convert(const FORMAT newFormat);
	bool swap(const int ch0, const int ch1);

	bool toRGBD16();
	bool toRGBE16(float &scale, float &bias);
	bool toE16(float *scale, float *bias, const bool useAllSameRange = false, const float minValue = FLT_MIN, const float maxValue = FLT_MAX);
	bool toFixedPointHDR(float *maxValue, const int finalRgbBits, const int finalRangeBits);
//	bool resize(const int newWidth, const int newHeight, const ResizeMethod method);
	bool toNormalMap(FORMAT destFormat, float sZ = 1.0f, float mipMapScaleZ = 2.0f);
	bool toGrayScale();
	bool getRange(float &min, float &max);
	bool scaleBias(const float scale, const float bias);
	bool normalize();

	bool removeChannels(bool keepCh0, bool keepCh1 = true, bool keepCh2 = true, bool keepCh3 = true);

protected:
	unsigned char *pixels;
	int width, height, depth;
	int nMipMaps;
	FORMAT format;

	int nExtraData;
	unsigned char *extraData;
};

#endif // _IMAGE_H_
