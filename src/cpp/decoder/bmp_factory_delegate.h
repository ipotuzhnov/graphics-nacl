#ifndef BMP_FACTORY_DELEGATE_H_
#define BMP_FACTORY_DELEGATE_H_

#include "ddjvu/IBmpFactory.h"
#include "bmp_delegate.h"

class BmpFactoryDelegate: public ddjvu::IBmpFactory<renderer::Bitmap> {
private:
	std::shared_ptr<SafeInstance> safeInstance_;  // Weak pointer.
public:
	BmpFactoryDelegate(std::shared_ptr<SafeInstance> safeInstance) {
		safeInstance_ = safeInstance;
	}

	std::shared_ptr<ddjvu::IBmp<renderer::Bitmap>> createBmp(int bitsPixel, int colors, int width, int height, int rowSize, char * imageBuffer) {
		return std::shared_ptr<BmpDelegate>(new BmpDelegate(bitsPixel, colors, width, height, rowSize, imageBuffer));
	}
};

#endif // BMP_FACTORY_DELEGATE_H_
