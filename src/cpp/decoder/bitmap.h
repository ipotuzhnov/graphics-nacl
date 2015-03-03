#ifndef __BITMAP_TYPE_H__
#define __BITMAP_TYPE_H__

#include <memory>

#include <iostream>
#include <fstream>
#include <tuple>

#include <png.h>

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

namespace decoder {

	class Bitmap {
	private:
		int bitsPixel_;
		int colors_;
		int width_;
		int height_;
		int rowSize_;
		char* imageBuffer_;
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

		~Bitmap() {
			delete [] imageBuffer_;
		}

		/* Creates a base64 encoded PNG of bitmap in bounds of given frame.
		 *  @param {DjVuFrame} frame The frame representing bound of required bitmap.
		 * Returns a {std::tuple} that contains {std::string} error and
		 *   {std::string} base64 representation of created PNG.
		 */
		std::tuple<std::string, std::string> getAsBase64Encoded(std::shared_ptr<DjVuFrame> frame) {
			std::string error;
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
				error = "Could not create base64 encoded bitmap. Could not create png_write_struct.";
				return  std::make_tuple(error, base64encoded);
			}

			info_ptr = png_create_info_struct (png_ptr);
			if (info_ptr == NULL) {
				png_destroy_write_struct (&png_ptr, &info_ptr);
				error = "Could not create base64 encoded bitmap. Could not create png_info_struct.";
				return  std::make_tuple(error, base64encoded);
			}

			/* Set up error handling. */

			if (setjmp (png_jmpbuf (png_ptr))) {
				png_destroy_write_struct (&png_ptr, &info_ptr);
				error = "Could not create base64 encoded bitmap. Could not set up libpng error handling.";
				return  std::make_tuple(error, base64encoded);
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

			int offsetX = frame->left;
			int offsetY = frame->top;
			row_pointers = (png_byte **) png_malloc (png_ptr, height * sizeof (png_byte *));
			for (int y = 0; y < height; ++y) {
				png_byte *row = (png_byte *) png_malloc (png_ptr, sizeof (uint8_t) * width * bitsPixel_);
				row_pointers[y] = row;
				for (int x = 0; x < width; ++x) {
					int dx = x + offsetX;
					int dy = y + offsetY;
					if (color_type == PNG_COLOR_TYPE_GRAY) {
						*row++ = imageBuffer_[dy * rowSize_ + dx];
					}
					if (color_type == PNG_COLOR_TYPE_RGB) {
						*row++ = imageBuffer_[dy * rowSize_ + dx * bitsPixel_ + 2];
						*row++ = imageBuffer_[dy * rowSize_ + dx * bitsPixel_ + 1];
						*row++ = imageBuffer_[dy * rowSize_ + dx * bitsPixel_ + 0];
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
			for (int y = 0; y < height; y++) {
				png_free (png_ptr, row_pointers[y]);
			}
			png_free (png_ptr, row_pointers);

			if(virtual_file.buffer)
				free(virtual_file.buffer);

			png_destroy_write_struct (&png_ptr, &info_ptr);

			return std::make_tuple(error, base64encoded);
		}

		std::tuple<std::string, pp::VarDictionary> getAsBase64Dictionary(std::shared_ptr<DjVuFrame> frame) {
			int width = frame->right - frame->left;
			int height = frame->bottom - frame->top;
			pp::VarDictionary bmp;
			bmp.Set("bitsPixel", bitsPixel_);
			bmp.Set("colors", colors_);
			bmp.Set("width", width);
			bmp.Set("height", height);
			bmp.Set("rowSize", rowSize_);
			std::string error;
			std::string png;
			std::tie (error, png) = getAsBase64Encoded(frame);
			if (png.empty()) {
				error = "Generated png is empty.";
				return std::make_tuple(error, bmp);
			}
			bmp.Set("imageData", png);
			return std::make_tuple(error, bmp);
		}

	};

}

#endif // __BITMAP_TYPE_H__
