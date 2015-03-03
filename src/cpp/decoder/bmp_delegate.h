#ifndef BMP_DELEGATE_H_
#define BMP_DELEGATE_H_

#include "ddjvu/IBmp.h"

#include "bitmap.h"

class BmpDelegate: public ddjvu::IBmp<renderer::Bitmap> {
private:
	std::shared_ptr<renderer::Bitmap> bmp_;
public:
	BmpDelegate() {
		bmp_ = std::shared_ptr<renderer::Bitmap> ();
	}

	BmpDelegate(int bitsPixel, int colors, int width, int height, int rowSize, char* imageBuffer){
		// here you should create bitmap_ = std::shared<ptr> (new T)
		//bmp_ = std::make_shared<Bitmap>(Bitmap (bitsPixel, colors, width, height, rowSize, imageBuffer));
		bmp_ = std::shared_ptr<renderer::Bitmap>(new renderer::Bitmap(bitsPixel, colors, width, height, rowSize, imageBuffer));
	}

	void setBmp(std::shared_ptr<renderer::Bitmap> bmp) {
		bmp_ = bmp;
	}

	std::shared_ptr<renderer::Bitmap> getBmp() {
		return bmp_;
	}

};

#endif // BMP_DELEGATE_H_
