#ifndef __BITMAP_TYPE_H__
#define __BITMAP_TYPE_H__

#include <memory>

#include <iostream>
#include <fstream>

#include <png.h>
//#include <stdio.h>
//#include <stdlib.h>
//#include <stdint.h>

//#include "bitmap.h"
#include "../helpers/virtual_file.h"
#include "../base64/base64.h"

#include "ppapi/cpp/var.h"
#include "ppapi/cpp/var_dictionary.h"
#include "ppapi/cpp/var_array_buffer.h"
#include "ppapi/cpp/image_data.h"
#include "ppapi/cpp/instance.h"

namespace {
struct DjVuSize {
	int width;
	int height;
};

struct DjVuFrame {
	int left;
	int top;
	int right;
	int bottom;
};
}

namespace renderer {

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

		std::string getAsBase64Encoded(std::shared_ptr<DjVuFrame> frame) {
			std::string base64encoded;

			/* Virtual file */
			struct mem_encode virtual_file;

			/* initialise - put this before png_write_png() call */
			virtual_file.buffer = NULL;
			virtual_file.size = 0;

			png_structp png_ptr = NULL;
			png_infop info_ptr = NULL;
			png_byte ** row_pointers = NULL;
			int color_type = bitsPixel_ == 1 ? PNG_COLOR_TYPE_GRAY : PNG_COLOR_TYPE_RGB;
			int depth = 8;

			png_ptr = png_create_write_struct (PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
			if (png_ptr == NULL) {
				// TODO (ilia) send error to JS
				return base64encoded;
			}

			info_ptr = png_create_info_struct (png_ptr);
			if (info_ptr == NULL) {
				// TODO (ilia) send error to JS
				png_destroy_write_struct (&png_ptr, &info_ptr);
				return base64encoded;
			}

			/* Set up error handling. */

			if (setjmp (png_jmpbuf (png_ptr))) {
				// TODO (ilia) send error to JS
				png_destroy_write_struct (&png_ptr, &info_ptr);
				return base64encoded;
			}

			/* Calculate image attributes. */
			int width = frame->right - frame->left;
			int height = frame->bottom - frame->top;

			/* Set image attributes. */

			png_set_IHDR (png_ptr,
				info_ptr,
				width,
				height,
				depth,
				color_type,
				PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_DEFAULT,
				PNG_FILTER_TYPE_DEFAULT);

			/* Initialize rows of PNG. */

			row_pointers = (png_byte **) png_malloc (png_ptr, height * sizeof (png_byte *));
			for (int y = frame->top; y < frame->bottom; ++y) {
				png_byte *row = (png_byte *) png_malloc (png_ptr, sizeof (uint8_t) * width * bitsPixel_);
				row_pointers[y] = row;
				for (int x = frame->left; x < frame->right; ++x) {
					if (color_type == PNG_COLOR_TYPE_GRAY) {
						*row++ = imageBuffer_[y * rowSize_ + x];
					}
					if (color_type == PNG_COLOR_TYPE_RGB) {
						*row++ = imageBuffer_[y * rowSize_ + x * bitsPixel_ + 2];
						*row++ = imageBuffer_[y * rowSize_ + x * bitsPixel_ + 1];
						*row++ = imageBuffer_[y * rowSize_ + x * bitsPixel_ + 0];
					}
				}
			}

			/* Write png to virtual file */
			png_set_write_fn(png_ptr, &virtual_file, png_write_data_callback, png_flush_callback);

			png_set_rows (png_ptr, info_ptr, row_pointers);
			png_write_png (png_ptr, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);

			/* now virtual_file.buffer contains the PNG image of size virtual_file.size bytes */
			/* Encode virtual_file.buffer to base64 */
			base64encoded = base64_encode(reinterpret_cast<const unsigned char*>(virtual_file.buffer), virtual_file.size);

			/* cleanup */
			for (int y = frame->top; y < frame->bottom; y++) {
				png_free (png_ptr, row_pointers[y]);
			}
			png_free (png_ptr, row_pointers);
			
			if(virtual_file.buffer)
				free(virtual_file.buffer);

			png_destroy_write_struct (&png_ptr, &info_ptr);

			return base64encoded;
		}

		pp::VarDictionary getAsBase64Dictionary(std::shared_ptr<DjVuFrame> frame) {
			int width = frame->right - frame->left;
			int height = frame->bottom - frame->top;
			pp::VarDictionary bmp;
			bmp.Set("bitsPixel", bitsPixel_);
			bmp.Set("colors", colors_);
			bmp.Set("width", width);
			bmp.Set("height", height);
			bmp.Set("rowSize", rowSize_);
			bmp.Set("imageData", getAsBase64Encoded(frame));
			return bmp;
		}

	};

}

#endif // __BITMAP_TYPE_H__
