#ifndef BITMAP_TYPE_H_
#define BITMAP_TYPE_H_

#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/instance.h"

class Bitmap {
private:
	int bitsPixel_;
	int colors_;
	int width_;
	int height_;
	int rowSize_;
	char* imageBuffer_;

	uint32_t MakeColor(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
		PP_ImageDataFormat format = pp::ImageData::GetNativeImageDataFormat();
		if (format == PP_IMAGEDATAFORMAT_BGRA_PREMUL) {
			return (a << 24) | (r << 16) | (g << 8) | b;
		} else {
			return (a << 24) | (b << 16) | (g << 8) | r;
		}
	}
public: 
	Bitmap(int bitsPixel, int colors, int width, int height, int rowSize, char* imageBuffer) {
		bitsPixel_ = bitsPixel;
		colors_ = colors;
		width_ = width;
		height_ = height;
		rowSize_ = rowSize;
		imageBuffer_ = new char[rowSize * height];
		//memset(imageBuffer, 0, rowSize * height);
		memcpy(imageBuffer_, imageBuffer, rowSize * height);
	}

	Bitmap(pp::VarDictionary bmp) {
		bitsPixel_ = bmp.Get("bitsPixel").AsInt();
		colors_ = bmp.Get("colors").AsInt();
		width_ = bmp.Get("width").AsInt();
		height_ = bmp.Get("height").AsInt();
		rowSize_ = bmp.Get("rowSize").AsInt();
		pp::VarArrayBuffer pageImageData(bmp.Get("imageData"));
		char *buffer = static_cast<char*>(pageImageData.Map());
		int size = pageImageData.ByteLength();
		imageBuffer_ = new char[size];
		memcpy(imageBuffer_, buffer, size);		
		pageImageData.Unmap();
	}

	~Bitmap() {
		delete [] imageBuffer_;
	}

	char *getImageBuffer() {
		return imageBuffer_;
	}

	int getImageBufferSize() {
		return rowSize_ * height_;
	}

	pp::VarDictionary getAsDictionary() {
		pp::VarDictionary bmp;
		bmp.Set("bitsPixel", bitsPixel_);
		bmp.Set("colors", colors_);
		bmp.Set("width", width_);
		bmp.Set("height", height_);
		bmp.Set("rowSize", rowSize_);
		int size = rowSize_ * height_;
		pp::VarArrayBuffer pageImageData(size);
		char *buffer = static_cast<char*>(pageImageData.Map());
		memcpy(buffer, imageBuffer_, size);
		bmp.Set("imageData", pageImageData);
		pageImageData.Unmap();
		return bmp;
	}

	pp::ImageData getAsImageData(pp::Instance *instance) {
		PP_ImageDataFormat format = pp::ImageData::GetNativeImageDataFormat();
		const bool kInitToZero = true;
		pp::Size size(width_, height_);
		pp::ImageData image(instance, format, size, kInitToZero);

		for (int y = 0;  y < height_; y++) {
			for (int x = 0; x < width_; x++) {
				if (bitsPixel_ == 1) {
					*image.GetAddr32(pp::Point(x, y)) = MakeColor(255, imageBuffer_[y * rowSize_ + x] , imageBuffer_[y * rowSize_ + x], imageBuffer_[y * rowSize_ + x]);
				} else {
					*image.GetAddr32(pp::Point(x, y)) = MakeColor(255, imageBuffer_[y * rowSize_ + x * 3 + 2], imageBuffer_[y * rowSize_ + x * 3 + 1], imageBuffer_[y * rowSize_ + x * 3]);
				}
			}
		}
		return image;
	}
};

#endif // BITMAP_TYPE_H_
