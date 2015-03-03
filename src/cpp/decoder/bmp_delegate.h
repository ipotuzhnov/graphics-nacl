#ifndef __BMP_DELEGATE_H__
#define __BMP_DELEGATE_H__

#include "ddjvu/IBmp.h"

#include "bitmap.h"

class BmpDelegate: public ddjvu::IBmp<decoder::Bitmap> {
private:
	std::shared_ptr<decoder::Bitmap> bmp_;
public:
	BmpDelegate() {
		bmp_ = std::shared_ptr<decoder::Bitmap> ();
	}

	BmpDelegate(int bitsPixel, int colors, int width, int height, int rowSize, char* imageBuffer){
		/* Here you should create bitmap_ = std::shared<ptr> (new T) */
		bmp_ = std::shared_ptr<decoder::Bitmap>(new decoder::Bitmap(bitsPixel, colors, width, height, rowSize, imageBuffer));
	}

	void setBmp(std::shared_ptr<decoder::Bitmap> bmp) {
		bmp_ = bmp;
	}

	std::shared_ptr<decoder::Bitmap> getBmp() {
		return bmp_;
	}

};

#endif //__BMP_DELEGATE_H__
