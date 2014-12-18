#ifndef BMP_DELEGATE_H_
#define BMP_DELEGATE_H_

#include "ddjvu/IBmp.h"

#include "ppapi/cpp/instance.h"
#include "ppapi/cpp/image_data.h"

class BmpDelegate: public ddjvu::IBmp<pp::ImageData> {
private:
	std::shared_ptr<pp::ImageData> bmp_;
	pp::Instance* instance_;  // Weak pointer.

	uint32_t MakeColor(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
		PP_ImageDataFormat format = pp::ImageData::GetNativeImageDataFormat();
		if (format == PP_IMAGEDATAFORMAT_BGRA_PREMUL) {
			return (a << 24) | (r << 16) | (g << 8) | b;
		} else {
			return (a << 24) | (b << 16) | (g << 8) | r;
		}
	}
public:
	BmpDelegate() 
		: instance_(nullptr){ 
		bmp_ = std::shared_ptr<pp::ImageData> ();
	}

	BmpDelegate(pp::Instance* instance, int bitsPixel, int colors, int width, int height, int rowSize, char* imageBuffer)
		: instance_(instance) { 
		// here you should create bitmap_ = std::shared<ptr> (new T)
		PP_ImageDataFormat format = pp::ImageData::GetNativeImageDataFormat();
		const bool kInitToZero = true;
		pp::Size size(width, height);
		pp::ImageData image(instance_, format, size, kInitToZero);

		for (int y = 0;  y < height; y++) {
			for (int x = 0; x < width; x++) {
				if (bitsPixel == 1) {
					*image.GetAddr32(pp::Point(x, y)) = MakeColor(imageBuffer[y * rowSize + x], 255 , 255, 255);
				} else {
					*image.GetAddr32(pp::Point(x, y)) = MakeColor(255, imageBuffer[y * rowSize + x * 3 + 2], imageBuffer[y * rowSize + x * 3 + 1], imageBuffer[y * rowSize + x * 3]);
				}
			}
		}

		bmp_ = std::make_shared<pp::ImageData> (image);

		/*
		BITMAPINFO *bmi = (BITMAPINFO *)calloc(1, sizeof(BITMAPINFOHEADER) + colors * sizeof(RGBQUAD));
		for (int i = 0; i < colors; i++)
			bmi->bmiColors[i].rgbRed = bmi->bmiColors[i].rgbGreen = bmi->bmiColors[i].rgbBlue = (BYTE)i;
		bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi->bmiHeader.biWidth = width;
		bmi->bmiHeader.biHeight = -height;
		bmi->bmiHeader.biPlanes = 1;
		bmi->bmiHeader.biCompression = BI_RGB;
		bmi->bmiHeader.biBitCount = (WORD)(bitsPixel * 8);
		bmi->bmiHeader.biSizeImage = rowSize * height;
		bmi->bmiHeader.biClrUsed = colors;

		HDC hDC = GetDC(NULL);
		_bmp = std::shared_ptr<pp::ImageData> (new pp::ImageData(CreateDIBitmap(hDC, &bmi->bmiHeader, CBM_INIT, imageBuffer, bmi, DIB_RGB_COLORS)));
		ReleaseDC(NULL, hDC);
		*/
	}

	void setBmp(std::shared_ptr<pp::ImageData> bmp) {
		bmp_ = bmp;
	}

	std::shared_ptr<pp::ImageData> getBmp() {
		return bmp_;
	}

};

#endif // BMP_DELEGATE_H_
